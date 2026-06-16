#include "approcksdb/cli.hpp"

#include <uk/print.h>

#include <cstring>

namespace approcksdb {

static bool parse_feature(const char* value, unsigned long* feature)
{
	if (!std::strcmp(value, "all")) {
		*feature = kDefaultFeatures;
		return true;
	}
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
	if (!std::strcmp(value, "merge")) {
		*feature = FEATURE_MERGE;
		return true;
	}
	if (!std::strcmp(value, "column-family")) {
		*feature = FEATURE_COLUMN_FAMILY;
		return true;
	}
	if (!std::strcmp(value, "sst-ingest")) {
		*feature = FEATURE_SST_INGEST;
		return true;
	}
	if (!std::strcmp(value, "write-batch-with-index")) {
		*feature = FEATURE_WBWI;
		return true;
	}
	if (!std::strcmp(value, "optimistic-transaction")) {
		*feature = FEATURE_OPTIMISTIC_TRANSACTION;
		return true;
	}
	if (!std::strcmp(value, "compaction-filter")) {
		*feature = FEATURE_COMPACTION_FILTER;
		return true;
	}

	return false;
}

static void print_usage(void)
{
	uk_pr_info("rocksdb usage:\n");
	uk_pr_info("  --db=<path>          DB directory path (default /rocksdb-demo)\n");
	uk_pr_info("  --checkpoint=<path>  checkpoint directory (default /rocksdb-demo-checkpoint)\n");
	uk_pr_info("  --feature=<name>     debug one feature: all | basic | batch | flush | snapshot | iterator | delete | reopen | checkpoint | transaction | backup\n");
	uk_pr_info("                       | cache-dump-load | secondary-index | merge | column-family | sst-ingest | write-batch-with-index\n");
	uk_pr_info("                       | optimistic-transaction | compaction-filter\n");
	uk_pr_info("                       repeat --feature to run multiple slices; default is all\n");
	uk_pr_info("  --all-features       explicit alias for running all feature slices\n");
	uk_pr_info("  --storage=<name>     label only: ramfs | 9p | blk | initrd\n");
	uk_pr_info("  --no-clean           skip DestroyDB() before the run\n");
	uk_pr_info("  --no-phase-logs      reduce phase logs\n");
	uk_pr_info("  --help               show this message\n");
}

const char* FeatureName(unsigned long feature)
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
	case FEATURE_MERGE:
		return "merge";
	case FEATURE_COLUMN_FAMILY:
		return "column-family";
	case FEATURE_SST_INGEST:
		return "sst-ingest";
	case FEATURE_WBWI:
		return "write-batch-with-index";
	case FEATURE_OPTIMISTIC_TRANSACTION:
		return "optimistic-transaction";
	case FEATURE_COMPACTION_FILTER:
		return "compaction-filter";
	}

	return "unknown";
}

int ParseArgs(int argc, char* argv[], AppConfig* cfg)
{
	for (int i = 1; i < argc; i++) {
		if (!std::strcmp(argv[i], "--"))
			continue;
		if (!std::strcmp(argv[i], "--help")) {
			print_usage();
			return 1;
		}
		if (!std::strcmp(argv[i], "--all-features")) {
			cfg->features = kDefaultFeatures;
			cfg->features_explicit = true;
			continue;
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

} /* namespace approcksdb */