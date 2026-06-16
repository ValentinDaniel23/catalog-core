#include "approcksdb/features.hpp"

namespace approcksdb {

int RunWbwiFeature(const AppConfig& cfg)
{
	rocksdb::Options options = MakeOptions();
	std::string db_path = DerivedPath(cfg.db_path, "-wbwi");
	std::unique_ptr<rocksdb::DB> db;
	rocksdb::WriteBatchWithIndex batch(rocksdb::BytewiseComparator(), 0, true);
	const rocksdb::DBOptions db_options(options);
	std::string value;
	rocksdb::Status status;

	Phase(cfg, "write batch with index setup");
	MaybeDestroy(cfg, options, db_path.c_str());
	if (CheckOk(rocksdb::DB::Open(options, db_path, &db), "wbwi-open"))
		return 1;
	if (CheckOk(db->Put(rocksdb::WriteOptions(), "wbwi-existing", "db-value"),
	    "wbwi-put-existing"))
		return 1;

	if (CheckOk(batch.Put("wbwi-a", "value-a"), "wbwi-batch-put-a"))
		return 1;
	if (CheckOk(batch.Put("wbwi-b", "value-b"), "wbwi-batch-put-b"))
		return 1;
	if (CheckOk(batch.Put("wbwi-existing", "batch-value"),
	    "wbwi-batch-put-existing"))
		return 1;
	if (CheckOk(batch.Delete("wbwi-a"), "wbwi-batch-delete-a"))
		return 1;

	status = batch.GetFromBatch(db_options, "wbwi-b", &value);
	if (CheckOk(status, "wbwi-get-from-batch"))
		return 1;
	if (value != "value-b") {
		uk_pr_err("rocksdb: wbwi batch value mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	Phase(cfg, "write batch with index overlay");
	std::unique_ptr<rocksdb::Iterator> overlay(
		batch.NewIteratorWithBase(db->NewIterator(rocksdb::ReadOptions())));
	overlay->Seek("wbwi-existing");
	if (!overlay->Valid()) {
		uk_pr_err("rocksdb: wbwi overlay iterator is invalid\n");
		return 1;
	}
	if (!overlay->status().ok()) {
		uk_pr_err("rocksdb: wbwi overlay iterator failed: %s\n",
			  overlay->status().ToString().c_str());
		return 1;
	}
	if (overlay->key().ToString() != "wbwi-existing" ||
	    overlay->value().ToString() != "batch-value") {
		uk_pr_err("rocksdb: wbwi overlay mismatch key='%s' value='%s'\n",
			  overlay->key().ToString().c_str(),
			  overlay->value().ToString().c_str());
		return 1;
	}

	if (CheckOk(db->Write(rocksdb::WriteOptions(), batch.GetWriteBatch()),
	    "wbwi-write"))
		return 1;

	if (CheckOk(db->Get(rocksdb::ReadOptions(), "wbwi-existing", &value),
	    "wbwi-get-existing"))
		return 1;
	if (value != "batch-value") {
		uk_pr_err("rocksdb: wbwi persisted value mismatch: got '%s'\n",
			  value.c_str());
		return 1;
	}

	status = db->Get(rocksdb::ReadOptions(), "wbwi-a", &value);
	if (CheckNotFound(status, "wbwi-get-deleted-a"))
		return 1;

	uk_pr_info("rocksdb: feature write-batch-with-index passed\n");
	return 0;
}

} /* namespace approcksdb */