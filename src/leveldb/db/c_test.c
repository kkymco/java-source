/* Copyright (c) 2011 The LevelDB Authors. All rights reserved.
   Use of this source code is governed by a BSD-style license that can be
   found in the LICENSE file. See the AUTHORS file for names of contributors. */

#include "leveldb/c.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

const char* phase = "";
static char dbname[200];

static void StartPhase(const char* name) {
  fprintf(stderr, "=== Test %s\n", name);
  phase = name;
}

static const char* GetTempDir(void) {
    const char* ret = getenv("TEST_TMPDIR");
    if (ret == NULL || ret[0] == '\0')
        ret = "/tmp";
    return ret;
}

#define CheckNoError(err)                                               \
  if ((err) != NULL) {                                                  \
    fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, __LINE__, phase, (err)); \
    abort();                                                            \
  }

#define CheckCondition(cond)                                            \
  if (!(cond)) {                                                        \
    fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, __LINE__, phase, #cond); \
    abort();                                                            \
  }

static void CheckEqual(const char* expected, const char* v, size_t n) {
  if (expected == NULL && v == NULL) {
    // ok
  } else if (expected != NULL && v != NULL && n == strlen(expected) &&
             memcmp(expected, v, n) == 0) {
    // ok
    return;
  } else {
    fprintf(stderr, "%s: expected '%s', got '%s'\n",
            phase,
            (expected ? expected : "(null)"),
            (v ? v : "(null"));
    abort();
  }
}

static void Free(char** ptr) {
  if (*ptr) {
    free(*ptr);
    *ptr = NULL;
  }
}

static void CheckGet(
    leveldb_t* db,
    const leveldb_readoptions_t* options,
    const char* key,
    const char* expected) {
  char* err = NULL;
  size_t val_len;
  char* val;
  val = leveldb_get(db, options, key, strlen(key), &val_len, &err);
  CheckNoError(err);
  CheckEqual(expected, val, val_len);
  Free(&val);
}

static void CheckIter(leveldb_iterator_t* iter,
                      const char* key, const char* val) {
  size_t len;
  const char* str;
  str = leveldb_iter_key(iter, &len);
  CheckEqual(key, str, len);
  str = leveldb_iter_value(iter, &len);
  CheckEqual(val, str, len);
}

// Callback from leveldb_writebatch_iterate()
static void CheckPut(void* ptr,
                     const char* k, size_t klen,
                     const char* v, size_t vlen) {
  int* state = (int*) ptr;
  CheckCondition(*state < 2);
  switch (*state) {
    case 0:
      CheckEqual("bar", k, klen);
      CheckEqual("b", v, vlen);
      break;
    case 1:
      CheckEqual("box", k, klen);
      CheckEqual("c", v, vlen);
      break;
  }
  (*state)++;
}

// Callback from leveldb_writebatch_iterate()
static void CheckDel(void* ptr, const char* k, size_t klen) {
  int* state = (int*) ptr;
  CheckCondition(*state == 2);
  CheckEqual("bar", k, klen);
  (*state)++;
}

static void CmpDestroy(void* arg) { }

static int CmpCompare(void* arg, const char* a, size_t alen,
                      const char* b, size_t blen) {
  int n = (alen < blen) ? alen : blen;
  int r = memcmp(a, b, n);
  if (r == 0) {
    if (alen < blen) r = -1;
    else if (alen > blen) r = +1;
  }
  return r;
}

static const char* CmpName(void* arg) {
  return "foo";
}

// Custom filter policy
static unsigned char fake_filter_result = 1;
static void FilterDestroy(void* arg) { }
static const char* FilterName(void* arg) {
  return "TestFilter";
}
static char* FilterCreate(
    void* arg,
    const char* const* key_array, const size_t* key_length_array,
    int num_keys,
    size_t* filter_length) {
  *filter_length = 4;
  char* result = malloc(4);
  memcpy(result, "fake", 4);
  return result;
}
unsigned char FilterKeyMatch(
    void* arg,
    const char* key, size_t length,
    const char* filter, size_t filter_length) {
  CheckCondition(filter_length == 4);
  CheckCondition(memcmp(filter, "fake", 4) == 0);
  return fake_filter_result;
}

