#include <cstdio>
#include <unistd.h>
#include <errno.h>
#include <uk/print.h>

extern "C" int rocksdb_test_main(void);

int main(int argc, char** argv)
{
	const char msg[] = "[app] main() entered\n";
	ssize_t r1 = write(1, msg, sizeof(msg) - 1);
	ssize_t r2 = write(2, msg, sizeof(msg) - 1);
	uk_pr_info("[app] write(1)=%zd errno=%d write(2)=%zd errno=%d\n",
		   r1, errno, r2, errno);
	uk_pr_info("[app] calling rocksdb_test_main()\n");
	int r = rocksdb_test_main();
	uk_pr_info("[app] rocksdb_test_main() returned %d\n", r);
	return r;
}
