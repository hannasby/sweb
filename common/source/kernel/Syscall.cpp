#include "Syscall.h"
#include "File.h"
#include "PageManager.h"
#include "ProcessRegistry.h"
#include "Terminal.h"
#include "UserProcess.h"
#include "VfsSyscall.h"
#include "debug_bochs.h"
#include "offsets.h"
#include "syscall-definitions.h"
#include "Loader.h"
#include "ArchMemory.h"

size_t Syscall::syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
  size_t return_value = 0;

  if ((syscall_number != sc_sched_yield) && (syscall_number != sc_outline)) // no debug print because these might occur very often
  {
    debug(SYSCALL, "Syscall %zd called with arguments %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx) %zd(=%zx)\n",
          syscall_number, arg1, arg1, arg2, arg2, arg3, arg3, arg4, arg4, arg5, arg5);
  }

  switch (syscall_number)
  {
    case sc_sched_yield:
      Scheduler::instance()->yield();
      break;
    case sc_createprocess:
      return_value = createprocess(arg1, arg2);
      break;
    case sc_exit:
      exit(arg1);
      break;
    case sc_write:
      return_value = write(arg1, arg2, arg3);
      break;
    case sc_read:
      return_value = read(arg1, arg2, arg3);
      break;
    case sc_open:
      return_value = open(arg1, arg2);
      break;
    case sc_close:
      return_value = close(arg1);
      break;
    case sc_outline:
      outline(arg1, arg2);
      break;
    case sc_trace:
      trace();
      break;
    case sc_pseudols:
      VfsSyscall::readdir((const char*)arg1);
      break;
    case sc_map_read:
      return_value = readMap(arg1, arg2, arg3);
      break;
    case sc_map_write:
      return_value = writeMap(arg1, arg2, arg3);
      break;
    case sc_virtualmem:
      return_value = virtualMemTutorial((char*) arg1);
      break;
    default:
      kprintf("Syscall::syscall_exception: Unimplemented Syscall Number %zd\n", syscall_number);
  }
  return return_value;
}

void Syscall::exit(size_t exit_code)
{
  debug(SYSCALL, "Syscall::EXIT: called, exit_code: %zd\n", exit_code);
  currentThread->kill();
}

size_t Syscall::write(size_t fd, pointer buffer, size_t size)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if ((buffer >= USER_BREAK) || (buffer + size > USER_BREAK))
  {
    return -1U;
  }

  size_t num_written = 0;

  if (fd == fd_stdout) //stdout
  {
    debug(SYSCALL, "Syscall::write: %.*s\n", (int)size, (char*) buffer);
    kprintf("%.*s", (int)size, (char*) buffer);
    num_written = size;
  }
  else
  {
    num_written = VfsSyscall::write(fd, (char*) buffer, size);
  }
  return num_written;
}

size_t Syscall::read(size_t fd, pointer buffer, size_t count)
{
  if ((buffer >= USER_BREAK) || (buffer + count > USER_BREAK))
  {
    return -1U;
  }

  size_t num_read = 0;

  if (fd == fd_stdin)
  {
    //this doesn't! terminate a string with \0, gotta do that yourself
    num_read = currentThread->getTerminal()->readLine((char*) buffer, count);
    debug(SYSCALL, "Syscall::read: %.*s\n", (int)num_read, (char*) buffer);
  }
  else
  {
    num_read = VfsSyscall::read(fd, (char*) buffer, count);
  }
  return num_read;
}

size_t Syscall::close(size_t fd)
{
  return VfsSyscall::close(fd);
}

size_t Syscall::open(size_t path, size_t flags)
{
  if (path >= USER_BREAK)
  {
    return -1U;
  }
  return VfsSyscall::open((char*) path, flags);
}

void Syscall::outline(size_t port, pointer text)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if (text >= USER_BREAK)
  {
    return;
  }
  if (port == 0xe9) // debug port
  {
    writeLine2Bochs((const char*) text);
  }
}

