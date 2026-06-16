#include "approcksdb/features.hpp"

namespace approcksdb {

int RunSecondaryIndexFeature(const AppConfig& cfg, const rocksdb::Options& options)
{
	std::string db_path = DerivedPath(cfg.db_path, "-secondary-index");
	rocksdb::TransactionDBOptions txn_db_options;
	rocksdb::TransactionDB* raw_txn_db = nullptr;
	std::unique_ptr<rocksdb::TransactionDB> txn_db;
	std::unique_ptr<rocksdb::Transaction> txn;
	std::unique_ptr<rocksdb::ColumnFamilyHandle> primary_cf;
	std::unique_ptr<rocksdb::ColumnFamilyHandle> secondary_cf;
	std::shared_ptr<rocksdb::SimpleSecondaryIndex> index;
	std::unique_ptr<rocksdb::Iterator> raw_index_it;
	std::unique_ptr<rocksdb::SecondaryIndexIterator> index_it;
	rocksdb::Status status;
	std::string value;

	Phase(cfg, "secondary index setup");
	MaybeDestroy(cfg, options, db_path.c_str());
	index = std::make_shared<rocksdb::SimpleSecondaryIndex>(
		rocksdb::kDefaultWideColumnName.ToString());
	txn_db_options.secondary_indices.emplace_back(index);

	if (CheckOk(rocksdb::TransactionDB::Open(options, txn_db_options,
				      db_path, &raw_txn_db),
	    "secondary-index-open"))
		return 1;
	txn_db.reset(raw_txn_db);

	{
		rocksdb::ColumnFamilyHandle* raw_primary_cf = nullptr;
		rocksdb::ColumnFamilyHandle* raw_secondary_cf = nullptr;

		if (CheckOk(txn_db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(),
					       "primary", &raw_primary_cf),
		    "secondary-index-create-primary-cf"))
			return 1;
		if (CheckOk(txn_db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(),
					       "secondary", &raw_secondary_cf),
		    "secondary-index-create-secondary-cf"))
			return 1;

		primary_cf.reset(raw_primary_cf);
		secondary_cf.reset(raw_secondary_cf);
	}

	index->SetPrimaryColumnFamily(primary_cf.get());
	index->SetSecondaryColumnFamily(secondary_cf.get());

	Phase(cfg, "secondary index populate");
	txn.reset(txn_db->BeginTransaction(rocksdb::WriteOptions()));
	if (!txn) {
		uk_pr_err("rocksdb: failed to begin secondary-index transaction\n");
		return 1;
	}
	if (CheckOk(txn->Put(primary_cf.get(), "user-1", "team-a"),
	    "secondary-index-put-user-1"))
		return 1;
	if (CheckOk(txn->Put(primary_cf.get(), "user-2", "team-a"),
	    "secondary-index-put-user-2"))
		return 1;
	if (CheckOk(txn->Put(primary_cf.get(), "user-3", "team-b"),
	    "secondary-index-put-user-3"))
		return 1;
	if (CheckOk(txn->Commit(), "secondary-index-commit"))
		return 1;
	txn.reset();

	Phase(cfg, "secondary index query");
	raw_index_it.reset(txn_db->NewIterator(rocksdb::ReadOptions(), secondary_cf.get()));
	index_it.reset(new rocksdb::SecondaryIndexIterator(index.get(),
					      std::move(raw_index_it)));
	index_it->Seek("team-a");
	if (!index_it->Valid()) {
		uk_pr_err("rocksdb: secondary index query for team-a returned no rows\n");
		return 1;
	}
	if (!index_it->status().ok()) {
		uk_pr_err("rocksdb: secondary index iterator failed: %s\n",
			  index_it->status().ToString().c_str());
		return 1;
	}
	if (index_it->key().ToString() != "user-1") {
		uk_pr_err("rocksdb: secondary index first key mismatch: got '%s'\n",
			  index_it->key().ToString().c_str());
		return 1;
	}
	index_it->Next();
	if (!index_it->Valid() || index_it->key().ToString() != "user-2") {
		uk_pr_err("rocksdb: secondary index second key mismatch\n");
		return 1;
	}
	index_it->Next();
	if (index_it->Valid()) {
		uk_pr_err("rocksdb: secondary index returned unexpected extra rows for team-a\n");
		return 1;
	}

	Phase(cfg, "secondary index update");
	if (CheckOk(txn_db->Put(rocksdb::WriteOptions(), primary_cf.get(),
			    "user-2", "team-b"),
	    "secondary-index-move-user-2"))
		return 1;
	if (CheckOk(txn_db->Delete(rocksdb::WriteOptions(), primary_cf.get(), "user-1"),
	    "secondary-index-delete-user-1"))
		return 1;

	raw_index_it.reset(txn_db->NewIterator(rocksdb::ReadOptions(), secondary_cf.get()));
	index_it.reset(new rocksdb::SecondaryIndexIterator(index.get(),
					      std::move(raw_index_it)));
	index_it->Seek("team-a");
	if (index_it->Valid()) {
		uk_pr_err("rocksdb: secondary index still returns team-a rows after update/delete\n");
		return 1;
	}

	raw_index_it.reset(txn_db->NewIterator(rocksdb::ReadOptions(), secondary_cf.get()));
	index_it.reset(new rocksdb::SecondaryIndexIterator(index.get(),
					      std::move(raw_index_it)));
	index_it->Seek("team-b");
	if (!index_it->Valid() || index_it->key().ToString() != "user-2") {
		uk_pr_err("rocksdb: secondary index missing moved user-2 entry\n");
		return 1;
	}
	index_it->Next();
	if (!index_it->Valid() || index_it->key().ToString() != "user-3") {
		uk_pr_err("rocksdb: secondary index missing original user-3 entry\n");
		return 1;
	}

	status = txn_db->Get(rocksdb::ReadOptions(), primary_cf.get(), "user-2", &value);
	if (CheckOk(status, "secondary-index-read-user-2"))
		return 1;
	if (value != "team-b") {
		uk_pr_err("rocksdb: primary row mismatch after secondary index update: got '%s'\n",
			  value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature secondary-index passed\n");
	return 0;
}

} /* namespace approcksdb */