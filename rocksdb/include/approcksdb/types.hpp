#pragma once

namespace approcksdb {

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
	FEATURE_MERGE = 1UL << 12,
	FEATURE_COLUMN_FAMILY = 1UL << 13,
	FEATURE_SST_INGEST = 1UL << 14,
	FEATURE_WBWI = 1UL << 15,
	FEATURE_OPTIMISTIC_TRANSACTION = 1UL << 16,
	FEATURE_COMPACTION_FILTER = 1UL << 17,
};

inline constexpr unsigned long kDefaultFeatures =
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
	FEATURE_SECONDARY_INDEX |
	FEATURE_MERGE |
	FEATURE_COLUMN_FAMILY |
	FEATURE_SST_INGEST |
	FEATURE_WBWI |
	FEATURE_OPTIMISTIC_TRANSACTION |
	FEATURE_COMPACTION_FILTER;

inline constexpr unsigned long kLastFeature = FEATURE_COMPACTION_FILTER;

struct AppConfig {
	const char* db_path = "/rocksdb-demo";
	const char* checkpoint_path = "/rocksdb-demo-checkpoint";
	const char* storage = "ramfs";
	unsigned long features = kDefaultFeatures;
	bool clean_before = true;
	bool phase_logs = true;
	bool features_explicit = false;
};

} /* namespace approcksdb */