#include "nonstd.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "stdlib.h"

int createprocess(const char* path, int sleep)
{
  return __syscall(sc_createprocess, (long) path, sleep, 0x00, 0x00, 0x00);
}

int changeByte(const char* str)
{
  return __syscall(sc_change_byte, (size_t)str, 0x00, 0x00, 0x00, 0x00);
}

extern int main();

void _start()
{
  exit(main());
}
