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
    if (s.ok() && count_random_reads_) {
      *r = new CountingFile(*r, &random_read_counter_);
    }
    return s;
  }
};

class DBTest {
 private:
  const FilterPolicy* filter_policy_;

  // Sequence of option configurations to try
  enum OptionConfig {
    kDefault,
    kFilter,
    kUncompressed,
    kEnd
  };
  int option_config_;

 public:
  std::string dbname_;
  SpecialEnv* env_;
  DB* db_;

  Options last_options_;

  DBTest() : option_config_(kDefault),
             env_(new SpecialEnv(Env::Default())) {
    filter_policy_ = NewBloomFilterPolicy(10);
    dbname_ = test::TmpDir() + "/db_test";
    DestroyDB(dbname_, Options());
    db_ = NULL;
    Reopen();
  }

  ~DBTest() {
    delete db_;
    DestroyDB(dbname_, Options());
    delete env_;
    delete filter_policy_;
  }

  // Switch to a fresh database with the next option configuration to
  // test.  Return false if there are no more configurations to test.
  bool ChangeOptions() {
    option_config_++;
    if (option_config_ >= kEnd) {
      return false;
    } else {
      DestroyAndReopen();
      return true;
    }
  }

  // Return the current option configuration.
  Options CurrentOptions() {
    Options options;
    switch (option_config_) {
      case kFilter:
        options.filter_policy = filter_policy_;
        break;
      case kUncompressed:
        options.compression = kNoCompression;
        break;
      default:
        break;
    }
    return options;
  }

  DBImpl* dbfull() {
    return reinterpret_cast<DBImpl*>(db_);
  }

  void Reopen(Options* options = NULL) {
    ASSERT_OK(TryReopen(options));
  }

  void Close() {
    delete db_;
    db_ = NULL;
  }

  void DestroyAndReopen(Options* options = NULL) {
    delete db_;
    db_ = NULL;
    DestroyDB(dbname_, Options());
    ASSERT_OK(TryReopen(options));
  }

  Status TryReopen(Options* options) {
    delete db_;
    db_ = NULL;
    Options opts;
    if (options != NULL) {
      opts = *options;
    } else {
      opts = CurrentOptions();
      opts.create_if_missing = true;
    }
    last_options_ = opts;

    return DB::Open(opts, dbname_, &db_);
  }

  Status Put(const std::string& k, const std::string& v) {
    return db_->Put(WriteOptions(), k, v);
  }

  Status Delete(const std::string& k) {
    return db_->Delete(WriteOptions(), k);
  }

  std::string Get(const std::string& k, const Snapshot* snapshot = NULL) {
    ReadOptions options;
    options.snapshot = snapshot;
    std::string result;
    Status s = db_->Get(options, k, &result);
    if (s.IsNotFound()) {
      result = "NOT_FOUND";
    } else if (!s.ok()) {
      result = s.ToString();
    }
    return result;
  }

  // Return a string that contains all key,value pairs in order,
  // formatted like "(k1->v1)(k2->v2)".
  std::string Contents() {
    std::vector<std::string> forward;
    std::string result;
    Iterator* iter = db_->NewIterator(ReadOptions());
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
      std::string s = IterStatus(iter);
      result.push_back('(');
      result.append(s);
      result.push_back(')');
      forward.push_back(s);
    }

    // Check reverse iteration results are the reverse of forward results
    size_t matched = 0;
    for (iter->SeekToLast(); iter->Valid(); iter->Prev()) {
      ASSERT_LT(matched, forward.size());
      ASSERT_EQ(IterStatus(iter), forward[forward.size() - matched - 1]);
      matched++;
    }
    ASSERT_EQ(matched, forward.size());

