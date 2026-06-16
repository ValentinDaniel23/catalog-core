#include "approcksdb/features.hpp"

namespace approcksdb {

int RunCheckpointFeature(const AppConfig& cfg, rocksdb::DB* db,
		 const rocksdb::Options& options)
{
	rocksdb::Checkpoint* raw_checkpoint = nullptr;
	std::unique_ptr<rocksdb::Checkpoint> checkpoint;
	std::unique_ptr<rocksdb::DB> checkpoint_db;
	rocksdb::FlushOptions fopts;
	uint64_t checkpoint_seq = 0;
	std::string value;

	Phase(cfg, "checkpoint setup");
	if (CheckOk(db->Put(rocksdb::WriteOptions(), "checkpoint-key", "checkpoint-value"),
	    "checkpoint-put-key"))
		return 1;

	fopts.wait = true;
	if (CheckOk(db->Flush(fopts), "checkpoint-flush"))
		return 1;

	Phase(cfg, "checkpoint handle");
	if (CheckOk(rocksdb::Checkpoint::Create(db, &raw_checkpoint),
	    "checkpoint-handle"))
		return 1;
	checkpoint.reset(raw_checkpoint);

	Phase(cfg, "checkpoint create");
	if (CheckOk(checkpoint->CreateCheckpoint(cfg.checkpoint_path, 0, &checkpoint_seq),
	    "checkpoint-create"))
		return 1;

	Phase(cfg, "checkpoint open");
	if (CheckOk(rocksdb::DB::Open(options, cfg.checkpoint_path, &checkpoint_db),
	    "checkpoint-open"))
		return 1;

	Phase(cfg, "checkpoint read");
	if (CheckOk(checkpoint_db->Get(rocksdb::ReadOptions(), "checkpoint-key", &value),
	    "checkpoint-read"))
		return 1;

	if (value != "checkpoint-value") {
		uk_pr_err("rocksdb: checkpoint mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature checkpoint passed (seq=%llu)\n",
		   static_cast<unsigned long long>(checkpoint_seq));
	return 0;
}

} /* namespace approcksdb */