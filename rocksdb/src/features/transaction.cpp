#include "approcksdb/features.hpp"

namespace approcksdb {

int RunTransactionFeature(const AppConfig& cfg, const rocksdb::Options& options)
{
	rocksdb::TransactionDBOptions txn_db_options;
	rocksdb::WriteOptions write_options;
	rocksdb::ReadOptions read_options;
	rocksdb::TransactionOptions txn_options;
	rocksdb::TransactionDB* raw_txn_db = nullptr;
	std::unique_ptr<rocksdb::TransactionDB> txn_db;
	std::unique_ptr<rocksdb::Transaction> txn;
	rocksdb::Status status;
	std::string value;
	const rocksdb::Snapshot* snapshot = nullptr;
	std::string prepared_path = std::string(cfg.db_path) + "-wp";

	Phase(cfg, "transaction open");
	if (CheckOk(rocksdb::TransactionDB::Open(options, txn_db_options,
				      cfg.db_path, &raw_txn_db),
	    "transaction-open"))
		return 1;
	txn_db.reset(raw_txn_db);

	Phase(cfg, "transaction commit path");
	txn.reset(txn_db->BeginTransaction(write_options, txn_options));
	if (!txn) {
		uk_pr_err("rocksdb: transaction begin failed\n");
		return 1;
	}

	status = txn->Get(read_options, "txn-key", &value);
	if (!status.IsNotFound()) {
		uk_pr_err("rocksdb: expected txn-key missing before write, got: %s\n",
			  status.ToString().c_str());
		return 1;
	}

	status = txn->Put("txn-key", "txn-value");
	if (CheckOk(status, "transaction-put"))
		return 1;

	status = txn_db->Get(read_options, "txn-key", &value);
	if (!status.IsNotFound()) {
		uk_pr_err("rocksdb: txn-key became visible before commit: %s\n",
			  status.ToString().c_str());
		return 1;
	}

	Phase(cfg, "transaction conflict path");
	status = txn_db->Put(write_options, "txn-key", "outside-write");
	if (status.subcode() != rocksdb::Status::kLockTimeout) {
		uk_pr_err("rocksdb: expected lock-timeout conflict, got: %s\n",
			  status.ToString().c_str());
		return 1;
	}

	status = txn->Commit();
	if (CheckOk(status, "transaction-commit"))
		return 1;
	txn.reset();

	status = txn_db->Get(read_options, "txn-key", &value);
	if (CheckOk(status, "transaction-get-after-commit"))
		return 1;
	if (value != "txn-value") {
		uk_pr_err("rocksdb: committed txn-key mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	Phase(cfg, "transaction reopen path");
	txn_db.reset();
	raw_txn_db = nullptr;
	if (CheckOk(rocksdb::TransactionDB::Open(options, txn_db_options,
				      cfg.db_path, &raw_txn_db),
	    "transaction-reopen"))
		return 1;
	txn_db.reset(raw_txn_db);

	status = txn_db->Get(read_options, "txn-key", &value);
	if (CheckOk(status, "transaction-get-after-reopen"))
		return 1;
	if (value != "txn-value") {
		uk_pr_err("rocksdb: reopened txn-key mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	Phase(cfg, "transaction snapshot conflict path");
	txn_options.set_snapshot = true;
	txn.reset(txn_db->BeginTransaction(write_options, txn_options));
	if (!txn) {
		uk_pr_err("rocksdb: snapshot transaction begin failed\n");
		return 1;
	}

	snapshot = txn->GetSnapshot();
	if (!snapshot) {
		uk_pr_err("rocksdb: transaction snapshot missing\n");
		return 1;
	}

	status = txn_db->Put(write_options, "txn-snapshot-key", "outside-snapshot");
	if (CheckOk(status, "transaction-outside-put-snapshot"))
		return 1;

	read_options.snapshot = snapshot;
	status = txn->GetForUpdate(read_options, "txn-snapshot-key", &value);
	if (!status.IsBusy()) {
		uk_pr_err("rocksdb: expected busy snapshot conflict, got: %s\n",
			  status.ToString().c_str());
		return 1;
	}
	read_options.snapshot = nullptr;

	status = txn->Rollback();
	if (CheckOk(status, "transaction-snapshot-rollback"))
		return 1;
	txn.reset();
	txn_options.set_snapshot = false;

	Phase(cfg, "transaction rollback path");
	txn.reset(txn_db->BeginTransaction(write_options, txn_options));
	if (!txn) {
		uk_pr_err("rocksdb: rollback transaction begin failed\n");
		return 1;
	}

	status = txn->Put("txn-savepoint-key", "savepoint-value");
	if (CheckOk(status, "transaction-put-savepoint"))
		return 1;
	txn->SetSavePoint();
	status = txn->Put("txn-rollback-key", "rollback-value");
	if (CheckOk(status, "transaction-put-rollback"))
		return 1;

	status = txn->RollbackToSavePoint();
	if (CheckOk(status, "transaction-rollback-to-savepoint"))
		return 1;

	status = txn->Commit();
	if (CheckOk(status, "transaction-commit-savepoint"))
		return 1;
	txn.reset();

	status = txn_db->Get(read_options, "txn-savepoint-key", &value);
	if (CheckOk(status, "transaction-get-savepoint-key"))
		return 1;
	if (value != "savepoint-value") {
		uk_pr_err("rocksdb: savepoint key mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	status = txn_db->Get(read_options, "txn-rollback-key", &value);
	if (!status.IsNotFound()) {
		uk_pr_err("rocksdb: rollback key visible after savepoint rollback: %s\n",
			  status.ToString().c_str());
		return 1;
	}

	Phase(cfg, "transaction write-prepared path");
	MaybeDestroy(cfg, options, prepared_path.c_str());
	txn_db.reset();
	raw_txn_db = nullptr;
	txn_db_options.write_policy = rocksdb::TxnDBWritePolicy::WRITE_PREPARED;
	if (CheckOk(rocksdb::TransactionDB::Open(options, txn_db_options,
				      prepared_path, &raw_txn_db),
	    "transaction-open-write-prepared"))
		return 1;
	txn_db.reset(raw_txn_db);

	txn.reset(txn_db->BeginTransaction(write_options, rocksdb::TransactionOptions()));
	if (!txn) {
		uk_pr_err("rocksdb: write-prepared transaction begin failed\n");
		return 1;
	}

	status = txn->Put("txn-wp-key", "txn-wp-value");
	if (CheckOk(status, "transaction-put-write-prepared"))
		return 1;
	status = txn->Commit();
	if (CheckOk(status, "transaction-commit-write-prepared"))
		return 1;
	txn.reset();

	status = txn_db->Get(read_options, "txn-wp-key", &value);
	if (CheckOk(status, "transaction-get-write-prepared"))
		return 1;
	if (value != "txn-wp-value") {
		uk_pr_err("rocksdb: write-prepared key mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature transaction passed\n");
	return 0;
}

} /* namespace approcksdb */