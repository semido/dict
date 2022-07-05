#pragma once
#include "timer.hpp"
#include <cstdio>
#include <string>
using namespace std::string_literals;
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class File
{
  FILE* fp = nullptr;
public:
  File() = default;
  File(const std::string& name, const std::string& mode)
  {
    fp = std::fopen(name.data(), mode.data());
  }
  ~File()
  {
    close();
  }
  operator FILE* ()
  {
    return fp;
  }
  operator bool()
  {
    if (fp)
      return !std::ferror(fp);
    return false;
  }
  bool open(const std::string& name, const std::string& mode)
  {
    fp = std::fopen(name.data(), mode.data());
    return *this;
  }
  void close()
  {
    if (fp)
      std::fclose(fp);
    fp = nullptr;
  }
  size_t size()
  {
    auto pos = std::ftell(fp);
    std::fseek(fp, 0, SEEK_END);
    auto fileSize = std::ftell(fp);
    std::fseek(fp, pos, SEEK_SET);
    return fileSize;
  }
  template<class T>
  auto read(T& v)
  {
    return std::fread(&v, sizeof(T), 1, fp);
  }
  template<class T>
  auto read(T* p, size_t sz)
  {
    return std::fread(p, sizeof(T), sz, fp);
  }
  template<class T>
  auto read(std::vector<T>& v)
  {
    return std::fread(v.data(), sizeof(T), v.size(), fp);
  }
  template<class T>
  auto write(T* p, size_t sz)
  {
    return std::fwrite(p, sizeof(T), sz, fp);
  }
  template<class T>
  auto write(const std::vector<T>& v)
  {
    return std::fwrite(v.data(), sizeof(T), v.size(), fp);
  }
};

// Helper for std::vector<NoInit<T>>
template<class T>
class NoInit {
public:
  NoInit() {}
  constexpr  NoInit(T v) : value(v) {}
  constexpr  operator T () const { return value; }
private:
  T value;
};

// Double buffered read.
// Main thread reads from buf. Additional thread is to load bufs from the file.
template<class T>
class FileReadBuf
{
public:

  FileReadBuf(const std::string& name, size_t sz = 256 * 1024) :
    file(name, "rb"s),
    t(&FileReadBuf::run, this),
    isEOF(false)
  {
    buf.reserve(sz);
    buf2.reserve(sz);
    setLoad(true);
  }

  FileReadBuf(const FileReadBuf&) {}

  ~FileReadBuf()
  {
    {
      std::lock_guard<std::mutex> lock(m);
      isEOF = true;
    }
    cv.notify_one();
    t.join();
  }

  size_t size() { return file.size(); }

  bool read(T& x)
  {
    if (bufpos >= buf.size()) {
      if (isEOF)
        return false; // early exit
      buf.clear();
      bufpos = 0;
      Timer timer;
      std::unique_lock<std::mutex> lock(m);
      cv.wait(lock, [this] { return !doLoad; }); // continue to swap bufs when load is finished and flag is down
      if (isEOF)
        return false;
      std::swap(buf, buf2);
      lock.unlock();
      setLoad(true);
      mainThreadWaits += timer;
    }
    x = buf[bufpos++];
    return true;
  }

  auto mainWaits() const { return mainThreadWaits; }

private:
  void setLoad(bool b)
  {
    {
      std::lock_guard<std::mutex> lock(m);
      doLoad = b;
    }
    cv.notify_one();
  }
  void run()
  {
    while (true)
    {
      std::unique_lock<std::mutex> lock(m);
      cv.wait(lock, [this] { return doLoad || isEOF; }); // continue to load when buf is ready and flag is raised
      lock.unlock();
      if (isEOF)
        break;
      buf2.resize(buf2.capacity());
      buf2.resize(file.read(buf2));
      if (buf2.size() == 0)
        isEOF = true;
      setLoad(false);
    }
  }

  File file;

  std::thread t;
  std::mutex m;
  std::condition_variable cv;
  bool doLoad = false; // false: loader awaits, reader can swap bufs if needs; true: loader can work, reader already swapped bufs and shall wait.
  std::atomic<bool> isEOF;
  double mainThreadWaits = 0.0;

  size_t bufpos = 0;
  std::vector<NoInit<T>> buf;
  std::vector<NoInit<T>> buf2;
};
