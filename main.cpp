#ifndef _MSC_VER
#include <signal.h>
#endif

#include <iostream>
#include <filesystem>
#include <array>
//#include <unordered_map>
#include "skarupke/bytell_hash_map.hpp" // https://github.com/skarupke/flat_hash_map
#include <algorithm>
#include <fstream>
#include "file.hpp"
#include "timer.hpp"

inline void debugBreak()
{
#ifdef _MSC_VER
	__debugbreak();
#else
	raise(SIGTRAP);
#endif
}

using CharTable = std::array<unsigned char, 256>;

// Make a table to project chars to letters and spaces.
constexpr CharTable initProjection()
{
  CharTable proj{};
  for (unsigned i = 0; i < proj.size(); i++)
    if ('A' <= i && i <= 'Z')
      proj[i] = i + ('a' - 'A');
    else if ('a' <= i && i <= 'z')
      proj[i] = i;
    else
      proj[i] = ' ';
  return proj;
}

void makeDict(const std::string& in, const std::string& out)
{
  Timer timer1;

  // this version still does not work on wins, because it shall open utf-8 txt to analize

  FileReadBuf<unsigned char> f(in, 512 * 1024);

  std::cout << "Load file: " << timer1 << "sec\n";
  timer1.reset();

  static constexpr auto proj = initProjection();

  //std::unordered_map<std::string, unsigned> dict;
  ska::bytell_hash_map<std::string, unsigned> dict;

  std::string word;
  unsigned char c0 = 0;
  while(f.read(c0)) {
    const auto c = proj[c0];
    if (c == ' ') {
      if (word.empty())
        continue;
      ++dict[word];
      word.clear();
    }
    else {
      word += c;
    }
  }
  if (word.size())
    ++dict[word];

  std::cout << "Spent in loading: " << f.mainWaits() << "sec\n";
  std::cout << "Fill dict: " << timer1 << "sec\n";
  timer1.reset();

  std::vector<std::pair<unsigned, std::string_view>> frequencies;
  frequencies.reserve(dict.size());
  for (const auto& kv : dict) {
    frequencies.emplace_back(kv.second, kv.first);
  }
  std::sort(frequencies.begin(), frequencies.end(), [](auto& a, auto& b) -> bool
    {
      return a.first > b.first || a.first == b.first && a.second < b.second;
    }
  );

  std::cout << "Sort frequencies: " << timer1 << "sec\n";
  timer1.reset();

  std::ofstream f2(out);
  for (const auto& fw : frequencies)
    f2 << fw.first << " " << fw.second << "\n";
  f2.close();

  std::cout << "Write output: " << timer1 << "sec\n";
}

int main(int argc, const char* argv[])
{
#ifdef _DEBUG
  debugBreak();
#endif

  Timer timer0;

  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <in.txt> <out.txt>\n";
    return 1;
  }
  if (!std::filesystem::is_regular_file(argv[1])) {
    std::cout << "Input file not found: " << argv[1] << "\n";
    return 1;
  }
  std::error_code ec;
  if (std::filesystem::exists(argv[2], ec)) {
    if (!std::filesystem::remove(argv[2], ec)) {
      std::cout << "Cannot remove output file: " << argv[1] << "\nError: " << ec.message() << "\n";
      return 1;
    }
  }

  makeDict(argv[1], argv[2]);
  std::cout << "Total time: " << timer0 << "sec\n";

  return 0;
}