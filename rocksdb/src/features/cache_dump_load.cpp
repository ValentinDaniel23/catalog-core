#include "approcksdb/features.hpp"

namespace approcksdb {

int RunCacheDumpLoadFeature(const AppConfig& cfg)
{
	rocksdb::Options options = MakeOptions();
	std::string db_path = DerivedPath(cfg.db_path, "-cache");
	std::string dump_path = DerivedPath(cfg.db_path, "-cache.dump");
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

	Phase(cfg, "cache dump/load setup");
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

	MaybeDestroy(cfg, options, db_path.c_str());
	MaybeDeleteFile(cfg, options, dump_path.c_str());
	if (CheckOk(rocksdb::DB::Open(options, db_path, &db), "cache-open"))
		return 1;

	if (CheckOk(db->Put(rocksdb::WriteOptions(), "cache-key-a", value),
	    "cache-put-a"))
		return 1;
	if (CheckOk(db->Put(rocksdb::WriteOptions(), "cache-key-b", value + "-b"),
	    "cache-put-b"))
		return 1;
	fopts.wait = true;
	if (CheckOk(db->Flush(fopts), "cache-flush"))
		return 1;
	if (CheckOk(db->Get(rocksdb::ReadOptions(), "cache-key-a", &value),
	    "cache-read-a"))
		return 1;

	dump_options.clock = GetEnv(options)->GetSystemClock().get();
	Phase(cfg, "cache dump");
	if (CheckIoOk(rocksdb::NewToFileCacheDumpWriter(GetEnv(options)->GetFileSystem(),
					 file_options, dump_path, &writer),
	      "cache-writer"))
		return 1;
	if (CheckOk(rocksdb::NewDefaultCacheDumper(dump_options, block_cache,
					   std::move(writer), &dumper),
	    "cache-dumper"))
		return 1;
	if (CheckOk(dumper->SetDumpFilter({db.get()}), "cache-dump-filter"))
		return 1;
	if (CheckIoOk(dumper->DumpCacheEntriesToWriter(), "cache-dump-run"))
		return 1;
	if (CheckOk(GetEnv(options)->GetFileSize(dump_path, &dump_size),
	    "cache-dump-size"))
		return 1;
	if (dump_size == 0) {
		uk_pr_err("rocksdb: cache dump file is empty\n");
		return 1;
	}

	Phase(cfg, "cache load");
	if (CheckIoOk(rocksdb::NewFromFileCacheDumpReader(GetEnv(options)->GetFileSystem(),
					      file_options, dump_path, &reader),
	      "cache-reader"))
		return 1;
	if (CheckOk(rocksdb::NewDefaultCacheDumpedLoader(dump_options, table_options,
					 secondary_cache,
					 std::move(reader), &loader),
	    "cache-loader"))
		return 1;
	if (CheckIoOk(loader->RestoreCacheEntriesToSecondaryCache(), "cache-load-run"))
		return 1;

	uk_pr_info("rocksdb: feature cache-dump-load passed (dump_size=%llu)\n",
		   static_cast<unsigned long long>(dump_size));
	return 0;
}

} /* namespace approcksdb */