int main(int argc, char** argv) {
  leveldb_t* db;
  leveldb_comparator_t* cmp;
  leveldb_cache_t* cache;
  leveldb_env_t* env;
  leveldb_options_t* options;
  leveldb_readoptions_t* roptions;
  leveldb_writeoptions_t* woptions;
  char* err = NULL;
  int run = -1;

  CheckCondition(leveldb_major_version() >= 1);
  CheckCondition(leveldb_minor_version() >= 1);

  snprintf(dbname, sizeof(dbname),
           "%s/leveldb_c_test-%d",
           GetTempDir(),
           ((int) geteuid()));

  StartPhase("create_objects");
  cmp = leveldb_comparator_create(NULL, CmpDestroy, CmpCompare, CmpName);
  env = leveldb_create_default_env();
  cache = leveldb_cache_create_lru(100000);

  options = leveldb_options_create();
  leveldb_options_set_comparator(options, cmp);
  leveldb_options_set_error_if_exists(options, 1);
  leveldb_options_set_cache(options, cache);
  leveldb_options_set_env(options, env);
  leveldb_options_set_info_log(options, NULL);
  leveldb_options_set_write_buffer_size(options, 100000);
  leveldb_options_set_paranoid_checks(options, 1);
  leveldb_options_set_max_open_files(options, 10);
  leveldb_options_set_block_size(options, 1024);
  leveldb_options_set_block_restart_interval(options, 8);
  leveldb_options_set_compression(options, leveldb_no_compression);

  roptions = leveldb_readoptions_create();
  leveldb_readoptions_set_verify_checksums(roptions, 1);
  leveldb_readoptions_set_fill_cache(roptions, 0);

  woptions = leveldb_writeoptions_create();
  leveldb_writeoptions_set_sync(woptions, 1);

  StartPhase("destroy");
  leveldb_destroy_db(options, dbname, &err);
  Free(&err);

  StartPhase("open_error");
  db = leveldb_open(options, dbname, &err);
  CheckCondition(err != NULL);
  Free(&err);

  StartPhase("leveldb_free");
  db = leveldb_open(options, dbname, &err);
  CheckCondition(err != NULL);
  leveldb_free(err);
  err = NULL;

  StartPhase("open");
  leveldb_options_set_create_if_missing(options, 1);
  db = leveldb_open(options, dbname, &err);
  CheckNoError(err);
  CheckGet(db, roptions, "foo", NULL);

  StartPhase("put");
  leveldb_put(db, woptions, "foo", 3, "hello", 5, &err);
  CheckNoError(err);
  CheckGet(db, roptions, "foo", "hello");

  StartPhase("compactall");
  leveldb_compact_range(db, NULL, 0, NULL, 0);
  CheckGet(db, roptions, "foo", "hello");

  StartPhase("compactrange");
  leveldb_compact_range(db, "a", 1, "z", 1);
  CheckGet(db, roptions, "foo", "hello");

  StartPhase("writebatch");
  {
    leveldb_writebatch_t* wb = leveldb_writebatch_create();
    leveldb_writebatch_put(wb, "foo", 3, "a", 1);
    leveldb_writebatch_clear(wb);
    leveldb_writebatch_put(wb, "bar", 3, "b", 1);
    leveldb_writebatch_put(wb, "box", 3, "c", 1);
    leveldb_writebatch_delete(wb, "bar", 3);
    leveldb_write(db, woptions, wb, &err);
    CheckNoError(err);
    CheckGet(db, roptions, "foo", "hello");
    CheckGet(db, roptions, "bar", NULL);
    CheckGet(db, roptions, "box", "c");
    int pos = 0;
    leveldb_writebatch_iterate(wb, &pos, CheckPut, CheckDel);
    CheckCondition(pos == 3);
    leveldb_writebatch_destroy(wb);
  }

  StartPhase("iter");
  {
    leveldb_iterator_t* iter = leveldb_create_iterator(db, roptions);
    CheckCondition(!leveldb_iter_valid(iter));
    leveldb_iter_seek_to_first(iter);
    CheckCondition(leveldb_iter_valid(iter));
    Chec