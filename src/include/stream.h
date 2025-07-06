#ifndef STREAM_H
#define STREAM_H

#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

struct FilePos {
  int row, col;
  
  friend std::ostream& operator<<(std::ostream& os, const FilePos& filepos);
};

std::ostream& operator<<(std::ostream& os, const FilePos& filepos);

bool operator==(const FilePos &lhs, const FilePos &rhs);

/// abstractions above std::ifstream
class FileStream {
public:
  FileStream(const char* filename);

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

  const std::vector<long>& getCurrentNewLineBuf() const;
private:
  void addNewLineLoc(long loc);

  /// check if we are at the end of file
  bool m_is_end_of_file = false;
  
  /// check for if a file is open or not
  bool m_open;

  // save and restore state
  long m_restore_loc; 
  FilePos m_restore_pos;
  bool m_is_in_save_state = false;
  void saveState();
  void restoreState();

  /// stores the new line locations
  /// {10,  12}, at the 10th bytes, there is a '\n', meaning that 11 is a
  /// character
  std::vector<long> m_new_line_loc{};
  std::FILE *m_file;
  FilePos m_pos = {1, 1};
};

#endif
