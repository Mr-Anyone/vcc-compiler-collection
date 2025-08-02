#ifndef DRIVER_H
#define DRIVER_H

namespace vcc {
class Parser;
Parser parseFile(const char *path_to_file);
}; // namespace vcc

#endif
