#include "approcksdb/features.hpp"

namespace approcksdb {

class RemoveValueCompactionFilter : public rocksdb::CompactionFilter {
public:
	bool Filter(int level, const rocksdb::Slice& key,
		    const rocksdb::Slice& existing_value, std::string* new_value,
		    bool* value_changed) const override
	{
		(void)level;
		(void)key;
		(void)new_value;
		(void)value_changed;

		return existing_value == "drop-me";
	}

	const char* Name() const override
	{
		return "RemoveValueCompactionFilter";
	}
};

int RunCompactionFilterFeature(const AppConfig& cfg)
{
	RemoveValueCompactionFilter filter;
	rocksdb::Options options = MakeOptions();
	std::string db_path = DerivedPath(cfg.db_path, "-compaction-filter");
	std::unique_ptr<rocksdb::DB> db;
	rocksdb::FlushOptions fopts;
	rocksdb::Status status;
	std::string value;

	Phase(cfg, "compaction filter setup");
	options.compaction_filter = &filter;
	MaybeDestroy(cfg, options, db_path.c_str());
	if (CheckOk(rocksdb::DB::Open(options, db_path, &db),
	    "compaction-filter-open"))
		return 1;

	if (CheckOk(db->Put(rocksdb::WriteOptions(), "a-drop", "drop-me"),
	    "compaction-filter-put-drop"))
		return 1;
	if (CheckOk(db->Put(rocksdb::WriteOptions(), "b-keep", "keep-me"),
	    "compaction-filter-put-keep"))
		return 1;
	if (CheckOk(db->Put(rocksdb::WriteOptions(), "z-sentinel", "sentinel"),
	    "compaction-filter-put-sentinel"))
		return 1;

	fopts.wait = true;
	if (CheckOk(db->Flush(fopts), "compaction-filter-flush"))
		return 1;
	if (CheckOk(db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr),
	    "compaction-filter-compact"))
		return 1;

	status = db->Get(rocksdb::ReadOptions(), "a-drop", &value);
	if (CheckNotFound(status, "compaction-filter-get-drop"))
		return 1;
	if (CheckOk(db->Get(rocksdb::ReadOptions(), "b-keep", &value),
	    "compaction-filter-get-keep"))
		return 1;
	if (value != "keep-me") {
		uk_pr_err("rocksdb: compaction filter keep mismatch: got '%s'\n",
			  value.c_str());
		return 1;
	}

	db.reset();
	MaybeDestroy(cfg, options, db_path.c_str());
	uk_pr_info("rocksdb: feature compaction-filter passed\n");
	return 0;
}

} /* namespace approcksdb */