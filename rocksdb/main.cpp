#include <uk/print.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <rocksdb/cache.h>
#include <rocksdb/db.h>
#include <rocksdb/env.h>
#include <rocksdb/file_system.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/table.h>
#include <rocksdb/utilities/backup_engine.h>
#include <rocksdb/utilities/cache_dump_load.h>
#include <rocksdb/utilities/checkpoint.h>
#include <rocksdb/utilities/secondary_index.h>
#include <rocksdb/utilities/secondary_index_simple.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/write_batch.h>

enum Feature : unsigned long {
	FEATURE_BASIC = 1UL << 0,
	FEATURE_BATCH = 1UL << 1,
	FEATURE_FLUSH = 1UL << 2,
	FEATURE_SNAPSHOT = 1UL << 3,
	FEATURE_ITERATOR = 1UL << 4,
	FEATURE_DELETE = 1UL << 5,
	FEATURE_REOPEN = 1UL << 6,
	FEATURE_CHECKPOINT = 1UL << 7,
	FEATURE_TRANSACTION = 1UL << 8,
	FEATURE_BACKUP = 1UL << 9,
	FEATURE_CACHE_DUMP_LOAD = 1UL << 10,
	FEATURE_SECONDARY_INDEX = 1UL << 11,
};

static constexpr unsigned long kDefaultFeatures =
	FEATURE_BASIC |
	FEATURE_BATCH |
	FEATURE_FLUSH |
	FEATURE_SNAPSHOT |
	FEATURE_ITERATOR |
	FEATURE_DELETE |
	FEATURE_REOPEN |
	FEATURE_CHECKPOINT |
	FEATURE_TRANSACTION |
	FEATURE_BACKUP |
	FEATURE_CACHE_DUMP_LOAD |
	FEATURE_SECONDARY_INDEX;

struct AppConfig {
	const char* db_path = "/rocksdb-demo";
	const char* checkpoint_path = "/rocksdb-demo-checkpoint";
	const char* storage = "ramfs";
	unsigned long features = kDefaultFeatures;
	bool clean_before = true;
	bool phase_logs = true;
	bool features_explicit = false;
};

static const char* status_code_name(const rocksdb::Status& st)
{
	switch (st.code()) {
	case rocksdb::Status::kOk:
		return "ok";
	case rocksdb::Status::kNotFound:
		return "not-found";
	case rocksdb::Status::kCorruption:
		return "corruption";
	case rocksdb::Status::kNotSupported:
		return "not-supported";
	case rocksdb::Status::kInvalidArgument:
		return "invalid-arg";
	case rocksdb::Status::kIOError:
		return "io-error";
	case rocksdb::Status::kMergeInProgress:
		return "merge-progress";
	case rocksdb::Status::kIncomplete:
		return "incomplete";
	case rocksdb::Status::kShutdownInProgress:
		return "shutdown";
	case rocksdb::Status::kTimedOut:
		return "timed-out";
	case rocksdb::Status::kAborted:
		return "aborted";
	case rocksdb::Status::kBusy:
		return "busy";
	case rocksdb::Status::kExpired:
		return "expired";
	case rocksdb::Status::kTryAgain:
		return "try-again";
	case rocksdb::Status::kCompactionTooLarge:
		return "compaction-large";
	case rocksdb::Status::kColumnFamilyDropped:
		return "cf-dropped";
	case rocksdb::Status::kMaxCode:
		return "max-code";
	}

	return "unknown";
}

static int check_ok(const rocksdb::Status& st, const char* step)
{
	if (!st.ok()) {
		uk_pr_err("rocksdb: fail step=%s code=%s sub=%u sev=%u\n",
			  step, status_code_name(st), static_cast<unsigned>(st.subcode()),
			  static_cast<unsigned>(st.severity()));
		uk_pr_err("rocksdb: msg=%s\n", st.ToString().c_str());
		return 1;
	}
	return 0;
}

static int check_io_ok(const rocksdb::IOStatus& st, const char* step)
{
	if (!st.ok()) {
		uk_pr_err("rocksdb: fail step=%s io=%s\n", step, st.ToString().c_str());
		return 1;
	}
	return 0;
}

static bool feature_enabled(const AppConfig& cfg, unsigned long feature)
{
	return (cfg.features & feature) != 0;
}

