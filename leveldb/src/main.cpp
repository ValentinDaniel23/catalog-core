#include <iostream>
#include <uk/config.h>

#if CONFIG_LIBLEVELDBTEST
extern "C" int leveldb_test_main(void);
#endif

int main()
{
	std::cout << "hello from the LevelDB Unikraft app" << std::endl;

#if CONFIG_LIBLEVELDBTEST
	return leveldb_test_main();
#else
	return 0;
#endif
}
