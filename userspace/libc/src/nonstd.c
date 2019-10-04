#include "nonstd.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "stdlib.h"

int sharedRead(char* string, int length)
{
  return __syscall(sc_shmread, (long) string, length, 0x00, 0x00, 0x00);
}

int sharedWrite(char* string, int length)
{
  return __syscall(sc_shmwrite, (long) string, length, 0x00, 0x00, 0x00);
}

int createprocess(const char* path, int sleep)
{
  return __syscall(sc_createprocess, (long) path, sleep, 0x00, 0x00, 0x00);
}

extern int main();

void _start()
{
  exit(main());
}
