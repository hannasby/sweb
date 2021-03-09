#include "stdio.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "sys/syscall.h"
#include "assert.h"
#include "string.h"

char* str = "SWEB";

int main()
{
    printf("STR: %s \n", str); // access str
    __syscall(sc_tutorial, (size_t) str, 0x00, 0x00, 0x00, 0x00);
}