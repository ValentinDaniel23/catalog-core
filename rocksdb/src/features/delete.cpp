#include "approcksdb/features.hpp"

namespace approcksdb {

int RunDeleteFeature(const AppConfig& cfg, rocksdb::DB* db)
{
	std::string value;
	rocksdb::Status status;

	Phase(cfg, "delete");
	if (CheckOk(db->Put(rocksdb::WriteOptions(), "delete-key", "value"),
	    "delete-put-key"))
		return 1;

	if (CheckOk(db->Delete(rocksdb::WriteOptions(), "delete-key"), "delete-key"))
		return 1;

	status = db->Get(rocksdb::ReadOptions(), "delete-key", &value);
	if (!status.IsNotFound()) {
		uk_pr_err("rocksdb: expected delete-key deleted, got: %s\n",
			  status.ToString().c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature delete passed\n");
	return 0;
}

} /* namespace approcksdb */