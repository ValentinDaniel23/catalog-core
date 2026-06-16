#include "approcksdb/features.hpp"

namespace approcksdb {

int RunBackupFeature(const AppConfig& cfg, rocksdb::DB* db,
	     const rocksdb::Options& options)
{
	std::string backup_path = DerivedPath(cfg.db_path, "-backup");
	std::string restore_path = DerivedPath(cfg.db_path, "-restore");
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

	Phase(cfg, "backup setup");
	backup_options.destroy_old_data = cfg.clean_before;
	MaybeDestroy(cfg, options, restore_path.c_str());
	if (CheckOk(GetEnv(options)->CreateDirIfMissing(backup_path), "backup-create-dir"))
		return 1;

	if (CheckOk(db->Put(rocksdb::WriteOptions(), "backup-key", "backup-value"),
	    "backup-put-key"))
		return 1;
	fopts.wait = true;
	if (CheckOk(db->Flush(fopts), "backup-flush"))
		return 1;

	Phase(cfg, "backup create");
	if (CheckOk(rocksdb::BackupEngine::Open(backup_options, GetEnv(options),
				     &raw_backup_engine),
	    "backup-open"))
		return 1;
	backup_engine.reset(raw_backup_engine);

	if (CheckIoOk(backup_engine->CreateNewBackup(db), "backup-create"))
		return 1;
	backup_engine->GetBackupInfo(&backup_info);
	if (backup_info.empty()) {
		uk_pr_err("rocksdb: backup metadata is empty after CreateNewBackup\n");
		return 1;
	}
	if (CheckIoOk(backup_engine->VerifyBackup(backup_info.back().backup_id),
	      "backup-verify"))
		return 1;

	if (CheckOk(db->Put(rocksdb::WriteOptions(), "backup-late-key", "late-value"),
	    "backup-put-late-key"))
		return 1;

	Phase(cfg, "backup restore");
	if (CheckOk(rocksdb::BackupEngineReadOnly::Open(backup_read_options,
					     GetEnv(options),
					     &raw_restore_engine),
	    "backup-open-readonly"))
		return 1;
	restore_engine.reset(raw_restore_engine);

	if (CheckIoOk(restore_engine->RestoreDBFromLatestBackup(restore_path, restore_path),
	      "backup-restore-latest"))
		return 1;

	if (CheckOk(rocksdb::DB::Open(options, restore_path, &restore_db),
	    "backup-open-restored-db"))
		return 1;
	if (CheckOk(restore_db->Get(rocksdb::ReadOptions(), "backup-key", &value),
	    "backup-read-restored-key"))
		return 1;
	if (value != "backup-value") {
		uk_pr_err("rocksdb: restored backup-key mismatch: got '%s'\n", value.c_str());
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

} /* namespace approcksdb */