static const char* feature_name(unsigned long feature)
{
	switch (feature) {
	case FEATURE_BASIC:
		return "basic";
	case FEATURE_BATCH:
		return "batch";
	case FEATURE_FLUSH:
		return "flush";
	case FEATURE_SNAPSHOT:
		return "snapshot";
	case FEATURE_ITERATOR:
		return "iterator";
	case FEATURE_DELETE:
		return "delete";
	case FEATURE_REOPEN:
		return "reopen";
	case FEATURE_CHECKPOINT:
		return "checkpoint";
	case FEATURE_TRANSACTION:
		return "transaction";
	case FEATURE_BACKUP:
		return "backup";
	case FEATURE_CACHE_DUMP_LOAD:
		return "cache-dump-load";
	case FEATURE_SECONDARY_INDEX:
		return "secondary-index";
	}

	return "unknown";
}

static bool parse_feature(const char* value, unsigned long* feature)
{
	if (!std::strcmp(value, "basic")) {
		*feature = FEATURE_BASIC;
		return true;
	}
	if (!std::strcmp(value, "batch")) {
		*feature = FEATURE_BATCH;
		return true;
	}
	if (!std::strcmp(value, "flush")) {
		*feature = FEATURE_FLUSH;
		return true;
	}
	if (!std::strcmp(value, "snapshot")) {
		*feature = FEATURE_SNAPSHOT;
		return true;
	}
	if (!std::strcmp(value, "iterator")) {
		*feature = FEATURE_ITERATOR;
		return true;
	}
	if (!std::strcmp(value, "delete")) {
		*feature = FEATURE_DELETE;
		return true;
	}
	if (!std::strcmp(value, "reopen")) {
		*feature = FEATURE_REOPEN;
		return true;
	}
	if (!std::strcmp(value, "checkpoint")) {
		*feature = FEATURE_CHECKPOINT;
		return true;
	}
	if (!std::strcmp(value, "transaction")) {
		*feature = FEATURE_TRANSACTION;
		return true;
	}
	if (!std::strcmp(value, "backup")) {
		*feature = FEATURE_BACKUP;
		return true;
	}
	if (!std::strcmp(value, "cache-dump-load")) {
		*feature = FEATURE_CACHE_DUMP_LOAD;
		return true;
	}
	if (!std::strcmp(value, "secondary-index")) {
		*feature = FEATURE_SECONDARY_INDEX;
		return true;
	}

	return false;
}

static void print_usage(void)
{
	uk_pr_info("rocksdb usage:\n");
	uk_pr_info("  --db=<path>          DB directory path (default /rocksdb-demo)\n");
	uk_pr_info("  --checkpoint=<path>  checkpoint directory (default /rocksdb-demo-checkpoint)\n");
	uk_pr_info("  --feature=<name>     debug one feature: basic | batch | flush | snapshot | iterator | delete | reopen | checkpoint | transaction | backup | cache-dump-load | secondary-index\n");
	uk_pr_info("                       repeat --feature to run multiple slices; default is all\n");
	uk_pr_info("  --storage=<name>     label only: ramfs | 9p | blk | initrd\n");
	uk_pr_info("  --no-clean           skip DestroyDB() before the run\n");
	uk_pr_info("  --no-phase-logs      reduce phase logs\n");
	uk_pr_info("  --help               show this message\n");
}

static int parse_args(int argc, char* argv[], AppConfig* cfg)
{
	for (int i = 1; i < argc; i++) {
		if (!std::strcmp(argv[i], "--"))
			continue;
		if (!std::strcmp(argv[i], "--help")) {
			print_usage();
			return 1;
		}
		if (!std::strncmp(argv[i], "--db=", 5)) {
			cfg->db_path = argv[i] + 5;
			continue;
		}
		if (!std::strncmp(argv[i], "--checkpoint=", 13)) {
			cfg->checkpoint_path = argv[i] + 13;
			continue;
		}
		if (!std::strncmp(argv[i], "--feature=", 10)) {
			unsigned long feature = 0;

			if (!parse_feature(argv[i] + 10, &feature)) {
				uk_pr_err("rocksdb: invalid feature: %s\n", argv[i] + 10);
				return -1;
			}

			if (!cfg->features_explicit) {
				cfg->features = 0;
				cfg->features_explicit = true;
			}

			cfg->features |= feature;
			continue;
		}
		if (!std::strncmp(argv[i], "--storage=", 10)) {
			cfg->storage = argv[i] + 10;
			continue;
		}
		if (!std::strcmp(argv[i], "--no-clean")) {
			cfg->clean_before = false;
			continue;
		}
		if (!std::strcmp(argv[i], "--no-phase-logs")) {
			cfg->phase_logs = false;
			continue;
		}
		if (i == 1 && argv[i][0] != '-') {
			cfg->db_path = argv[i];
			continue;
		}

		uk_pr_err("rocksdb: unknown argument: %s\n", argv[i]);
		print_usage();
		return -1;
	}

	return 0;
}

