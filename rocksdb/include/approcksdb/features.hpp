#pragma once

#include "approcksdb/support.hpp"

namespace approcksdb {

int RunBasicFeature(const AppConfig& cfg, rocksdb::DB* db);
int RunBatchFeature(const AppConfig& cfg, rocksdb::DB* db);
int RunFlushFeature(const AppConfig& cfg, rocksdb::DB* db);
int RunSnapshotFeature(const AppConfig& cfg, rocksdb::DB* db);
int RunIteratorFeature(const AppConfig& cfg, rocksdb::DB* db);
int RunDeleteFeature(const AppConfig& cfg, rocksdb::DB* db);
int RunReopenFeature(const AppConfig& cfg, const rocksdb::Options& options,
	     std::unique_ptr<rocksdb::DB>* db);
int RunCheckpointFeature(const AppConfig& cfg, rocksdb::DB* db,
		 const rocksdb::Options& options);
int RunTransactionFeature(const AppConfig& cfg,
		  const rocksdb::Options& options);
int RunBackupFeature(const AppConfig& cfg, rocksdb::DB* db,
	     const rocksdb::Options& options);
int RunCacheDumpLoadFeature(const AppConfig& cfg);
int RunSecondaryIndexFeature(const AppConfig& cfg,
		     const rocksdb::Options& options);
int RunMergeFeature(const AppConfig& cfg);
int RunColumnFamilyFeature(const AppConfig& cfg);
int RunSstIngestFeature(const AppConfig& cfg);
int RunWbwiFeature(const AppConfig& cfg);
int RunOptimisticTransactionFeature(const AppConfig& cfg);
int RunCompactionFilterFeature(const AppConfig& cfg);

} /* namespace approcksdb */