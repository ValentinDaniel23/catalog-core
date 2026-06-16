#include "approcksdb/features.hpp"

namespace approcksdb {

int RunReopenFeature(const AppConfig& cfg, const rocksdb::Options& options,
		    std::unique_ptr<rocksdb::DB>* db)
{
	std::string value;
	rocksdb::FlushOptions fopts;

	Phase(cfg, "reopen");
	if (CheckOk((*db)->Put(rocksdb::WriteOptions(), "reopen-key", "reopen-value"),
	    "reopen-put-key"))
		return 1;

	fopts.wait = true;
	if (CheckOk((*db)->Flush(fopts), "reopen-flush"))
		return 1;

	db->reset();
	if (CheckOk(rocksdb::DB::Open(options, cfg.db_path, db), "reopen-open"))
		return 1;

	if (CheckOk((*db)->Get(rocksdb::ReadOptions(), "reopen-key", &value),
	    "reopen-get-key"))
		return 1;

	if (value != "reopen-value") {
		uk_pr_err("rocksdb: reopen mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature reopen passed\n");
	return 0;
}

} /* namespace approcksdb */