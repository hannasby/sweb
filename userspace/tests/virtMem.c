#include "stdio.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "sys/syscall.h"
#include "assert.h"
#include "string.h"

int main() 
{ 
  printf("hi\n"); 
  char* str = "SWEB";
  printf("STR: %p \n", str);
  __syscall(sc_virtualmem, (size_t) str, 0x00, 0x00, 0x00, 0x00);
  printf("done!\n");
}