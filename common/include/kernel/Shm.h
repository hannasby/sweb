#include "paging-definitions.h"
#include "Mutex.h"

class Shm {
  public:
    static Shm* instance();

    void setString(char* string, size_t length);
    void getString(char* string, size_t length);

  private:
    static Shm* instance_;
    Shm();

    char* string_;
    Mutex lock_;
};