    delete iter;
    return result;
  }

  std::string AllEntriesFor(const Slice& user_key) {
    Iterator* iter = dbfull()->TEST_NewInternalIterator();
    InternalKey target(user_key, kMaxSequenceNumber, kTypeValue);
    iter->Seek(target.Encode());
    std::string result;
    if (!iter->status().ok()) {
      result = iter->status().ToString();
    } else {
      result = "[ ";
      bool first = true;
      while (iter->Valid()) {
        ParsedInternalKey ikey;
        if (!ParseInternalKey(iter->key(), &ikey)) {
          result += "CORRUPTED";
        } else {
          if (last_options_.comparator->Compare(ikey.user_key, user_key) != 0) {
            break;
          }
          if (!first) {
            result += ", ";
          }
          first = false;
          switch (ikey.type) {
            case kTypeValue:
              result += iter->value().ToString();
              break;
            case kTypeDeletion:
              result += "DEL";
              break;
          }
        }
        iter->Next();
      }
      if (!first) {
        result += " ";
      }
      result += "]";
    }
    delete iter;
    return result;
  }

  int NumTableFilesAtLevel(int level) {
    std::string property;
    ASSERT_TRUE(
        db_->GetProperty("leveldb.num-files-at-level" + NumberToString(level),
                         &property));
    return atoi(property.c_str());
  }

  int TotalTableFiles() {
    int result = 0;
    for (int level = 0; level < config::kNumLevels; level++) {
      result += NumTableFilesAtLevel(level);
    }
    return result;
  }

  // Return spread of files per level
  std::string FilesPerLevel() {
    std::string result;
    int last_non_zero_offset = 0;
    for (int level = 0; level < config::kNumLevels; level++) {
      int f = NumTableFilesAtLevel(level);
      char buf[100];
      snprintf(buf, sizeof(buf), "%s%d", (level ? "," : ""), f);
      result += buf;
      if (f > 0) {
        last_non_zero_offset = result.size();
      }
    }
    result.resize(last_non_zero_offset);
    return result;
  }

  int CountFiles() {
    std::vector<std::string> files;
    env_->GetChildren(dbname_, &files);
    return static_cast<int>(files.size());
  }

  uint64_t Size(const Slice& start, const Slice& limit) {
    Range r(start, limit);
    uint64_t size;
    db_->GetApproximateSizes(&r, 1, &size);
    return size;
  }

  void Compact(const Slice& start, const Slice& limit) {
    db_->CompactRange(&start, &limit);
  }

  // Do n memtable compactions, each of which produces an sstable
  // covering the range [small,large].
  void MakeTables(int n, const std::string& small, const std::string& large) {
    for (int i = 0; i < n; i++) {
      Put(small, "begin");
      Put(large, "end");
      dbfull()->TEST_CompactMemTable();
    }
  }

  // Prevent pushing of new sstables into deeper levels by adding
  // tables that cover a specified range to all levels.
  void FillLevels(const std::string& smallest, const std::string& largest) {
    MakeTables(config::kNumLevels, smallest, largest);
  }

  void DumpFileCounts(const char* label) {
    fprintf(stderr, "---\n%s:\n", label);
    fprintf(stderr, "maxoverlap: %lld\n",
            static_cast<long long>(
                dbfull()->TEST_MaxNextLevelOverlappingBytes()));
    for (int level = 0; level < config::kNumLevels; level++) {
      int num = NumTableFilesAtLevel(level);
      if (num > 0) {
        fprintf(stderr, "  level %3d : %d files\n", level, num);
      }
    }
  }

  std::string DumpSSTableList() {
    std::string property;
    db_->GetProperty("leveldb.sstables", &property);
    return property;
  }

  std::string IterStatus(Iterator* iter) {
    std::string result;
    if (iter->Valid()) {
      result = iter->key().ToString() + "->" + iter->value().ToString();
    } else {
      result = "(invalid)";
    }
    return result;
  }

  bool DeleteAnSSTFile() {
    std::vector<std::string> filenames;
    ASSERT_OK(env_->GetChildren(dbname_, &filenames));
    uint64_t number;
    FileType type;
    for (size_t i = 0; i < filenames.size(); i++) {
      if (ParseFileName(filenames[i], &number, &type) && type == kTableFile) {
        ASSERT_OK(env_->DeleteFile(TableFileName(dbname_, number)));
        return true;
      }
    }
    return false;
  }

  // Returns number of files renamed.
  int RenameLDBToSST() {
    std::vector<std::string> filenames;
    ASSERT_OK(env_->GetChildren(dbname_, &filenames));
    uint64_t number;
    FileType type;
    int files_renamed = 0;
    for (size_t i = 0; i < filenames.size(); i++) {
      if (ParseFileName(filenames[i], &number, &type) && type == kTableFile) {
        const std::string from = TableFileName(dbname_, number);
        const std::string to = SSTTableFileName(dbname_, number);
        ASSERT_OK(env_->RenameFile(from, to));
        files_renamed++;
      }
    }
    return files_renamed;
  }
};

TEST(DBTest, Empty) {
  do {
    ASSERT_TRUE(db_ != NULL);
    ASSERT_EQ("NOT_FOUND", Get("foo"));
  } while (ChangeOptions());
}

