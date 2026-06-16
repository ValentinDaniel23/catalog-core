#include "approcksdb/cli.hpp"
#include "approcksdb/feature_runner.hpp"

int main(int argc, char** argv)
{
	approcksdb::AppConfig cfg;
	int parsed = approcksdb::ParseArgs(argc, argv, &cfg);

	if (parsed > 0)
		return 0;
	if (parsed < 0)
		return 1;

	approcksdb::FeatureRunner runner(cfg);
	return runner.Run();
}