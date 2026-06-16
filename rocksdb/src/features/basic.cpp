#include "approcksdb/features.hpp"

namespace approcksdb {

int RunBasicFeature(const AppConfig& cfg, rocksdb::DB* db)
{
	std::string value;

	Phase(cfg, "basic");
	if (CheckOk(db->Put(rocksdb::WriteOptions(), "hello", "unikraft"),
	    "basic-put-hello"))
		return 1;

	if (CheckOk(db->Get(rocksdb::ReadOptions(), "hello", &value),
	    "basic-get-hello"))
		return 1;

	if (value != "unikraft") {
		uk_pr_err("rocksdb: value mismatch for hello: got '%s'\n", value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature basic passed\n");
	return 0;
}

} /* namespace approcksdb */