TEST(DBTest, ReadWrite) {
  do {
    ASSERT_OK(Put("foo", "v1"));
    ASSERT_EQ("v1", Get("foo"));
    ASSERT_OK(Put("bar", "v2"));
    ASSERT_OK(Put("foo", "v3"));
    ASSERT_EQ("v3", Get("foo"));
    ASSERT_EQ("v2", Get("bar"));
  } while (ChangeOptions());
}

TEST(DBTest, PutDeleteGet) {
  do {
    ASSERT_OK(db_->Put(WriteOptions(), "foo", "v1"));
    ASSERT_EQ("v1", Get("foo"));
    ASSERT_OK(db_->Put(WriteOptions(), "foo", "v2"));
    ASSERT_EQ("v2", Get("foo"));
    ASSERT_OK(db_->Delete(WriteOptions(), "foo"));
    ASSERT_EQ("NOT_FOUND", Get("foo"));
  } while (ChangeOptions());
}

TEST(DBTest, GetFromImmutableLayer) {
  do {
    Options options = CurrentOptions();
    options.env = env_;
    options.write_buffer_size = 100000;  // Small write buffer
    Reopen(&options);

    ASSERT_OK(Put("foo", "v1"));
    ASSERT_EQ("v1", Get("foo"));

    env_->delay_data_sync_.Release_Store(env_);      // Block sync calls
    Put("k1", std::string(100000, 'x'));             // Fill memtable
    Put("k2", std::string(100000, 'y'));             // Trigger compaction
    ASSERT_EQ("v1", Get("foo"));
    env_->delay_data_sync_.Release_Store(NULL);      // Release sync calls
  } while (ChangeOptions());
}

TEST(DBTest, GetFromVersions) {
  do {
    ASSERT_OK(Put("foo", "v1"));
    dbfull()->TEST_CompactMemTable();
    ASSERT_EQ("v1", Get("foo"));
  } while (ChangeOptions());
}

TEST(DBTest, GetSnapshot) {
  do {
    // Try with both a short key and a long key
    for (int i = 0; i < 2; i++) {
      std::string key = (i == 0) ? std::string("foo") : std::string(200, 'x');
      ASSERT_OK(Put(key, "v1"));
      const Snapshot* s1 = db_->GetSnapshot();
      ASSERT_OK(Put(key, "v2"));
      ASSERT_EQ("v2", Get(key));
      ASSERT_EQ("v1", Get(key, s1));
      dbfull()->TEST_CompactMemTable();
      ASSERT_EQ("v2", Get(key));
      ASSERT_EQ("v1", Get(key, s1));
      db_->ReleaseSnapshot(s1);
    }
  } while (ChangeOptions());
}

TEST(DBTest, GetLevel0Ordering) {
  do {
    // Check that we process level-0 files in correct order.  The code
    // below generates two level-0 files where the earlier one comes
    // before the later one in the level-0 file list since the earlier
    // one has a smaller "smallest" key.
    ASSERT_OK(Put("bar", "b"));
    ASSERT_OK(Put("foo", "v1"));
    dbfull()->TEST_CompactMemTable();
    ASSERT_OK(Put("foo", "v2"));
    dbfull()->TEST_CompactMemTable();
    ASSERT_EQ("v2", Get("foo"));
  } while (ChangeOptions());
}

TEST(DBTest, GetOrderedByLevels) {
  do {
    ASSERT_OK(Put("foo", "v1"));
    Compact("a", "z");
    ASSERT_EQ("v1", Get("foo"));
    ASSERT_OK(Put("foo", "v2"));
    ASSERT_EQ("v2", Get("foo"));
    dbfull()->TEST_CompactMemTable();
    ASSERT_EQ("v2", Get("foo"));
  } while (ChangeOptions());
}

TEST(DBTest, GetPicksCorrectFile) {
  do {
    // Arrange to have multiple files in a non-level-0 level.
    ASSERT_OK(Put("a", "va"));
    Compact("a", "b");
    ASSERT_OK(Put("x", "vx"));
    Compact("x", "y");
    ASSERT_OK(Put("f", "vf"));
    Compact("f", "g");
    ASSERT_EQ("va", Get("a"));
    ASSERT_EQ("vf", Get("f"));
    ASSERT_EQ("vx", Get("x"));
  } while (ChangeOptions());
}

