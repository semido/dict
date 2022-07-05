#pragma once
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

class MMFileRead
{
public:
  MMFileRead(const std::string& name)
  {
    auto f = open(name.data(), O_RDONLY);
    if (f < 0) {
      std::cout << "Cannot open file: " << name << "(" << strerror(errno) << ")\n";
      return;
    }

    struct stat st;
    auto err = fstat(f, &st);
    if (err < 0) {
      close(f);
      std::cout << "Cannot get fstat: " << name << "(" << strerror(errno) << ")\n";
      return;
    }

    p = mmap(nullptr, st.st_size, PROT_READ , MAP_PRIVATE, f, 0);
    close(f);
    if (p == MAP_FAILED) {
      p = nullptr;
      std::cout << "Cannot map file: " << name << "(" << strerror(errno) << ")\n";
      return;
    }
    sz = st.st_size;
  }
  ~MMFileRead()
  {
    if (p == nullptr)
      return;
    auto err = munmap(p, sz);
    if (err != 0) {
      std::cout << "File unmap failed\n";
    }
  }
  void* ptr() const { return p; }
  size_t size() const { return sz; }
private:
  void* p = nullptr;
  size_t sz = 0;
};
