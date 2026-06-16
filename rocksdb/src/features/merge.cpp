#include "approcksdb/features.hpp"

namespace approcksdb {

class CommaAppendMergeOperator : public rocksdb::AssociativeMergeOperator {
public:
	bool Merge(const rocksdb::Slice& key, const rocksdb::Slice* existing_value,
		   const rocksdb::Slice& value, std::string* new_value,
		   rocksdb::Logger* logger) const override
	{
		(void)key;
		(void)logger;

		if (existing_value && !existing_value->empty()) {
			*new_value = existing_value->ToString();
			new_value->push_back(',');
			new_value->append(value.data(), value.size());
		} else {
			new_value->assign(value.data(), value.size());
		}

		return true;
	}

	const char* Name() const override
	{
		return "CommaAppendMergeOperator";
	}
};

int RunMergeFeature(const AppConfig& cfg)
{
	std::shared_ptr<rocksdb::MergeOperator> merge_operator =
		std::make_shared<CommaAppendMergeOperator>();
	rocksdb::Options options = MakeOptions();
	std::string db_path = DerivedPath(cfg.db_path, "-merge");
	std::unique_ptr<rocksdb::DB> db;
	rocksdb::FlushOptions fopts;
	std::string value;

	Phase(cfg, "merge setup");
	options.merge_operator = merge_operator;
	MaybeDestroy(cfg, options, db_path.c_str());
	if (CheckOk(rocksdb::DB::Open(options, db_path, &db), "merge-open"))
		return 1;

	if (CheckOk(db->Put(rocksdb::WriteOptions(), "merge-key", "base"),
	    "merge-put-base"))
		return 1;
	if (CheckOk(db->Merge(rocksdb::WriteOptions(), "merge-key", "one"),
	    "merge-operand-one"))
		return 1;
	if (CheckOk(db->Merge(rocksdb::WriteOptions(), "merge-key", "two"),
	    "merge-operand-two"))
		return 1;

	if (CheckOk(db->Get(rocksdb::ReadOptions(), "merge-key", &value),
	    "merge-get-live"))
		return 1;
	if (value != "base,one,two") {
		uk_pr_err("rocksdb: merge live mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	fopts.wait = true;
	if (CheckOk(db->Flush(fopts), "merge-flush"))
		return 1;
	if (CheckOk(db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr),
	    "merge-compact"))
		return 1;

	value.clear();
	if (CheckOk(db->Get(rocksdb::ReadOptions(), "merge-key", &value),
	    "merge-get-after-compact"))
		return 1;
	if (value != "base,one,two") {
		uk_pr_err("rocksdb: merge compact mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	db.reset();
	Phase(cfg, "merge reopen");
	if (CheckOk(rocksdb::DB::Open(options, db_path, &db), "merge-reopen"))
		return 1;
	value.clear();
	if (CheckOk(db->Get(rocksdb::ReadOptions(), "merge-key", &value),
	    "merge-get-after-reopen"))
		return 1;
	if (value != "base,one,two") {
		uk_pr_err("rocksdb: merge reopen mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	db.reset();
	MaybeDestroy(cfg, options, db_path.c_str());
	uk_pr_info("rocksdb: feature merge passed\n");
	return 0;
}

} /* namespace approcksdb */