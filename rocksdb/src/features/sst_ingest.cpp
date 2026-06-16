#include "approcksdb/features.hpp"

namespace approcksdb {

int RunSstIngestFeature(const AppConfig& cfg)
{
	rocksdb::Options options = MakeOptions();
	std::string db_path = DerivedPath(cfg.db_path, "-ingest");
	std::string sst_path = DerivedPath(cfg.db_path, "-ingest.sst");
	std::unique_ptr<rocksdb::DB> db;
	rocksdb::ExternalSstFileInfo file_info;
	std::string value;

	Phase(cfg, "sst ingest setup");
	MaybeDestroy(cfg, options, db_path.c_str());
	MaybeDeleteFile(cfg, options, sst_path.c_str());
	if (CheckOk(rocksdb::DB::Open(options, db_path, &db), "sst-open-db"))
		return 1;

	if (CheckOk(db->Put(rocksdb::WriteOptions(), "ingest-live", "live-value"),
	    "sst-put-live"))
		return 1;

	Phase(cfg, "sst ingest write");
	{
		rocksdb::SstFileWriter writer(rocksdb::EnvOptions(), options);

		if (CheckOk(writer.Open(sst_path), "sst-open-writer"))
			return 1;
		if (CheckOk(writer.Put("ingest-a", "one"), "sst-put-a"))
			return 1;
		if (CheckOk(writer.Put("ingest-b", "two"), "sst-put-b"))
			return 1;
		if (CheckOk(writer.Finish(&file_info), "sst-finish"))
			return 1;
	}

	if (file_info.num_entries != 2 || file_info.file_size == 0) {
		uk_pr_err("rocksdb: unexpected sst metadata entries=%llu size=%llu\n",
			  static_cast<unsigned long long>(file_info.num_entries),
			  static_cast<unsigned long long>(file_info.file_size));
		return 1;
	}

	Phase(cfg, "sst ingest load");
	if (CheckOk(db->IngestExternalFile({sst_path}, rocksdb::IngestExternalFileOptions()),
	    "sst-ingest"))
		return 1;

	if (CheckOk(db->Get(rocksdb::ReadOptions(), "ingest-a", &value), "sst-get-a"))
		return 1;
	if (value != "one") {
		uk_pr_err("rocksdb: ingested value mismatch for ingest-a: got '%s'\n",
			  value.c_str());
		return 1;
	}
	if (CheckOk(db->Get(rocksdb::ReadOptions(), "ingest-live", &value),
	    "sst-get-live"))
		return 1;
	if (value != "live-value") {
		uk_pr_err("rocksdb: live value mismatch after ingest: got '%s'\n",
			  value.c_str());
		return 1;
	}

	db.reset();
	MaybeDeleteFile(cfg, options, sst_path.c_str());
	MaybeDestroy(cfg, options, db_path.c_str());
	uk_pr_info("rocksdb: feature sst-ingest passed\n");
	return 0;
}

} /* namespace approcksdb */