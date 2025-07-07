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
    m_pos.row++;
    m_pos.col = 1;
  } else {
    m_pos.col++;
  }
  m_pos.loc++;

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

void FileStream::seekg(long pos) { 
    // update the current position
    std::fseek(m_file, 0, SEEK_SET); 
    FilePos new_pos (1, 1, pos);
    for(int i = 0;i<pos;++i){
        char c = std::fgetc(m_file);
        if(c == '\n'){
            new_pos.row += 1;
            new_pos.col = 0;
        }else{
            new_pos.col++;
        }
    }
    
    // update the position
    m_pos = new_pos;
    assert(tellg() == pos && "must be true if we have seekg");
}

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

std::string FileStream::getLine(long pos){
    saveState();
    long begin_line_start = -1;
    std::fseek(m_file, 0, SEEK_SET);
    for(int i = 0;i<pos;++i){
        char c = std::fgetc(m_file);
        if(c == '\n')
            begin_line_start = i;
    }
    // the loop above returns the last location of a '\n' 
    // adding one gives the new line
    begin_line_start += 1; // 

    std::fseek(m_file, begin_line_start, SEEK_SET);
    std::string line = "";
    char c;
    while((c = std::fgetc(m_file)) != EOF){
        if (c == '\n')
            break; 
        else 
            line += c;
    }

    restoreState();
    return line;
} 