size_t Syscall::createprocess(size_t path, size_t sleep)
{
  // THIS METHOD IS FOR TESTING PURPOSES ONLY!
  // AVOID USING IT AS SOON AS YOU HAVE AN ALTERNATIVE!

  // parameter check begin
  if (path >= USER_BREAK)
  {
    return -1U;
  }
  debug(SYSCALL, "Syscall::createprocess: path:%s sleep:%zd\n", (char*) path, sleep);
  ssize_t fd = VfsSyscall::open((const char*) path, O_RDONLY);
  if (fd == -1)
  {
    return -1U;
  }
  VfsSyscall::close(fd);
  // parameter check end

  size_t process_count = ProcessRegistry::instance()->processCount();
  ProcessRegistry::instance()->createProcess((const char*) path);
  if (sleep)
  {
    while (ProcessRegistry::instance()->processCount() > process_count) // please note that this will fail ;)
    {
      Scheduler::instance()->yield();
    }
  }
  return 0;
}

void Syscall::trace()
{
  currentThread->printBacktrace();
}

Mutex mutex_map_("mutex_map_");

size_t Syscall::writeMap(size_t key, size_t str, size_t len) 
{
  if (len > PAGE_SIZE || str == 0 || ((str >= USER_BREAK) || (str + len > USER_BREAK))) 
  {
    return -1ULL;
  }

  char buf[len];
  memcpy((char*) buf, (char*) str, len);

  // or MutexLock
  mutex_map_.acquire();
  auto it = PageManager::instance()->map_.find(key);
  if (it != PageManager::instance()->map_.end()) 
  {
    mutex_map_.release();
    return -1ULL;
  }

  char* str_2 = new char(len);
  memcpy(str_2, (char*) buf, len);

  PageManager::instance()->map_.insert({key, str_2});
  mutex_map_.release();
  return 0;
}

size_t Syscall::readMap(size_t key, size_t str, size_t len)
{
  if (len > PAGE_SIZE || str == 0 || ((str >= USER_BREAK) || (str + len > USER_BREAK))) 
  {
    return -1ULL;
  }

  mutex_map_.acquire();

  auto it = PageManager::instance()->map_.find(key);
  if (it == PageManager::instance()->map_.end()) 
  {
    mutex_map_.release();
    return -1ULL;
  }

  char buf[len];
  memcpy((char*) buf, (char*) it, len);
  mutex_map_.release();

  memcpy((char*) str, (char*)buf, len);
  return 0;
}

size_t Syscall::virtualMemTutorial(char* str)
{
  debug(SYSCALL, "addr %p \n", str);
  size_t vpn = (size_t) str;
  
  size_t pml4i = (vpn >> 39);
  debug(SYSCALL, "pml4i %zd \n", pml4i);
  size_t pml4Address = ArchMemory::getIdentAddressOfPPN(currentThread->loader_->arch_memory_.page_map_level_4_);
  size_t ppn_pdpt = ((PageMapLevel4Entry*) pml4Address)[pml4i].page_ppn;
  debug(SYSCALL, "ppn_pdpt %zd \n", ppn_pdpt);

  size_t pdpti = (vpn >> 30) & 0x1FF;
  debug(SYSCALL, "pdpti %zd \n", pdpti);
  size_t pdptAddress = ArchMemory::getIdentAddressOfPPN(ppn_pdpt);
  size_t ppn_pd = ((PageDirPointerTableEntry*) pdptAddress)[pdpti].pd.page_ppn;
  debug(SYSCALL, "ppn_pd %zd \n", ppn_pd);

  size_t pdi = (vpn >> 21) & 0x1FF; // 0000000000 1 1111 1111
  debug(SYSCALL, "pdi %zd \n", pdi);
  size_t pdAddress = ArchMemory::getIdentAddressOfPPN(ppn_pd);
  size_t ppn_pt = ((PageDirEntry*)pdAddress)[pdi].pt.page_ppn;
  debug(SYSCALL, "ppn_pt %zd \n", ppn_pt);

  size_t pti = (vpn >> 12) & 0x1FF;
  debug(SYSCALL, "pti %zd \n", pti);
  size_t ptAddress = ArchMemory::getIdentAddressOfPPN(ppn_pt);
  size_t page = ((PageTableEntry*)ptAddress)[pti].page_ppn;
  debug(SYSCALL, "page %zd \n", page);

  char* p = (char*) ArchMemory::getIdentAddressOfPPN(page);
  size_t offset = vpn & 0xFFF; // 1111 1111 1111
  debug(SYSCALL, "offset %zd \n", offset);
  char x = p[offset];

  debug(SYSCALL, "X %c \n", x);

  return 0;
}
