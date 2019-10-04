#include "nonstd.h"
#include "stdio.h"

int main () 
{
  char buffer[11];
  sharedRead(buffer, 11);

  printf("%s", buffer);
}