TEST(DBTest, GetEncountersEmptyLevel) {
  do {
    // Arrange for the following to happen:
    //   * sstable A in level 0
    //   * nothing in level 1
    //   * sstable B in level 2
    // Then do enough Get() calls to arrange for an automatic compaction
    // of sstable A.  A bug would cause the compaction to be marked as
    // occuring at level 1 (instead of the correct level 0).

    // Step 1: First place sstables in levels 0 and 2
    int compaction_count = 0;
    while (NumTableFilesAtLevel(0) == 0 ||
           NumTableFilesAtLevel(2) == 0) {
      ASSERT_LE(compaction_count, 100) << "could not fill levels 0 and 2";
      compaction_count++;
      Put("a", "begin");
      Put("z", "end");
      dbfull()->TEST_CompactMemTable();
    }

    // Step 2: clear level 1 if necessary.
    dbfull()->TEST_CompactRange(1, NULL, NULL);
    ASSERT_EQ(NumTableFilesAtLevel(0), 1);
    ASSERT_EQ(NumTableFilesAtLevel(1), 0);
    ASSERT_EQ(NumTableFilesAtLevel(2), 1);

    // Step 3: read a bunch of times
    for (int i = 0; i < 1000; i++) {
      ASSERT_EQ("NOT_FOUND", Get("missing"));
    }

    // Step 4: Wait for compaction to finish
    DelayMilliseconds(1000);

    ASSERT_EQ(NumTableFilesAtLevel(0), 0);
  } while (ChangeOptions());
}

TEST(DBTest, IterEmpty) {
  Iterator* iter = db_->NewIterator(ReadOptions());

  iter->SeekToFirst();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  iter->SeekToLast();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  iter->Seek("foo");
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  delete iter;
}

TEST(DBTest, IterSingle) {
  ASSERT_OK(Put("a", "va"));
  Iterator* iter = db_->NewIterator(ReadOptions());

  iter->SeekToFirst();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "(invalid)");
  iter->SeekToFirst();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  iter->SeekToLast();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "(invalid)");
  iter->SeekToLast();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  iter->Seek("");
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  iter->Seek("a");
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  iter->Seek("b");
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  delete iter;
}

TEST(DBTest, IterMulti) {
  ASSERT_OK(Put("a", "va"));
  ASSERT_OK(Put("b", "vb"));
  ASSERT_OK(Put("c", "vc"));
  Iterator* iter = db_->NewIterator(ReadOptions());

  iter->SeekToFirst();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "b->vb");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "c->vc");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "(invalid)");
  iter->SeekToFirst();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  iter->SeekToLast();
  ASSERT_EQ(IterStatus(iter), "c->vc");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "b->vb");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "(invalid)");
  iter->SeekToLast();
  ASSERT_EQ(IterStatus(iter), "c->vc");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  iter->Seek("");
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Seek("a");
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Seek("ax");
  ASSERT_EQ(IterStatus(iter), "b->vb");
  iter->Seek("b");
  ASSERT_EQ(IterStatus(iter), "b->vb");
  iter->Seek("z");
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  // Switch from reverse to forward
  iter->SeekToLast();
  iter->Prev();
  iter->Prev();
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "b->vb");

  // Switch from forward to reverse
  iter->SeekToFirst();
  iter->Next();
  iter->Next();
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "b->vb");

  // Make sure iter stays at snapshot
  ASSERT_OK(Put("a",  "va2"));
  ASSERT_OK(Put("a2", "va3"));
  ASSERT_OK(Put("b",  "vb2"));
  ASSERT_OK(Put("c",  "vc2"));
  ASSERT_OK(Delete("b"));
  iter->SeekToFirst();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "b->vb");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "c->vc");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "(invalid)");
  iter->SeekToLast();
  ASSERT_EQ(IterStatus(iter), "c->vc");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "b->vb");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  delete iter;
}

TEST(DBTest, IterSmallAndLargeMix) {
  ASSERT_OK(Put("a", "va"));
  ASSERT_OK(Put("b", std::string(100000, 'b')));
  ASSERT_OK(Put("c", "vc"));
  ASSERT_OK(Put("d", std::string(100000, 'd')));
  ASSERT_OK(Put("e", std::string(100000, 'e')));

  Iterator* iter = db_->NewIterator(ReadOptions());

  iter->SeekToFirst();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "b->" + std::string(100000, 'b'));
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "c->vc");
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "d->" + std::string(100000, 'd'));
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "e->" + std::string(100000, 'e'));
  iter->Next();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  iter->SeekToLast();
  ASSERT_EQ(IterStatus(iter), "e->" + std::string(100000, 'e'));
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "d->" + std::string(100000, 'd'));
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "c->vc");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "b->" + std::string(100000, 'b'));
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "a->va");
  iter->Prev();
  ASSERT_EQ(IterStatus(iter), "(invalid)");

  delete iter;
}

