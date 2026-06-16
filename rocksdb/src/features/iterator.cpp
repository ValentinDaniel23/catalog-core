#include "approcksdb/features.hpp"

namespace approcksdb {

int RunIteratorFeature(const AppConfig& cfg, rocksdb::DB* db)
{
	std::unique_ptr<rocksdb::Iterator> it;
	unsigned long keys_scanned = 0;

	Phase(cfg, "iterator");
	if (CheckOk(db->Put(rocksdb::WriteOptions(), "iter-a", "1"), "iterator-put-a"))
		return 1;
	if (CheckOk(db->Put(rocksdb::WriteOptions(), "iter-b", "2"), "iterator-put-b"))
		return 1;

	it.reset(db->NewIterator(rocksdb::ReadOptions()));
	for (it->SeekToFirst(); it->Valid(); it->Next())
		keys_scanned++;

	if (!it->status().ok()) {
		uk_pr_err("rocksdb: iterator failed: %s\n", it->status().ToString().c_str());
		return 1;
	}
	if (keys_scanned == 0) {
		uk_pr_err("rocksdb: iterator saw no keys\n");
		return 1;
	}

	uk_pr_info("rocksdb: feature iterator passed (keys_scanned=%lu)\n", keys_scanned);
	return 0;
}

} /* namespace approcksdb */