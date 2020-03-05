

#include "stdio.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "sys/syscall.h"
#include "assert.h"
#include "string.h"

int main() 
{ 
  printf("hi\n"); 
  int key = 4;
  char* str = "hello";
  size_t len = strlen(str);

  assert(__syscall(sc_map_write, key, (size_t) str, len, 0x00, 0x00) == 0);
  assert(__syscall(sc_map_write, key, (size_t) str, 4096 + 1, 0x00, 0x00) != 0);
  assert(__syscall(sc_map_write, key, (size_t) 0, len, 0x00, 0x00) != 0);
  assert(__syscall(sc_map_write, key, (size_t) 1<<48, len, 0x00, 0x00) != 0);

  char str_2[len];
  assert(__syscall(sc_map_read, key, (size_t) str_2, (size_t) len, 0x00, 0x00) == 0);
  assert(memcmp(str, str_2, (size_t) len));
  printf("done!\n");
}