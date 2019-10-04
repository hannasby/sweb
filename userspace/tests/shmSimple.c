#include "nonstd.h"
#include "stdio.h"

int main () 
{
  char* str = "Hello SWEB";
  sharedWrite(str, 11);

  char buffer[11];
  sharedRead(buffer, 11);

  printf("%s", buffer);
}
