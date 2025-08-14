#ifndef CORE_STREAM_H
#define CORE_STREAM_H

#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

namespace vcc {
/// 0 means we are at \n character
/// therefore it is impossible to have row=1, and col=0
struct FilePos {
  FilePos(int row, int col, long loc) : row(row), col(col), loc(loc) {}
  int row, col;
  long loc;

  friend std::ostream &operator<<(std::ostream &os, const FilePos &filepos);
};

std::ostream &operator<<(std::ostream &os, const FilePos &filepos);

bool operator==(const FilePos &lhs, const FilePos &rhs);

/// abstractions above std::ifstream
class FileStream {
public:
  FileStream(const char *filename);

  // Remove copy constructor, because this is unsafe
  FileStream(const FileStream &other) = delete;
  FileStream &operator=(const FileStream &other) = delete;

  /// consumes the character
  char get();
  char get(char &c);

  /// look ahead into the next one
  char peek();

  /// is end of file?
  bool eof();

  /// is there error flags?
  bool good();

  /// tellg returns the current offset
  long tellg();
  void seekg(long pos);

  FilePos getPos();

  /// is the file open?
  bool is_open();

  std::string getLine(long pos);

private:
  /// check if we are at the end of file
  bool m_is_end_of_file = false;

  /// check for if a file is open or not
  bool m_open;

  // save and restore state
  long m_restore_loc;
  FilePos m_restore_pos{1, 1, 0};
  bool m_is_in_save_state = false;
  void saveState();
  void restoreState();

  std::FILE *m_file;
  FilePos m_pos = {1, 1, 0};
};

}; // namespace vcc
#endif
