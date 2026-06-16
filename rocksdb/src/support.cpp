#include "approcksdb/support.hpp"

namespace approcksdb {

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

int CheckOk(const rocksdb::Status& st, const char* step)
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

int CheckNotFound(const rocksdb::Status& st, const char* step)
{
	if (!st.IsNotFound()) {
		uk_pr_err("rocksdb: fail step=%s expected=not-found got=%s\n",
			  step, st.ToString().c_str());
		return 1;
	}

	return 0;
}

int CheckIoOk(const rocksdb::IOStatus& st, const char* step)
{
	if (!st.ok()) {
		uk_pr_err("rocksdb: fail step=%s io=%s\n", step, st.ToString().c_str());
		return 1;
	}

	return 0;
}

bool FeatureEnabled(const AppConfig& cfg, unsigned long feature)
{
	return (cfg.features & feature) != 0;
}

bool HasName(const std::vector<std::string>& names, const std::string& expected)
{
	for (const auto& name : names)
		if (name == expected)
			return true;

	return false;
}

void Phase(const AppConfig& cfg, const char* name)
{
	if (cfg.phase_logs)
		uk_pr_info("rocksdb: phase: %s\n", name);
}

rocksdb::Options MakeOptions(void)
{
	rocksdb::Options options;

	options.create_if_missing = true;
	options.error_if_exists = false;
	options.paranoid_checks = true;
	options.compression = rocksdb::kNoCompression;
	options.allow_mmap_reads = false;
	options.allow_mmap_writes = false;
	options.avoid_unnecessary_blocking_io = false;
	options.bytes_per_sync = 0;
	options.wal_bytes_per_sync = 0;

	return options;
}

rocksdb::Env* GetEnv(const rocksdb::Options& options)
{
	return options.env ? options.env : rocksdb::Env::Default();
}

std::string DerivedPath(const char* base, const char* suffix)
{
	return std::string(base) + suffix;
}

void MaybeDeleteFile(const AppConfig& cfg, const rocksdb::Options& options,
		     const char* path)
{
	rocksdb::Status status;

	if (!cfg.clean_before)
		return;

	std::remove(path);
	status = GetEnv(options)->DeleteFile(path);
	if (!status.ok() && !status.IsNotFound())
		uk_pr_warn("rocksdb: file cleanup skipped for %s: %s\n",
			   path, status.ToString().c_str());
}

int MaybeDestroy(const AppConfig& cfg, const rocksdb::Options& options,
		 const char* db_path)
{
	if (!cfg.clean_before)
		return 0;

	rocksdb::Status status = rocksdb::DestroyDB(db_path, options);
	if (!status.ok())
		uk_pr_warn("rocksdb: initial cleanup skipped: %s\n", status.ToString().c_str());

	return 0;
}

} /* namespace approcksdb */