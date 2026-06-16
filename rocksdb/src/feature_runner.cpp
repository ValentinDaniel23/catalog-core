#include "approcksdb/feature_runner.hpp"

#include "approcksdb/cli.hpp"

namespace approcksdb {

namespace {

template <typename RunFn>
int RunEnabledFeature(const AppConfig& cfg, unsigned long feature, RunFn&& run_feature_fn)
{
	if (!FeatureEnabled(cfg, feature))
		return 0;

	return run_feature_fn();
}

template <typename RunFn, typename CleanupFn>
int RunEnabledFeatureWithCleanup(const AppConfig& cfg, unsigned long feature,
					 RunFn&& run_feature_fn,
					 CleanupFn&& cleanup_fn)
{
	if (!FeatureEnabled(cfg, feature))
		return 0;

	int rc = run_feature_fn();
	if (rc != 0)
		return rc;

	cleanup_fn();
	return 0;
}

} /* namespace */

FeatureRunner::FeatureRunner(AppConfig cfg)
	: cfg_(cfg)
	, options_(MakeOptions())
{
}

int FeatureRunner::RunCoreFeatures()
{
	if (RunEnabledFeature(cfg_, FEATURE_BASIC,
			      [this] { return RunBasicFeature(cfg_, db_.get()); }))
		return 1;
	if (RunEnabledFeature(cfg_, FEATURE_BATCH,
			      [this] { return RunBatchFeature(cfg_, db_.get()); }))
		return 1;
	if (RunEnabledFeature(cfg_, FEATURE_FLUSH,
			      [this] { return RunFlushFeature(cfg_, db_.get()); }))
		return 1;
	if (RunEnabledFeature(cfg_, FEATURE_SNAPSHOT,
			      [this] { return RunSnapshotFeature(cfg_, db_.get()); }))
		return 1;
	if (RunEnabledFeature(cfg_, FEATURE_ITERATOR,
			      [this] { return RunIteratorFeature(cfg_, db_.get()); }))
		return 1;
	if (RunEnabledFeature(cfg_, FEATURE_DELETE,
			      [this] { return RunDeleteFeature(cfg_, db_.get()); }))
		return 1;
	if (RunEnabledFeature(cfg_, FEATURE_REOPEN,
			      [this] { return RunReopenFeature(cfg_, options_, &db_); }))
		return 1;
	if (RunEnabledFeature(
		    cfg_, FEATURE_CHECKPOINT,
		    [this] { return RunCheckpointFeature(cfg_, db_.get(), options_); }))
		return 1;

	return 0;
}

int FeatureRunner::RunStorageFeatures()
{
	if (RunEnabledFeatureWithCleanup(
		    cfg_, FEATURE_BACKUP,
		    [this] { return RunBackupFeature(cfg_, db_.get(), options_); },
		    [this] { CleanupAfterBackup(); }))
		return 1;
	if (RunEnabledFeatureWithCleanup(
		    cfg_, FEATURE_CACHE_DUMP_LOAD,
		    [this] { return RunCacheDumpLoadFeature(cfg_); },
		    [this] { CleanupAfterCacheDumpLoad(); }))
		return 1;

	ClosePrimaryDb();

	if (RunEnabledFeatureWithCleanup(
		    cfg_, FEATURE_TRANSACTION,
		    [this] { return RunTransactionFeature(cfg_, options_); },
		    [this] { CleanupAfterTransaction(); }))
		return 1;
	if (RunEnabledFeatureWithCleanup(
		    cfg_, FEATURE_SECONDARY_INDEX,
		    [this] { return RunSecondaryIndexFeature(cfg_, options_); },
		    [this] { CleanupAfterSecondaryIndex(); }))
		return 1;

	return 0;
}

int FeatureRunner::RunAdvancedFeatures()
{
	if (RunEnabledFeature(cfg_, FEATURE_MERGE,
			      [this] { return RunMergeFeature(cfg_); }))
		return 1;
	if (RunEnabledFeature(cfg_, FEATURE_COLUMN_FAMILY,
			      [this] { return RunColumnFamilyFeature(cfg_); }))
		return 1;
	if (RunEnabledFeature(cfg_, FEATURE_SST_INGEST,
			      [this] { return RunSstIngestFeature(cfg_); }))
		return 1;
	if (RunEnabledFeatureWithCleanup(
		    cfg_, FEATURE_WBWI,
		    [this] { return RunWbwiFeature(cfg_); },
		    [this] { CleanupAfterWbwi(); }))
		return 1;
	if (RunEnabledFeature(cfg_, FEATURE_OPTIMISTIC_TRANSACTION,
			      [this] { return RunOptimisticTransactionFeature(cfg_); }))
		return 1;
	if (RunEnabledFeature(cfg_, FEATURE_COMPACTION_FILTER,
			      [this] { return RunCompactionFilterFeature(cfg_); }))
		return 1;

	return 0;
}

void FeatureRunner::PrintCompletedFeatures() const
{
	for (unsigned long feature = FEATURE_BASIC; feature <= kLastFeature;
	     feature <<= 1) {
		if (FeatureEnabled(cfg_, feature))
			uk_pr_info("rocksdb: completed feature %s\n", FeatureName(feature));
	}
}

void FeatureRunner::ClosePrimaryDb()
{
	db_.reset();
}

void FeatureRunner::CleanupAfterBackup()
{
	if (!FeatureEnabled(cfg_, FEATURE_BACKUP))
		return;

	std::string backup_path = DerivedPath(cfg_.db_path, "-backup");
	std::string restore_path = DerivedPath(cfg_.db_path, "-restore");

	ClosePrimaryDb();
	MaybeDestroy(cfg_, options_, cfg_.db_path);
	if (cfg_.checkpoint_path)
		MaybeDestroy(cfg_, options_, cfg_.checkpoint_path);
	MaybeDestroy(cfg_, options_, backup_path.c_str());
	MaybeDestroy(cfg_, options_, restore_path.c_str());
}

void FeatureRunner::CleanupAfterCacheDumpLoad()
{
	if (!FeatureEnabled(cfg_, FEATURE_CACHE_DUMP_LOAD))
		return;

	std::string cache_path = DerivedPath(cfg_.db_path, "-cache");
	std::string dump_path = DerivedPath(cfg_.db_path, "-cache.dump");

	MaybeDeleteFile(cfg_, options_, dump_path.c_str());
	MaybeDestroy(cfg_, options_, cache_path.c_str());
}

void FeatureRunner::CleanupAfterTransaction()
{
	if (!FeatureEnabled(cfg_, FEATURE_TRANSACTION))
		return;

	MaybeDestroy(cfg_, options_, cfg_.db_path);
	{
		std::string prepared_path = std::string(cfg_.db_path) + "-wp";
		MaybeDestroy(cfg_, options_, prepared_path.c_str());
	}
	if (cfg_.checkpoint_path)
		MaybeDestroy(cfg_, options_, cfg_.checkpoint_path);
}

void FeatureRunner::CleanupAfterSecondaryIndex()
{
	if (!FeatureEnabled(cfg_, FEATURE_SECONDARY_INDEX))
		return;

	std::string secondary_index_path = DerivedPath(cfg_.db_path, "-secondary-index");
	MaybeDestroy(cfg_, options_, secondary_index_path.c_str());
}

void FeatureRunner::CleanupAfterWbwi()
{
	if (!FeatureEnabled(cfg_, FEATURE_WBWI))
		return;

	std::string wbwi_path = DerivedPath(cfg_.db_path, "-wbwi");
	MaybeDestroy(cfg_, options_, wbwi_path.c_str());
}

int FeatureRunner::Run()
{
	uk_pr_info("rocksdb: db=%s checkpoint=%s storage=%s clean=%s\n",
		   cfg_.db_path, cfg_.checkpoint_path, cfg_.storage,
		   cfg_.clean_before ? "yes" : "no");

	MaybeDestroy(cfg_, options_, cfg_.db_path);
	if (FeatureEnabled(cfg_, FEATURE_CHECKPOINT))
		MaybeDestroy(cfg_, options_, cfg_.checkpoint_path);

	Phase(cfg_, "open");
	if (CheckOk(rocksdb::DB::Open(options_, cfg_.db_path, &db_), "open"))
		return 1;

	if (RunCoreFeatures())
		return 1;
	if (RunStorageFeatures())
		return 1;
	if (RunAdvancedFeatures())
		return 1;
	PrintCompletedFeatures();

	uk_pr_info("rocksdb: feature debug run passed\n");
	return 0;
}

} /* namespace approcksdb */