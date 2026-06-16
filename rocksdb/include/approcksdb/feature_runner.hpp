#pragma once

#include "approcksdb/features.hpp"

namespace approcksdb {

class FeatureRunner {
public:
	explicit FeatureRunner(AppConfig cfg);

	int Run();

private:
	int RunCoreFeatures();
	int RunStorageFeatures();
	int RunAdvancedFeatures();
	void PrintCompletedFeatures() const;
	void ClosePrimaryDb();
	void CleanupAfterBackup();
	void CleanupAfterCacheDumpLoad();
	void CleanupAfterTransaction();
	void CleanupAfterSecondaryIndex();
	void CleanupAfterWbwi();

	AppConfig cfg_;
	rocksdb::Options options_;
	std::unique_ptr<rocksdb::DB> db_;
};

} /* namespace approcksdb */