static void phase(const AppConfig& cfg, const char* name)
{
	if (cfg.phase_logs)
		uk_pr_info("rocksdb: phase: %s\n", name);
}

static rocksdb::Options make_options(void)
{
	rocksdb::Options options;

	options.create_if_missing = true;
	options.error_if_exists = false;
	options.paranoid_checks = true;
	options.compression = rocksdb::kNoCompression;
	options.allow_mmap_reads = false;
	options.allow_mmap_writes = false;
	options.enable_thread_tracking = false;
	options.max_background_jobs = 1;
	options.max_subcompactions = 1;
	options.max_background_compactions = 1;
	options.max_background_flushes = 1;
	options.disable_auto_compactions = true;
	options.enable_pipelined_write = false;
	options.allow_concurrent_memtable_write = false;
	options.avoid_unnecessary_blocking_io = false;
	options.bytes_per_sync = 0;
	options.wal_bytes_per_sync = 0;

	return options;
}

static rocksdb::Env* get_env(const rocksdb::Options& options)
{
	return options.env ? options.env : rocksdb::Env::Default();
}

static std::string derived_path(const char* base, const char* suffix)
{
	return std::string(base) + suffix;
}

static void maybe_delete_file(const AppConfig& cfg, const rocksdb::Options& options,
			      const char* path)
{
	rocksdb::Status status;

	if (!cfg.clean_before)
		return;

	std::remove(path);
	status = get_env(options)->DeleteFile(path);
	if (!status.ok() && !status.IsNotFound())
		uk_pr_warn("rocksdb: file cleanup skipped for %s: %s\n",
			   path, status.ToString().c_str());
}

static int maybe_destroy(const AppConfig& cfg, const rocksdb::Options& options,
			 const char* db_path)
{
	if (!cfg.clean_before)
		return 0;

	rocksdb::Status status = rocksdb::DestroyDB(db_path, options);
	if (!status.ok())
		uk_pr_warn("rocksdb: initial cleanup skipped: %s\n", status.ToString().c_str());

	return 0;
}

