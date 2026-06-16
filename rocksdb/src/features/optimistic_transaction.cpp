#include "approcksdb/features.hpp"

namespace approcksdb {

int RunOptimisticTransactionFeature(const AppConfig& cfg)
{
	rocksdb::Options options = MakeOptions();
	std::string db_path = DerivedPath(cfg.db_path, "-optimistic");
	rocksdb::OptimisticTransactionDB* raw_txn_db = nullptr;
	std::unique_ptr<rocksdb::OptimisticTransactionDB> txn_db;
	std::unique_ptr<rocksdb::Transaction> txn;
	rocksdb::WriteOptions write_options;
	rocksdb::ReadOptions read_options;
	rocksdb::OptimisticTransactionOptions txn_options;
	rocksdb::Status status;
	std::string value;
	rocksdb::DB* base_db = nullptr;

	Phase(cfg, "optimistic transaction setup");
	MaybeDestroy(cfg, options, db_path.c_str());
	if (CheckOk(rocksdb::OptimisticTransactionDB::Open(options, db_path, &raw_txn_db),
	    "optimistic-open"))
		return 1;
	txn_db.reset(raw_txn_db);
	base_db = txn_db->GetBaseDB();

	Phase(cfg, "optimistic transaction success");
	txn.reset(txn_db->BeginTransaction(write_options));
	if (!txn) {
		uk_pr_err("rocksdb: optimistic transaction begin failed\n");
		return 1;
	}
	if (CheckOk(txn->Put("opt-ok", "committed"), "optimistic-put-ok"))
		return 1;
	if (CheckOk(txn->Commit(), "optimistic-commit-ok"))
		return 1;
	txn.reset();
	if (CheckOk(base_db->Get(read_options, "opt-ok", &value), "optimistic-get-ok"))
		return 1;
	if (value != "committed") {
		uk_pr_err("rocksdb: optimistic committed mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	Phase(cfg, "optimistic transaction conflict");
	txn.reset(txn_db->BeginTransaction(write_options));
	if (!txn) {
		uk_pr_err("rocksdb: optimistic conflict transaction begin failed\n");
		return 1;
	}
	if (CheckOk(txn->Put("opt-key", "txn-value"), "optimistic-put-conflict"))
		return 1;
	if (CheckOk(base_db->Put(write_options, "opt-key", "outside"),
	    "optimistic-outside-put"))
		return 1;
	status = txn->Commit();
	if (!status.IsBusy()) {
		uk_pr_err("rocksdb: expected optimistic commit conflict, got: %s\n",
			  status.ToString().c_str());
		return 1;
	}
	txn.reset();

	if (CheckOk(base_db->Get(read_options, "opt-key", &value),
	    "optimistic-get-conflict-result"))
		return 1;
	if (value != "outside") {
		uk_pr_err("rocksdb: optimistic conflict result mismatch: got '%s'\n",
			  value.c_str());
		return 1;
	}

	Phase(cfg, "optimistic transaction snapshot");
	if (CheckOk(base_db->Put(write_options, "opt-snap", "before"),
	    "optimistic-put-snapshot-base"))
		return 1;
	txn_options.set_snapshot = true;
	txn.reset(txn_db->BeginTransaction(write_options, txn_options));
	if (!txn) {
		uk_pr_err("rocksdb: optimistic snapshot transaction begin failed\n");
		return 1;
	}
	const rocksdb::Snapshot* snapshot = txn->GetSnapshot();
	if (!snapshot) {
		uk_pr_err("rocksdb: optimistic snapshot missing\n");
		return 1;
	}
	if (CheckOk(base_db->Put(write_options, "opt-snap", "after"),
	    "optimistic-put-snapshot-outside"))
		return 1;
	read_options.snapshot = snapshot;
	status = txn->GetForUpdate(read_options, "opt-snap", &value);
	if (CheckOk(status, "optimistic-snapshot-read"))
		return 1;
	if (value != "before") {
		uk_pr_err("rocksdb: optimistic snapshot value mismatch: got '%s'\n",
			  value.c_str());
		return 1;
	}
	read_options.snapshot = nullptr;
	status = txn->Commit();
	if (!status.IsBusy()) {
		uk_pr_err("rocksdb: expected optimistic snapshot conflict, got: %s\n",
			  status.ToString().c_str());
		return 1;
	}
	txn.reset();

	if (CheckOk(base_db->Get(read_options, "opt-snap", &value),
	    "optimistic-get-snapshot-result"))
		return 1;
	if (value != "after") {
		uk_pr_err("rocksdb: optimistic snapshot final mismatch: got '%s'\n",
			  value.c_str());
		return 1;
	}

	txn_db.reset();
	MaybeDestroy(cfg, options, db_path.c_str());
	uk_pr_info("rocksdb: feature optimistic-transaction passed\n");
	return 0;
}

} /* namespace approcksdb */