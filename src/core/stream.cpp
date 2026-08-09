#include "core/stream.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>

#include "core/util.h"

using namespace vcc;

static std::optional<std::string> readFile(std::string path)
{
    std::ifstream file(path);
    std::stringstream buf;

    buf << file.rdbuf();
    return buf.str();
}

bool FileStream::good(){
    /// FIXME: the constructor returns a std::exit when the file stream is
    /// no good. If we have constructed this class, it means that the 
    /// Filestream is good. We should deprecate this class
    return true;
}
FileStream::FileStream(const char* filename)
    : m_is_end_of_file(false),
      m_current_loc(0),
      m_restore_loc(0),
      m_restore_pos(1, 1, 0),
      m_is_in_save_state(false)
{
    std::optional<std::string> maybe_content = readFile(filename);
    if (!maybe_content.has_value())
    {
        std::cerr << "cannot read file: " << filename << "\n";
        std::exit(-1);
    }

    m_content = maybe_content.value();
}

char FileStream::get()
{
    // return 0 if we are at the end of file
    m_is_end_of_file = false;
    if (m_current_loc == m_content.size())
    {
        m_is_end_of_file = true;
        return 0;
    }

    char c = m_content[m_current_loc];
    m_current_loc += 1;
    if (c == '\n')
    {
        // -1 from the side effect of fread
        m_pos.row++;
        m_pos.col = 1;
    }
    else
    {
        m_pos.col++;
    }
    m_pos.loc = m_current_loc;

    return c;
}

char FileStream::peek()
{
    saveState();
    char c = get();
    restoreState();
    return c;
}

char FileStream::get(char& c)
{
    c = get();
    return c;
}

long FileStream::tellg()
{
    // the byte offset
    return m_current_loc;
}

void FileStream::seekg(long pos)
{
    VCC_ASSERT("pos must be less than the size of the file" && pos <= m_content.size());

    // special case: We have to handle EOF. Currently, EOF and the last character of
    // the file shares the FilePos row and column
    int iterate_count = (pos == m_content.size()) ? pos - 1 : pos;

    // update the current position
    FilePos new_pos(1, 0, pos);
    for (int i = 0; i <= iterate_count; ++i)
    {
        char c = m_content[i];
        if (c == '\n')
        {
            new_pos.row += 1;
            new_pos.col = 1;
        }
        else
        {
            new_pos.col++;
        }
    }

    // update the position
    m_current_loc = pos;
    m_pos = new_pos;
    VCC_ASSERT(tellg() == pos && "must be true if we have seekg");
}

bool FileStream::eof()
{
    return m_is_end_of_file;
}

void FileStream::saveState()
{
    VCC_ASSERT(!m_is_in_save_state);
    m_restore_loc      = tellg();
    m_restore_pos      = m_pos;
    m_is_in_save_state = true;
}

void FileStream::restoreState()
{
    VCC_ASSERT(m_is_in_save_state);

    seekg(m_restore_loc);
    m_pos              = m_restore_pos;
    m_is_in_save_state = false;
}

FilePos FileStream::getPos()
{
    return m_pos;
}

bool vcc::operator==(const FilePos& lhs, const FilePos& rhs)
{
    return lhs.col == rhs.col && lhs.row == rhs.row;
}

std::ostream& vcc::operator<<(std::ostream& os, const FilePos& pos)
{
    os << "row: " << pos.row << " col: " << pos.col;
    return os;
}

std::string FileStream::getLine(long pos)
{
    long begin_line_start = -1;
    for (int i = 0; i < pos; ++i)
    {
        char c = m_content[i];
        if (c == '\n')
            begin_line_start = i;
    }
    // the loop above returns the last location of a '\n'
    // adding one gives the new line
    begin_line_start += 1;

    std::string line = "";
    char c;
    while (begin_line_start < m_content.size() && m_content[begin_line_start] != '\n')
    {
        c = m_content[begin_line_start];
        begin_line_start++;

        line += c;
    }
    return line;
}
