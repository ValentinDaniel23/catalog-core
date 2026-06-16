#include "approcksdb/features.hpp"

namespace approcksdb {

int RunSnapshotFeature(const AppConfig& cfg, rocksdb::DB* db)
{
	const rocksdb::Snapshot* snap;
	rocksdb::ReadOptions ro;
	std::string value;
	rocksdb::Status status;

	Phase(cfg, "snapshot");
	snap = db->GetSnapshot();
	if (!snap) {
		uk_pr_err("rocksdb: failed to create snapshot\n");
		return 1;
	}

	status = db->Put(rocksdb::WriteOptions(), "snapkey", "new");
	if (CheckOk(status, "snapshot-put-new")) {
		db->ReleaseSnapshot(snap);
		return 1;
	}

	status = db->Put(rocksdb::WriteOptions(), "snapkey", "newer");
	if (CheckOk(status, "snapshot-put-newer")) {
		db->ReleaseSnapshot(snap);
		return 1;
	}

	ro.snapshot = snap;
	status = db->Get(ro, "snapkey", &value);
	if (!status.IsNotFound()) {
		uk_pr_err("rocksdb: expected snapkey absent in snapshot, got: %s\n",
			  status.ToString().c_str());
		db->ReleaseSnapshot(snap);
		return 1;
	}

	status = db->Get(rocksdb::ReadOptions(), "snapkey", &value);
	if (CheckOk(status, "snapshot-live-get")) {
		db->ReleaseSnapshot(snap);
		return 1;
	}

	if (value != "newer") {
		uk_pr_err("rocksdb: live mismatch for snapkey: got '%s'\n", value.c_str());
		db->ReleaseSnapshot(snap);
		return 1;
	}

	db->ReleaseSnapshot(snap);
	uk_pr_info("rocksdb: feature snapshot passed\n");
	return 0;
}

} /* namespace approcksdb */