static int run_basic_feature(const AppConfig& cfg, rocksdb::DB* db)
{
	std::string value;

	phase(cfg, "basic");
	if (check_ok(db->Put(rocksdb::WriteOptions(), "hello", "unikraft"),
		     "basic-put-hello"))
		return 1;

	if (check_ok(db->Get(rocksdb::ReadOptions(), "hello", &value),
		     "basic-get-hello"))
		return 1;

	if (value != "unikraft") {
		uk_pr_err("rocksdb: value mismatch for hello: got '%s'\n", value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature basic passed\n");
	return 0;
}

static int run_batch_feature(const AppConfig& cfg, rocksdb::DB* db)
{
	rocksdb::WriteBatch batch;
	std::string value;

	phase(cfg, "batch");
	batch.Put("k1", "v1");
	batch.Put("k2", "v2");
	batch.Delete("k1");

	if (check_ok(db->Write(rocksdb::WriteOptions(), &batch), "batch-write"))
		return 1;

	if (!db->Get(rocksdb::ReadOptions(), "k1", &value).IsNotFound()) {
		uk_pr_err("rocksdb: expected k1 deleted after write batch\n");
		return 1;
	}

	if (check_ok(db->Get(rocksdb::ReadOptions(), "k2", &value), "batch-get-k2"))
		return 1;

	if (value != "v2") {
		uk_pr_err("rocksdb: value mismatch for k2: got '%s'\n", value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature batch passed\n");
	return 0;
}

static int run_flush_feature(const AppConfig& cfg, rocksdb::DB* db)
{
	rocksdb::FlushOptions fopts;

	phase(cfg, "flush");
	if (check_ok(db->Put(rocksdb::WriteOptions(), "flush-key", "flush-value"),
		     "flush-put-key"))
		return 1;

	fopts.wait = true;
	if (check_ok(db->Flush(fopts), "flush-db"))
		return 1;

	uk_pr_info("rocksdb: feature flush passed\n");
	return 0;
}

static int run_snapshot_feature(const AppConfig& cfg, rocksdb::DB* db)
{
	const rocksdb::Snapshot* snap;
	rocksdb::ReadOptions ro;
	std::string value;
	rocksdb::Status status;

	phase(cfg, "snapshot");
	snap = db->GetSnapshot();
	if (!snap) {
		uk_pr_err("rocksdb: failed to create snapshot\n");
		return 1;
	}

	status = db->Put(rocksdb::WriteOptions(), "snapkey", "new");
	if (check_ok(status, "snapshot-put-new")) {
		db->ReleaseSnapshot(snap);
		return 1;
	}

	status = db->Put(rocksdb::WriteOptions(), "snapkey", "newer");
	if (check_ok(status, "snapshot-put-newer")) {
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
	if (check_ok(status, "snapshot-live-get")) {
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

static int run_iterator_feature(const AppConfig& cfg, rocksdb::DB* db)
{
	std::unique_ptr<rocksdb::Iterator> it;
	unsigned long keys_scanned = 0;

	phase(cfg, "iterator");
	if (check_ok(db->Put(rocksdb::WriteOptions(), "iter-a", "1"), "iterator-put-a"))
		return 1;
	if (check_ok(db->Put(rocksdb::WriteOptions(), "iter-b", "2"), "iterator-put-b"))
		return 1;

	it.reset(db->NewIterator(rocksdb::ReadOptions()));
	for (it->SeekToFirst(); it->Valid(); it->Next())
		keys_scanned++;

	if (!it->status().ok()) {
		uk_pr_err("rocksdb: iterator failed: %s\n", it->status().ToString().c_str());
		return 1;
	}
	if (keys_scanned == 0) {
		uk_pr_err("rocksdb: iterator saw no keys\n");
		return 1;
	}

	uk_pr_info("rocksdb: feature iterator passed (keys_scanned=%lu)\n", keys_scanned);
	return 0;
}

static int run_delete_feature(const AppConfig& cfg, rocksdb::DB* db)
{
	std::string value;
	rocksdb::Status status;

	phase(cfg, "delete");
	if (check_ok(db->Put(rocksdb::WriteOptions(), "delete-key", "value"),
		     "delete-put-key"))
		return 1;

	if (check_ok(db->Delete(rocksdb::WriteOptions(), "delete-key"),
		     "delete-key"))
		return 1;

	status = db->Get(rocksdb::ReadOptions(), "delete-key", &value);
	if (!status.IsNotFound()) {
		uk_pr_err("rocksdb: expected delete-key deleted, got: %s\n",
			  status.ToString().c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature delete passed\n");
	return 0;
}

static int run_reopen_feature(const AppConfig& cfg, const rocksdb::Options& options,
			      std::unique_ptr<rocksdb::DB>* db)
{
	std::string value;
	rocksdb::FlushOptions fopts;

	phase(cfg, "reopen");
	if (check_ok((*db)->Put(rocksdb::WriteOptions(), "reopen-key", "reopen-value"),
		     "reopen-put-key"))
		return 1;

	fopts.wait = true;
	if (check_ok((*db)->Flush(fopts), "reopen-flush"))
		return 1;

	db->reset();
	if (check_ok(rocksdb::DB::Open(options, cfg.db_path, db), "reopen-open"))
		return 1;

	if (check_ok((*db)->Get(rocksdb::ReadOptions(), "reopen-key", &value),
		     "reopen-get-key"))
		return 1;

	if (value != "reopen-value") {
		uk_pr_err("rocksdb: reopen mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature reopen passed\n");
	return 0;
}

static int run_checkpoint_feature(const AppConfig& cfg, rocksdb::DB* db,
			  const rocksdb::Options& options)
{
	rocksdb::Checkpoint* raw_checkpoint = nullptr;
	std::unique_ptr<rocksdb::Checkpoint> checkpoint;
	std::unique_ptr<rocksdb::DB> checkpoint_db;
	rocksdb::FlushOptions fopts;
	uint64_t checkpoint_seq = 0;
	std::string value;

	phase(cfg, "checkpoint setup");
	if (check_ok(db->Put(rocksdb::WriteOptions(), "checkpoint-key", "checkpoint-value"),
		     "checkpoint-put-key"))
		return 1;

	fopts.wait = true;
	if (check_ok(db->Flush(fopts), "checkpoint-flush"))
		return 1;

	phase(cfg, "checkpoint handle");
	if (check_ok(rocksdb::Checkpoint::Create(db, &raw_checkpoint),
		     "checkpoint-handle"))
		return 1;
	checkpoint.reset(raw_checkpoint);

	phase(cfg, "checkpoint create");
	if (check_ok(checkpoint->CreateCheckpoint(cfg.checkpoint_path, 0, &checkpoint_seq),
		     "checkpoint-create"))
		return 1;

	phase(cfg, "checkpoint open");
	if (check_ok(rocksdb::DB::Open(options, cfg.checkpoint_path, &checkpoint_db),
		     "checkpoint-open"))
		return 1;

	phase(cfg, "checkpoint read");
	if (check_ok(checkpoint_db->Get(rocksdb::ReadOptions(), "checkpoint-key", &value),
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

static int run_transaction_feature(const AppConfig& cfg,
				   const rocksdb::Options& options)
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

	phase(cfg, "transaction open");
	if (check_ok(rocksdb::TransactionDB::Open(options, txn_db_options,
					      cfg.db_path, &raw_txn_db),
		     "transaction-open"))
		return 1;
	 txn_db.reset(raw_txn_db);

	phase(cfg, "transaction commit path");
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
	if (check_ok(status, "transaction-put"))
		return 1;

	status = txn_db->Get(read_options, "txn-key", &value);
	if (!status.IsNotFound()) {
		uk_pr_err("rocksdb: txn-key became visible before commit: %s\n",
			  status.ToString().c_str());
		return 1;
	}

	phase(cfg, "transaction conflict path");
	status = txn_db->Put(write_options, "txn-key", "outside-write");
	if (status.subcode() != rocksdb::Status::kLockTimeout) {
		uk_pr_err("rocksdb: expected lock-timeout conflict, got: %s\n",
			  status.ToString().c_str());
		return 1;
	}

	status = txn->Commit();
	if (check_ok(status, "transaction-commit"))
		return 1;
	txn.reset();

	status = txn_db->Get(read_options, "txn-key", &value);
	if (check_ok(status, "transaction-get-after-commit"))
		return 1;
	if (value != "txn-value") {
		uk_pr_err("rocksdb: committed txn-key mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	phase(cfg, "transaction reopen path");
	txn_db.reset();
	raw_txn_db = nullptr;
	if (check_ok(rocksdb::TransactionDB::Open(options, txn_db_options,
					      cfg.db_path, &raw_txn_db),
		     "transaction-reopen"))
		return 1;
	txn_db.reset(raw_txn_db);

	status = txn_db->Get(read_options, "txn-key", &value);
	if (check_ok(status, "transaction-get-after-reopen"))
		return 1;
	if (value != "txn-value") {
		uk_pr_err("rocksdb: reopened txn-key mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	phase(cfg, "transaction snapshot conflict path");
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
	if (check_ok(status, "transaction-outside-put-snapshot"))
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
	if (check_ok(status, "transaction-snapshot-rollback"))
		return 1;
	txn.reset();
	txn_options.set_snapshot = false;

	phase(cfg, "transaction rollback path");
	txn.reset(txn_db->BeginTransaction(write_options, txn_options));
	if (!txn) {
		uk_pr_err("rocksdb: rollback transaction begin failed\n");
		return 1;
	}

	status = txn->Put("txn-savepoint-key", "savepoint-value");
	if (check_ok(status, "transaction-put-savepoint"))
		return 1;
	txn->SetSavePoint();
	status = txn->Put("txn-rollback-key", "rollback-value");
	if (check_ok(status, "transaction-put-rollback"))
		return 1;

	status = txn->RollbackToSavePoint();
	if (check_ok(status, "transaction-rollback-to-savepoint"))
		return 1;

	status = txn->Commit();
	if (check_ok(status, "transaction-commit-savepoint"))
		return 1;
	txn.reset();

	status = txn_db->Get(read_options, "txn-savepoint-key", &value);
	if (check_ok(status, "transaction-get-savepoint-key"))
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

	phase(cfg, "transaction write-prepared path");
	maybe_destroy(cfg, options, prepared_path.c_str());
	txn_db.reset();
	raw_txn_db = nullptr;
	txn_db_options.write_policy = rocksdb::TxnDBWritePolicy::WRITE_PREPARED;
	if (check_ok(rocksdb::TransactionDB::Open(options, txn_db_options,
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
	if (check_ok(status, "transaction-put-write-prepared"))
		return 1;
	status = txn->Commit();
	if (check_ok(status, "transaction-commit-write-prepared"))
		return 1;
	txn.reset();

	status = txn_db->Get(read_options, "txn-wp-key", &value);
	if (check_ok(status, "transaction-get-write-prepared"))
		return 1;
	if (value != "txn-wp-value") {
		uk_pr_err("rocksdb: write-prepared key mismatch: got '%s'\n", value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature transaction passed\n");
	return 0;
}

static int run_backup_feature(const AppConfig& cfg, rocksdb::DB* db,
			      const rocksdb::Options& options)
{
	std::string backup_path = derived_path(cfg.db_path, "-backup");
	std::string restore_path = derived_path(cfg.db_path, "-restore");
	rocksdb::BackupEngineOptions backup_options(backup_path);
	rocksdb::BackupEngineOptions backup_read_options(backup_path);
	std::unique_ptr<rocksdb::BackupEngine> backup_engine;
	std::unique_ptr<rocksdb::BackupEngineReadOnly> restore_engine;
	rocksdb::BackupEngine* raw_backup_engine = nullptr;
	rocksdb::BackupEngineReadOnly* raw_restore_engine = nullptr;
	std::unique_ptr<rocksdb::DB> restore_db;
	rocksdb::FlushOptions fopts;
	std::vector<rocksdb::BackupInfo> backup_info;
	std::string value;

	phase(cfg, "backup setup");
	backup_options.destroy_old_data = cfg.clean_before;
	maybe_destroy(cfg, options, restore_path.c_str());
	if (check_ok(get_env(options)->CreateDirIfMissing(backup_path),
		     "backup-create-dir"))
		return 1;

	if (check_ok(db->Put(rocksdb::WriteOptions(), "backup-key", "backup-value"),
		     "backup-put-key"))
		return 1;
	fopts.wait = true;
	if (check_ok(db->Flush(fopts), "backup-flush"))
		return 1;

	phase(cfg, "backup create");
	if (check_ok(rocksdb::BackupEngine::Open(backup_options, get_env(options),
					     &raw_backup_engine),
		     "backup-open"))
		return 1;
	backup_engine.reset(raw_backup_engine);

	if (check_io_ok(backup_engine->CreateNewBackup(db), "backup-create"))
		return 1;
	backup_engine->GetBackupInfo(&backup_info);
	if (backup_info.empty()) {
		uk_pr_err("rocksdb: backup metadata is empty after CreateNewBackup\n");
		return 1;
	}
	if (check_io_ok(backup_engine->VerifyBackup(backup_info.back().backup_id),
			"backup-verify"))
		return 1;

	if (check_ok(db->Put(rocksdb::WriteOptions(), "backup-late-key", "late-value"),
		     "backup-put-late-key"))
		return 1;

	phase(cfg, "backup restore");
	if (check_ok(rocksdb::BackupEngineReadOnly::Open(backup_read_options,
						     get_env(options),
						     &raw_restore_engine),
		     "backup-open-readonly"))
		return 1;
	restore_engine.reset(raw_restore_engine);

	if (check_io_ok(restore_engine->RestoreDBFromLatestBackup(restore_path,
							 restore_path),
			"backup-restore-latest"))
		return 1;

	if (check_ok(rocksdb::DB::Open(options, restore_path, &restore_db),
		     "backup-open-restored-db"))
		return 1;
	if (check_ok(restore_db->Get(rocksdb::ReadOptions(), "backup-key", &value),
		     "backup-read-restored-key"))
		return 1;
	if (value != "backup-value") {
		uk_pr_err("rocksdb: restored backup-key mismatch: got '%s'\n",
			  value.c_str());
		return 1;
	}

	if (!restore_db->Get(rocksdb::ReadOptions(), "backup-late-key", &value)
		     .IsNotFound()) {
		uk_pr_err("rocksdb: backup restore unexpectedly included post-backup write\n");
		return 1;
	}

	uk_pr_info("rocksdb: feature backup passed (backup_id=%u)\n",
		   backup_info.back().backup_id);
	return 0;
}

static int run_cache_dump_load_feature(const AppConfig& cfg)
{
	rocksdb::Options options = make_options();
	std::string db_path = derived_path(cfg.db_path, "-cache");
	std::string dump_path = derived_path(cfg.db_path, "-cache.dump");
	rocksdb::LRUCacheOptions block_cache_options;
	std::shared_ptr<rocksdb::Cache> block_cache;
	rocksdb::CompressedSecondaryCacheOptions secondary_cache_options(1 << 20,
								 -1, false, 0.5);
	std::shared_ptr<rocksdb::SecondaryCache> secondary_cache;
	rocksdb::BlockBasedTableOptions table_options;
	std::unique_ptr<rocksdb::DB> db;
	std::unique_ptr<rocksdb::CacheDumpWriter> writer;
	std::unique_ptr<rocksdb::CacheDumper> dumper;
	std::unique_ptr<rocksdb::CacheDumpReader> reader;
	std::unique_ptr<rocksdb::CacheDumpedLoader> loader;
	rocksdb::CacheDumpOptions dump_options{};
	rocksdb::FileOptions file_options(options);
	rocksdb::FlushOptions fopts;
	std::string value(8192, 'v');
	uint64_t dump_size = 0;

	phase(cfg, "cache dump/load setup");
	secondary_cache_options.compression_type = rocksdb::kNoCompression;
	secondary_cache = rocksdb::NewCompressedSecondaryCache(secondary_cache_options);
	if (!secondary_cache) {
		uk_pr_err("rocksdb: failed to create secondary cache for dump/load feature\n");
		return 1;
	}

	block_cache_options.capacity = 1 << 20;
	block_cache_options.secondary_cache = secondary_cache;
	block_cache = rocksdb::NewLRUCache(block_cache_options);
	if (!block_cache) {
		uk_pr_err("rocksdb: failed to create block cache for dump/load feature\n");
		return 1;
	}

	table_options.block_cache = block_cache;
	table_options.cache_index_and_filter_blocks = true;
	table_options.pin_l0_filter_and_index_blocks_in_cache = true;
	options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

	maybe_destroy(cfg, options, db_path.c_str());
	maybe_delete_file(cfg, options, dump_path.c_str());
	if (check_ok(rocksdb::DB::Open(options, db_path, &db), "cache-open"))
		return 1;

	if (check_ok(db->Put(rocksdb::WriteOptions(), "cache-key-a", value),
		     "cache-put-a"))
		return 1;
	if (check_ok(db->Put(rocksdb::WriteOptions(), "cache-key-b", value + "-b"),
		     "cache-put-b"))
		return 1;
	fopts.wait = true;
	if (check_ok(db->Flush(fopts), "cache-flush"))
		return 1;
	if (check_ok(db->Get(rocksdb::ReadOptions(), "cache-key-a", &value),
		     "cache-read-a"))
		return 1;

	dump_options.clock = get_env(options)->GetSystemClock().get();
	phase(cfg, "cache dump");
	if (check_io_ok(rocksdb::NewToFileCacheDumpWriter(get_env(options)->GetFileSystem(),
						 file_options, dump_path, &writer),
		     "cache-writer"))
		return 1;
	if (check_ok(rocksdb::NewDefaultCacheDumper(dump_options, block_cache,
					   std::move(writer), &dumper),
		     "cache-dumper"))
		return 1;
	if (check_ok(dumper->SetDumpFilter({db.get()}), "cache-dump-filter"))
		return 1;
	if (check_io_ok(dumper->DumpCacheEntriesToWriter(), "cache-dump-run"))
		return 1;
	if (check_ok(get_env(options)->GetFileSize(dump_path, &dump_size),
		     "cache-dump-size"))
		return 1;
	if (dump_size == 0) {
		uk_pr_err("rocksdb: cache dump file is empty\n");
		return 1;
	}

	phase(cfg, "cache load");
	if (check_io_ok(rocksdb::NewFromFileCacheDumpReader(get_env(options)->GetFileSystem(),
						  file_options, dump_path, &reader),
		     "cache-reader"))
		return 1;
	if (check_ok(rocksdb::NewDefaultCacheDumpedLoader(dump_options, table_options,
						 secondary_cache,
						 std::move(reader), &loader),
		     "cache-loader"))
		return 1;
	if (check_io_ok(loader->RestoreCacheEntriesToSecondaryCache(),
			"cache-load-run"))
		return 1;

	uk_pr_info("rocksdb: feature cache-dump-load passed (dump_size=%llu)\n",
		   static_cast<unsigned long long>(dump_size));
	return 0;
}

static int run_secondary_index_feature(const AppConfig& cfg,
			       const rocksdb::Options& options)
{
	std::string db_path = derived_path(cfg.db_path, "-secondary-index");
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

	phase(cfg, "secondary index setup");
	maybe_destroy(cfg, options, db_path.c_str());
	index = std::make_shared<rocksdb::SimpleSecondaryIndex>(
		rocksdb::kDefaultWideColumnName.ToString());
	txn_db_options.secondary_indices.emplace_back(index);

	if (check_ok(rocksdb::TransactionDB::Open(options, txn_db_options,
					      db_path, &raw_txn_db),
		     "secondary-index-open"))
		return 1;
	txn_db.reset(raw_txn_db);

	{
		rocksdb::ColumnFamilyHandle* raw_primary_cf = nullptr;
		rocksdb::ColumnFamilyHandle* raw_secondary_cf = nullptr;

		if (check_ok(txn_db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(),
					       "primary", &raw_primary_cf),
			     "secondary-index-create-primary-cf"))
			return 1;
		if (check_ok(txn_db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(),
					       "secondary", &raw_secondary_cf),
			     "secondary-index-create-secondary-cf"))
			return 1;

		primary_cf.reset(raw_primary_cf);
		secondary_cf.reset(raw_secondary_cf);
	}

	index->SetPrimaryColumnFamily(primary_cf.get());
	index->SetSecondaryColumnFamily(secondary_cf.get());

	phase(cfg, "secondary index populate");
	txn.reset(txn_db->BeginTransaction(rocksdb::WriteOptions()));
	if (!txn) {
		uk_pr_err("rocksdb: failed to begin secondary-index transaction\n");
		return 1;
	}
	if (check_ok(txn->Put(primary_cf.get(), "user-1", "team-a"),
		     "secondary-index-put-user-1"))
		return 1;
	if (check_ok(txn->Put(primary_cf.get(), "user-2", "team-a"),
		     "secondary-index-put-user-2"))
		return 1;
	if (check_ok(txn->Put(primary_cf.get(), "user-3", "team-b"),
		     "secondary-index-put-user-3"))
		return 1;
	if (check_ok(txn->Commit(), "secondary-index-commit"))
		return 1;
	txn.reset();

	phase(cfg, "secondary index query");
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

	phase(cfg, "secondary index update");
	if (check_ok(txn_db->Put(rocksdb::WriteOptions(), primary_cf.get(),
			    "user-2", "team-b"),
		     "secondary-index-move-user-2"))
		return 1;
	if (check_ok(txn_db->Delete(rocksdb::WriteOptions(), primary_cf.get(), "user-1"),
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
	if (check_ok(status, "secondary-index-read-user-2"))
		return 1;
	if (value != "team-b") {
		uk_pr_err("rocksdb: primary row mismatch after secondary index update: got '%s'\n",
			  value.c_str());
		return 1;
	}

	uk_pr_info("rocksdb: feature secondary-index passed\n");
	return 0;
}

int main(int argc, char** argv)
{
	AppConfig cfg;
	rocksdb::Options options;
	std::unique_ptr<rocksdb::DB> db;
	int parsed;

	parsed = parse_args(argc, argv, &cfg);
	if (parsed > 0)
		return 0;
	if (parsed < 0)
		return 1;

	uk_pr_info("rocksdb: db=%s checkpoint=%s storage=%s clean=%s\n",
		   cfg.db_path, cfg.checkpoint_path, cfg.storage,
		   cfg.clean_before ? "yes" : "no");

	options = make_options();
	maybe_destroy(cfg, options, cfg.db_path);
	if (feature_enabled(cfg, FEATURE_CHECKPOINT))
		maybe_destroy(cfg, options, cfg.checkpoint_path);

	phase(cfg, "open");
	if (check_ok(rocksdb::DB::Open(options, cfg.db_path, &db), "open"))
		return 1;

	if (feature_enabled(cfg, FEATURE_BASIC) && run_basic_feature(cfg, db.get()))
		return 1;
	if (feature_enabled(cfg, FEATURE_BATCH) && run_batch_feature(cfg, db.get()))
		return 1;
	if (feature_enabled(cfg, FEATURE_FLUSH) && run_flush_feature(cfg, db.get()))
		return 1;
	if (feature_enabled(cfg, FEATURE_SNAPSHOT) && run_snapshot_feature(cfg, db.get()))
		return 1;
	if (feature_enabled(cfg, FEATURE_ITERATOR) && run_iterator_feature(cfg, db.get()))
		return 1;
	if (feature_enabled(cfg, FEATURE_DELETE) && run_delete_feature(cfg, db.get()))
		return 1;
	if (feature_enabled(cfg, FEATURE_REOPEN) && run_reopen_feature(cfg, options, &db))
		return 1;
	if (feature_enabled(cfg, FEATURE_CHECKPOINT) && run_checkpoint_feature(cfg, db.get(), options))
		return 1;
	if (feature_enabled(cfg, FEATURE_BACKUP) && run_backup_feature(cfg, db.get(), options))
		return 1;
	if (feature_enabled(cfg, FEATURE_CACHE_DUMP_LOAD) && run_cache_dump_load_feature(cfg))
		return 1;
	if (feature_enabled(cfg, FEATURE_TRANSACTION)) {
		db.reset();
		if (run_transaction_feature(cfg, options))
			return 1;
	}
	if (feature_enabled(cfg, FEATURE_SECONDARY_INDEX) &&
	    run_secondary_index_feature(cfg, options))
		return 1;

	for (unsigned long feature = FEATURE_BASIC; feature <= FEATURE_SECONDARY_INDEX;
	     feature <<= 1) {
		if (feature_enabled(cfg, feature))
			uk_pr_info("rocksdb: completed feature %s\n", feature_name(feature));
	}

	uk_pr_info("rocksdb: feature debug run passed\n");
	return 0;
}
