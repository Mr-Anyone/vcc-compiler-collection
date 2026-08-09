#ifndef CORE_STREAM_H
#define CORE_STREAM_H

#include <iostream>
#include <string>

namespace vcc
{
struct FilePos
{
    FilePos(int row, int col, long loc) : row(row), col(col), loc(loc) {}
    int row, col;
    long loc;

    friend std::ostream& operator<<(std::ostream& os, const FilePos& filepos);
};

std::ostream& operator<<(std::ostream& os, const FilePos& filepos);

bool operator==(const FilePos& lhs, const FilePos& rhs);

class FileStream
{
   public:
    FileStream(const char* filename);

    /// Remove copy constructor, because this is unsafe
    FileStream(const FileStream& other)            = delete;
    FileStream& operator=(const FileStream& other) = delete;

    /// consumes the character
    char get();
    char get(char& c);

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

    std::string getLine(long pos);

   private:
    void setAtEOF();

    /// The content of the file
    std::string m_content;

    /// check if we are at the end of file
    bool m_is_end_of_file = false;

    // keeping track of locations
    long m_current_loc, m_restore_loc;
    FilePos m_pos = {1, 1, 0}, m_restore_pos = m_pos;

    // keeping track of save and restore states
    bool m_is_in_save_state = false;
    void saveState();
    void restoreState();
};

};  // namespace vcc
#endif
