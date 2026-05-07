#include <uk/print.h>

#include <string>

#include <leveldb/db.h>

int main(int argc, char *argv[])
{
	const char *db_path = "/leveldb-demo";
	const char *key = "hello";
	const char *value = "unikraft";

	if (argc > 1)
		db_path = argv[1];
	if (argc > 2)
		key = argv[2];
	if (argc > 3)
		value = argv[3];

	uk_pr_info("leveldb: opening %s\n", db_path);

	leveldb::Options options;
	options.create_if_missing = true;

	leveldb::DB *db = nullptr;
	leveldb::Status status = leveldb::DB::Open(options, db_path, &db);
	if (!status.ok()) {
		uk_pr_err("leveldb: open failed: %s\n", status.ToString().c_str());
		return 1;
	}

	status = db->Put(leveldb::WriteOptions(), key, value);
	if (!status.ok()) {
		uk_pr_err("leveldb: put failed: %s\n", status.ToString().c_str());
		delete db;
		return 1;
	}

	std::string read_value;
	status = db->Get(leveldb::ReadOptions(), key, &read_value);
	if (!status.ok()) {
		uk_pr_err("leveldb: get failed: %s\n", status.ToString().c_str());
		delete db;
		return 1;
	}

	uk_pr_info("leveldb: %s=%s\n", key, read_value.c_str());

	status = db->Delete(leveldb::WriteOptions(), key);
	if (!status.ok()) {
		uk_pr_err("leveldb: delete failed: %s\n", status.ToString().c_str());
		delete db;
		return 1;
	}

	delete db;
	return 0;
}
