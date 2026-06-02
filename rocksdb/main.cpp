#include <uk/print.h>

#include <cstring>
#include <memory>
#include <string>

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/checkpoint.h>
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
};

static constexpr unsigned long kDefaultFeatures =
	FEATURE_BASIC |
	FEATURE_BATCH |
	FEATURE_FLUSH |
	FEATURE_SNAPSHOT |
	FEATURE_ITERATOR |
	FEATURE_DELETE |
	FEATURE_REOPEN |
	FEATURE_CHECKPOINT;

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

	return false;
}

static void print_usage(void)
{
	uk_pr_info("rocksdb usage:\n");
	uk_pr_info("  --db=<path>          DB directory path (default /rocksdb-demo)\n");
	uk_pr_info("  --checkpoint=<path>  checkpoint directory (default /rocksdb-demo-checkpoint)\n");
	uk_pr_info("  --feature=<name>     debug one feature: basic | batch | flush | snapshot | iterator | delete | reopen | checkpoint\n");
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

	for (unsigned long feature = FEATURE_BASIC; feature <= FEATURE_CHECKPOINT; feature <<= 1) {
		if (feature_enabled(cfg, feature))
			uk_pr_info("rocksdb: completed feature %s\n", feature_name(feature));
	}

	uk_pr_info("rocksdb: feature debug run passed\n");
	return 0;
}
