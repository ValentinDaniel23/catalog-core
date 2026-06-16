#include "approcksdb/features.hpp"

namespace approcksdb {

int RunFlushFeature(const AppConfig& cfg, rocksdb::DB* db)
{
	rocksdb::FlushOptions fopts;

	Phase(cfg, "flush");
	if (CheckOk(db->Put(rocksdb::WriteOptions(), "flush-key", "flush-value"),
	    "flush-put-key"))
		return 1;

	fopts.wait = true;
	if (CheckOk(db->Flush(fopts), "flush-db"))
		return 1;

	uk_pr_info("rocksdb: feature flush passed\n");
	return 0;
}

} /* namespace approcksdb */