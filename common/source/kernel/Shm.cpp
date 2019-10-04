#include "Shm.h"
#include "kstring.h"
#include "debug.h"
#include "offsets.h"
#include "assert.h"

Shm* Shm::instance_ = 0;

Shm::Shm(): string_(0), lock_(0)
{
  debug(SHM, "ctor SHM \n");
}

Shm* Shm::instance()
{
  if (instance_ == 0) 
  {
    instance_ = new Shm();
  }
  return instance_;
}

void Shm::setString(char* string, size_t length)
{
  if ((size_t) string >= USER_BREAK || ((size_t) string + length > USER_BREAK)) 
  {
    assert(false && "oh no");
  }

  debug(SHM, "Shm::setString: %p", string);

  lock_.acquire();
  string_ = new char(length);
  strncpy(string_, string, length);
  lock_.release();
}

void Shm::getString(char* string, size_t length)
{
  lock_.acquire();
  strncpy(string, string_, length);
  lock_.release();
}
