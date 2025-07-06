#include "stream.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>

FileStream::FileStream(const char* filename) {
  m_file = std::fopen(filename, "rb");
  m_open = true;

  if (!m_file)
    m_open = false;
}

char FileStream::get() {
  char c;
  const int count = std::fread(&c, sizeof(char), 1, m_file);

  // setting the end of file state
  m_is_end_of_file = false;
  if(count == 0){
      assert(!ferror(m_file) && "not sure how to handle this case");
      if(std::feof(m_file))
          m_is_end_of_file = true;

      return 0;
  }

  if (c == '\n') {
    // -1 from the side effect of fread
    addNewLineLoc(tellg() - 1);

    m_pos.row++;
    m_pos.col = 1;
  } else {
    m_pos.col++;
  }

  return c;
}

char FileStream::peek() {
  saveState();
  char c = get();
  restoreState();
  return c;
}

char FileStream::get(char &c) {
  c = get();
  return c;
}

bool FileStream::good() { return std::ferror(m_file) == 0; }

long FileStream::tellg() {
  // the byte offset
  return std::ftell(m_file);
}

void FileStream::seekg(long pos) { std::fseek(m_file, pos, SEEK_SET); }

bool FileStream::eof() {
    return m_is_end_of_file;
}

void FileStream::saveState() {
  assert(!m_is_in_save_state);
  m_restore_loc = tellg();
  m_restore_pos = m_pos;
  m_is_in_save_state = true;
}

void FileStream::restoreState() {
  assert(m_is_in_save_state);

  seekg(m_restore_loc);
  m_pos = m_restore_pos;
  m_is_in_save_state = false;
}

FilePos FileStream::getPos() { return m_pos; }

bool operator==(const FilePos &lhs, const FilePos &rhs) {
  return lhs.col == rhs.col && lhs.row == rhs.row;
}

std::ostream& operator<<(std::ostream& os, const FilePos& pos){
    os <<  "row: " << pos.row << " col: " << pos.col;
    return os;
}

bool FileStream::is_open() {return m_open;}

void FileStream::addNewLineLoc(long loc) {
  if (m_new_line_loc.empty()) {
    m_new_line_loc.push_back(loc);
    return;
  }

  // making sure of the monotonically increasing property
  long end_loc = m_new_line_loc[m_new_line_loc.size() - 1];
  if (loc > end_loc) {
    m_new_line_loc.push_back(loc);
  }
}

const std::vector<long> &FileStream::getCurrentNewLineBuf() const {
  return m_new_line_loc;
}