TEST(DBTest, IterMultiWithDelete) {
  do {
    ASSERT_OK(Put("a", "va"));
    ASSERT_OK(Put("b", "vb"));
    ASSERT_OK(Put("c", "vc"));
    ASSERT_OK(Delete("b"));
    ASSERT_EQ("NOT_FOUND", Get("b"));

    Iterator* iter = db_->NewIterator(ReadOptions());
    iter->Seek("c");
    ASSERT_EQ(IterStatus(iter), "c->vc");
    iter->Prev();
    ASSERT_EQ(IterStatus(iter), "a->va");
    delete iter;
  } while (ChangeOptions());
}

TEST(DBTest, Recover) {
  do {
    ASSERT_OK(Put("foo", "v1"));
    ASSERT_OK(Put("baz", "v5"));

    Reopen();
    ASSERT_EQ("v1", Get("foo"));

    ASSERT_EQ("v1", Get("foo"));
    ASSERT_EQ("v5", Get("baz"));
    ASSERT_OK(Put("bar", "v2"));
    ASSERT_OK(Put("foo", "v3"));

    Reopen();
    ASSERT_EQ("v3", Get("foo"));
    ASSERT_OK(Put("foo", "v4"));
    ASSERT_EQ("v4", Get("foo"));
    ASSERT_EQ("v2", Get("bar"));
    ASSERT_EQ("v5", Get("baz"));
  } while (ChangeOptions());
}

TEST(DBTest, RecoveryWithEmptyLog) {
  do {
    ASSERT_OK(Put("foo", "v1"));
    ASSERT_OK(Put("foo", "v2"));
    Reopen();
    Reopen();
    ASSERT_OK(Put("foo", "v3"));
    Reopen();
    ASSERT_EQ("v3", Get("foo"));
  } while (ChangeOptions());
}

// Check that writes done during a memtable compaction are recovered
// if the database is shutdown during the memtable compaction.
TEST(DBTest, RecoverDuringMemtableCompaction) {
  do {
    Options options = CurrentOptions();
    options.env = env_;
    options.write_buffer_size = 1000000;
    Reopen(&options);

    // Trigger a long memtable compaction and reopen the database during it
    ASSERT_OK(Put("foo", "v1"));                         // Goes to 1st log file
    ASSERT_OK(Put("big1", std::string(10000000, 'x')));  // Fills memtable
    ASSERT_OK(Put("big2", std::string(1000, 'y')));      // Triggers compaction
    ASSERT_OK(Put("bar", "v2"));                         // Goes to new log file

    Reopen(&options);
    ASSERT_EQ("v1", Get("foo"));
    ASSERT_EQ("v2", Get("bar"));
    ASSERT_EQ(std::string(10000000, 'x'), Get("big1"));
    ASSERT_EQ(std::string(1000, 'y'), Get("big2"));
  } while (ChangeOptions());
}

static std::string Key(int i) {
  char buf[100];
  snprintf(buf, sizeof(buf), "key%06d", i);
  return std::string(buf);
}

TEST(DBTest, MinorCompactionsHappen) {
  Options options = CurrentOptions();
  options.write_buffer_size = 10000;
  Reopen(&options);

  const int N = 500;

  int starting_num_tables = TotalTableFiles();
  for (int i = 0; i < N; i++) {
    ASSERT_OK(Put(Key(i), Key(i) + std::string(1000, 'v')));
  }
  int ending_num_tables = TotalTableFiles();
  ASSERT_GT(ending_num_tables, starting_num_tables);

  for (int i = 0; i < N; i++) {
    ASSERT_EQ(Key(i) + std::string(1000, 'v'), Get(Key(i)));
  }

  Reopen();

  for (int i = 0; i < N; i++) {
    ASSERT_EQ(Key(i) + std::string(1000, 'v'), Get(Key(i)));
  }
}

TEST(DBTest, RecoverWithLargeLog) {
  {
    Options options = CurrentOptions();
    Reopen(&options);
    ASSERT_OK(Put("big1", std::string(200000, '1')));
    ASSERT_OK(Put("big2", std::string(200000, '2')));
    ASSERT_OK(Put("small3", std::string(10, '3')));
    ASSERT_OK(Put("small4", std::string(10, '4')));
    ASSERT_EQ(NumTableFilesAtLevel(0), 0);
  }

  // Make sure that if we re-open with a small write buffer size that
  // we flush table files in the middle of a large log file.
  Options options = CurrentOptions();
  options.write_buffer_size = 100000;
  Reopen(&options);
  ASSERT_EQ(NumTableFilesAtLevel(0), 3);
  ASSERT_EQ(std::string(200000,