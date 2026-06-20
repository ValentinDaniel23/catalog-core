#include <iostream>
#include <uk/config.h>

#if CONFIG_LIBROCKSDBTEST
extern "C" int rocksdb_test_main(void);
#endif

int main()
{
	std::cout << "hello from the RocksDB Unikraft app" << std::endl;

#if CONFIG_LIBROCKSDBTEST
	return rocksdb_test_main();
#else
	return 0;
#endif
}
