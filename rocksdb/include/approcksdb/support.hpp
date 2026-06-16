#pragma once

#include <uk/print.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <rocksdb/cache.h>
#include <rocksdb/compaction_filter.h>
#include <rocksdb/db.h>
#include <rocksdb/env.h>
#include <rocksdb/file_system.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/sst_file_writer.h>
#include <rocksdb/table.h>
#include <rocksdb/utilities/backup_engine.h>
#include <rocksdb/utilities/cache_dump_load.h>
#include <rocksdb/utilities/checkpoint.h>
#include <rocksdb/utilities/optimistic_transaction_db.h>
#include <rocksdb/utilities/secondary_index.h>
#include <rocksdb/utilities/secondary_index_simple.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/write_batch_with_index.h>
#include <rocksdb/write_batch.h>

#include "approcksdb/types.hpp"

namespace approcksdb {

int CheckOk(const rocksdb::Status& st, const char* step);
int CheckNotFound(const rocksdb::Status& st, const char* step);
int CheckIoOk(const rocksdb::IOStatus& st, const char* step);
bool FeatureEnabled(const AppConfig& cfg, unsigned long feature);
bool HasName(const std::vector<std::string>& names,
	     const std::string& expected);
void Phase(const AppConfig& cfg, const char* name);
rocksdb::Options MakeOptions(void);
rocksdb::Env* GetEnv(const rocksdb::Options& options);
std::string DerivedPath(const char* base, const char* suffix);
void MaybeDeleteFile(const AppConfig& cfg, const rocksdb::Options& options,
	     const char* path);
int MaybeDestroy(const AppConfig& cfg, const rocksdb::Options& options,
	 const char* db_path);

} /* namespace approcksdb */