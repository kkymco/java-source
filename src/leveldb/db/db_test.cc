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
    delay_data_sync_.Release_Store(NULL);
    data_sync_error_.Release_Store(NULL);
    no_space_.Release_Store(NULL);
    non_writable_.Release_Store(NULL);
    count_random_reads_ = false;
    manifest_sync_error_.Release_Store(NULL);
    manifest_write_error_.Release_Store(NULL);
  }

  Status NewWritableFile(const std::string& f, WritableFile** r) {
    class DataFile : public WritableFile {
     private:
      SpecialEnv* env_;
      WritableFile* base_;

     public:
      DataFile(SpecialEnv* env, WritableFile* base)
          : env_(env),
            base_(base) {
      }
      ~DataFile() { delete base_; }
      Status Append(const Slice& data) {
        if (env_->no_space_.Acquire_Load() != NULL) {
          // Drop writes on the floor
          return Status::OK();
        } else {
          return base_->Append(data);
        }
      }
      Status Close() { return base_->Close(); }
      Status Flush() { return base_->Flush(); }
      Status Sync() {
        if (env_->data_sync_error_.Acquire_Load() != NULL) {
          return Status::IOError("simulated data sync error");
        }
        while (env_->delay_data_sync_.Acquire_Load() != NULL) {
          DelayMilliseconds(100);
        }
        return base_->Sync();
      }
    };
    class ManifestFile : public WritableFile {
     private:
      SpecialEnv* env_;
      WritableFile* base_;
     public:
      ManifestFile(SpecialEnv* env, WritableFile* b) : env_(env), base_(b) { }
      ~ManifestFile() { delete base_; }
      Status Append(const Slice& data) {
        if (env_->manifest_write_error_.Acquire_Load() != NULL) {
          return Status::IOError("simulated writer error");
        } else {
          return base_->Append(data);
        }
      }
      Status Close() { return base_->Close(); }
      Status Flush() { return base_->Flush(); }
      Status Sync() {
        if (env_->manifest_sync_error_.Acquire_Load() != NULL) {
          return Status::IOError("simulated sync error");
        } else {
          return base_->Sync();
        }
      }
    };

    if (non_writable_.Acquire_Load() != NULL) {
      return Status::IOError("simulated write error");
    }

    Status s = target()->NewWritableFile(f, r);
    if (s.ok()) {
      if (strstr(f.c_str(), ".ldb") != NULL ||
          strstr(f.c_str(), ".log") != NULL) {
        *r = new DataFile(this, *r);
      } else if (strstr(f.c_str(), "MANIFEST") != NULL) {
        *r = new ManifestFile(this, *r);
      }
    }
    return s;
  }

  Status NewRandomAccessFile(const std::string& f, RandomAccessFile** r) {
    class CountingFile : public RandomAccessFile {
     private:
      RandomAccessFile* target_;
      AtomicCounter* counter_;
     public:
      CountingFile(RandomAccessFile* target, AtomicCounter* counter)
          : target_(target), counter_(counter) {
      }
      virtual ~CountingFile() { delete target_; }
      virtual Status Read(uint64_t offset, size_t n, Slice* result,
                          char* scratch) const {
        counter_->Increment();
        return target_->Read(offset, n, result, scratch);
      }
    };

    Status s = target()->NewRandomAccessFile(f, r);
    if (s.ok() &&