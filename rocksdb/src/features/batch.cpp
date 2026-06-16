#include "approcksdb/features.hpp"

namespace approcksdb {

int RunBatchFeature(const AppConfig& cfg, rocksdb::DB* db)
{
	rocksdb::WriteBatch batch;
	std::string value;

	Phase(cfg, "batch");
	batch.Put("k1", "v1");
	batch.Put("k2", "v2");
	batch.Delete("k1");

	if (CheckOk(db->Write(rocksdb::WriteOptions(), &batch), "batch-write"))
		return 1;

	if (!db->Get(rocksdb::ReadOptions(), "k1", &value).IsNotFound()) {
		uk_pr_err("rocksdb: expected k1 deleted after write batch\n");
		return 1;
	}

	if (CheckOk(db->Get(rocksdb::ReadOptions(), "k2", &value), "batch-get-k2"))
		return 1;

	if (value != "v2") {
		uk_pr_err("rocksdb: value mismatch for k2: got '%s'\n", value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature batch passed\n");
	return 0;
}

} /* namespace approcksdb */