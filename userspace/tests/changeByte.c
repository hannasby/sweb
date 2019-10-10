#include "../../common/include/kernel/syscall-definitions.h"
#include "nonstd.h"
#include "stdio.h"

int main()
{
  char* str = "HELLO SWEB!";
  printf("Before changeByte: %s \n", str);
  changeByte(str);
  printf("After changeByte: %s \n", str);
  return 0;
}
