#include "approcksdb/features.hpp"

namespace approcksdb {

int RunColumnFamilyFeature(const AppConfig& cfg)
{
	rocksdb::Options options = MakeOptions();
	std::string db_path = DerivedPath(cfg.db_path, "-cf");
	std::unique_ptr<rocksdb::DB> db;
	std::vector<std::string> cf_names;
	rocksdb::ColumnFamilyHandle* cf_a = nullptr;
	rocksdb::ColumnFamilyHandle* cf_b = nullptr;
	std::string value;

	Phase(cfg, "column family setup");
	MaybeDestroy(cfg, options, db_path.c_str());
	if (CheckOk(rocksdb::DB::Open(options, db_path, &db), "cf-open"))
		return 1;

	if (CheckOk(db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), "cf-a", &cf_a),
	    "cf-create-a"))
		return 1;
	if (CheckOk(db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), "cf-b", &cf_b),
	    "cf-create-b")) {
		delete cf_a;
		return 1;
	}

	if (CheckOk(db->Put(rocksdb::WriteOptions(), cf_a, "cf-a-key", "cf-a-value"),
	    "cf-put-a")) {
		delete cf_b;
		delete cf_a;
		return 1;
	}
	if (CheckOk(db->Put(rocksdb::WriteOptions(), cf_b, "cf-b-key", "cf-b-value"),
	    "cf-put-b")) {
		delete cf_b;
		delete cf_a;
		return 1;
	}

	if (CheckOk(rocksdb::DB::ListColumnFamilies(rocksdb::DBOptions(options),
				       db_path, &cf_names),
	    "cf-list-before-drop")) {
		delete cf_b;
		delete cf_a;
		return 1;
	}
	if (!HasName(cf_names, rocksdb::kDefaultColumnFamilyName) ||
	    !HasName(cf_names, "cf-a") || !HasName(cf_names, "cf-b")) {
		uk_pr_err("rocksdb: column family list missing expected names\n");
		delete cf_b;
		delete cf_a;
		return 1;
	}

	if (CheckOk(db->DropColumnFamily(cf_b), "cf-drop-b")) {
		delete cf_b;
		delete cf_a;
		return 1;
	}
	delete cf_b;
	cf_b = nullptr;

	cf_names.clear();
	if (CheckOk(rocksdb::DB::ListColumnFamilies(rocksdb::DBOptions(options),
				       db_path, &cf_names),
	    "cf-list-after-drop")) {
		delete cf_a;
		return 1;
	}
	if (HasName(cf_names, "cf-b")) {
		uk_pr_err("rocksdb: dropped column family still listed\n");
		delete cf_a;
		return 1;
	}

	delete cf_a;
	cf_a = nullptr;
	db.reset();

	Phase(cfg, "column family reopen");
	std::vector<rocksdb::ColumnFamilyDescriptor> descriptors;
	std::vector<rocksdb::ColumnFamilyHandle*> handles;
	std::unique_ptr<rocksdb::DB> reopened;

	descriptors.emplace_back(rocksdb::kDefaultColumnFamilyName,
				 rocksdb::ColumnFamilyOptions());
	descriptors.emplace_back("cf-a", rocksdb::ColumnFamilyOptions());
	if (CheckOk(rocksdb::DB::Open(rocksdb::DBOptions(options), db_path,
				      descriptors, &handles, &reopened),
	    "cf-reopen"))
		return 1;

	if (handles.size() != 2) {
		uk_pr_err("rocksdb: column family reopen returned %lu handles\n",
			  static_cast<unsigned long>(handles.size()));
		for (auto* handle : handles)
			delete handle;
		return 1;
	}
	if (CheckOk(reopened->Get(rocksdb::ReadOptions(), handles[1], "cf-a-key", &value),
	    "cf-get-a")) {
		for (auto* handle : handles)
			delete handle;
		return 1;
	}
	if (value != "cf-a-value") {
		uk_pr_err("rocksdb: column family value mismatch: got '%s'\n", value.c_str());
		for (auto* handle : handles)
			delete handle;
		return 1;
	}

	for (auto* handle : handles)
		delete handle;

	reopened.reset();
	MaybeDestroy(cfg, options, db_path.c_str());
	uk_pr_info("rocksdb: feature column-family passed\n");
	return 0;
}

} /* namespace approcksdb */