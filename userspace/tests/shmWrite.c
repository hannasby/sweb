#include "nonstd.h"
#include "stdio.h"

int main () 
{
  char* __attribute__((unused)) str = "Hello SWEB";
  // sharedWrite((char*) 0x0000800000000001ULL, 11);
  sharedWrite(str, 11);
}
