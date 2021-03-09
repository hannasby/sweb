#include "offsets.h"
#include "Syscall.h"
#include "syscall-definitions.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "VfsSyscall.h"
#include "UserProcess.h"
#include "ProcessRegistry.h"
#include "File.h"
#include "ArchMemory.h"
#include "paging-definitions.h"
#include "Loader.h"

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
      VfsSyscall::readdir((const char*) arg1);
      break;
    case sc_tutorial:
      changeByte((char*) arg1);
      break;
    default:
      kprintf("Syscall::syscall_exception: Unimplemented Syscall Number %zd\n", syscall_number);
  }
  return return_value;
}

void Syscall::changeByte(char* str) 
{
  // TODO: check arguments

  debug(SYSCALL, "addr %p \n", str);
  size_t virtual_address = (size_t) str;

  // 9 bit PML4i
  // 9 bit PDPTi
  // 9 bit page dir i
  // 9 bit for page table i
  // 12 bit offset

  size_t pml4i = (virtual_address >> 39);
  debug(SYSCALL, "pml4i %zd \n", pml4i);

  size_t pdpti = (virtual_address >> 30) & 0x1FF;
  debug(SYSCALL, "pdpti %zd \n", pdpti);

  size_t pdi = (virtual_address >> 21) & 0x1FF;
  debug(SYSCALL, "pdi %zd \n", pdi);

  size_t pti = (virtual_address >> 12) & 0x1FF;
  debug(SYSCALL, "pti %zd \n", pti);

  size_t offset = (virtual_address) & 0xFFF;
  debug(SYSCALL, "offset %zd \n", offset);

  size_t pml4Address = ArchMemory::getIdentAddressOfPPN(currentThread->loader_->arch_memory_.page_map_level_4_);
  PageMapLevel4Entry* pml4Entry = ((PageMapLevel4Entry*) pml4Address);
  size_t ppn_pdpt = pml4Entry->page_ppn;
  debug(SYSCALL, "ppn_pdpt %zd \n", ppn_pdpt);

  size_t pdptAddress = ArchMemory::getIdentAddressOfPPN(ppn_pdpt);
  size_t ppn_pd = ((PageDirPointerTableEntry *) pdptAddress)[pdpti].pd.page_ppn;
  debug(SYSCALL, "ppn_pd %zd \n", ppn_pd);

  size_t pdAddress = ArchMemory::getIdentAddressOfPPN(ppn_pd);
  size_t ppn_pt = ((PageDirEntry *) pdAddress)[pdi].pt.page_ppn;
  debug(SYSCALL, "ppn_pt %zd \n", ppn_pt);

  size_t ptAddress = ArchMemory::getIdentAddressOfPPN(ppn_pt);
  size_t ppn_page = ((PageTableEntry *) ptAddress)[pti].page_ppn;
  debug(SYSCALL, "ppn_page %zd \n", ppn_page);

  size_t pageAddress = ArchMemory::getIdentAddressOfPPN(ppn_page);
  char* pageAddressChar = (char*) pageAddress;
  char firstChar = pageAddressChar[offset];
  debug(SYSCALL, "firstChar= %d \n", firstChar);
  debug(SYSCALL, "firstChar= %c \n", firstChar);
  debug(SYSCALL, "p= %p \n", pageAddressChar);

  // OR:
  ArchMemoryMapping mapping = ArchMemory::resolveMapping(currentThread->loader_->arch_memory_.page_map_level_4_, virtual_address >> 12);
  debug(SYSCALL, "p= %lx \n", mapping.page);

  debug(SYSCALL, "str = %s \n", str);
  pageAddressChar[offset] = 'X';
  debug(SYSCALL, "str = %s \n", str);
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

