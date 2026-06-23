#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <leveldb/db.h>

int main()
{
	mkdir("/tmp", 0755);

	leveldb::Options options;
	options.create_if_missing = true;

	leveldb::DB *db;
	if (!leveldb::DB::Open(options, "/tmp/testdb", &db).ok())
		return 1;

	if (!db->Put(leveldb::WriteOptions(), "key", "hello from LevelDB").ok()) {
		delete db;
		return 1;
	}

	std::string value;
	if (!db->Get(leveldb::ReadOptions(), "key", &value).ok()) {
		delete db;
		return 1;
	}

	printf("%s\n", value.c_str());

	delete db;
	return 0;
}
