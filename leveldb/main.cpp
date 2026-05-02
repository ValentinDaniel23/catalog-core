#include <string>

#include <uk/print.h>

#include "leveldb_benchmark.hpp"

namespace {

void unikraft_log(void *, const char *message)
{
	uk_pr_err("%s\n", message);
}

} /* namespace */

int main(int argc, char *argv[])
{
	leveldb_benchmark::Config config =
		leveldb_benchmark::DefaultConfig("/leveldb-benchmark-db");
	std::string error;
	if (!leveldb_benchmark::ParseArgs(argc, argv, &config, &error)) {
		uk_pr_err("BENCHMARK|EVENT=ERROR|step=ParseArgs|message=%s\n",
			  error.c_str());
		return 2;
	}

	const leveldb_benchmark::Logger logger = {nullptr, unikraft_log};
	return leveldb_benchmark::RunBenchmark(config, logger);
}
