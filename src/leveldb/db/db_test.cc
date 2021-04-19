// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "leveldb/db.h"
#include "leveldb/filter_policy.h"
#include "db/db_impl.h"
#include "db/filename.h"
#include "db/version_set.h"
#include "db/write_batch_internal.h"
#include "leveldb/cache.h"
#include "leveldb/env.h"
#include "leveldb/table.h"
#include "util/hash.h"
#include "util/logging.h"
#include "util/mutexlock.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace leveldb {

static std::string RandomString(Random* rnd, int len) {
  std::string r;
  test::RandomString(rnd, len, &r);
  return r;
}

namespace {
class AtomicCounter {
 private:
  port::Mutex mu_;
  int count_;
 public:
  AtomicCounter() : count_(0) { }
  void Increment() {
    IncrementBy(1);
  }
  void IncrementBy(int count) {
    MutexLock l(&mu_);
    count_ += count;
  }
  int Read() {
    MutexLock l(&mu_);
    return count_;
  }
  void Reset() {
    MutexLock l(&mu_);
    count_ = 0;
  }
};

void DelayMilliseconds(int millis) {
  Env::Default()->SleepForMicroseconds(millis * 1000);
}
}

// Special Env used to delay background operations
class SpecialEnv : public EnvWrapper {
 public:
  // sstable/log Sync() calls are blocked while this pointer is non-NULL.
  port::AtomicPointer delay_data_sync_;

  // sstable/log Sync() calls return an error.
  port::AtomicPointer data_sync_error_;

  // Simulate no-space errors while this pointer is non-NULL.
  port::AtomicPointer no_space_;

  // Simulate non-writable file system while this pointer is non-NULL
  port::AtomicPointer non_writable_;

  // Force sync of manifest files to fail while this pointer is non-NULL
  port::AtomicPointer manifest_sync_error_;

  // Force write to manifest files to fail while this pointer is non-NULL
  port::AtomicPointer manifest_write_error_;

  bool count_random_reads_;
  AtomicCounter random_read_counter_;

  explicit SpecialEnv(Env* base) : EnvWrapper(base) {
    delay_data_sync_.Release