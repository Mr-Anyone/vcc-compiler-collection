#include "core/util.h"

using namespace vcc;

[[noreturn]] void vcc::assert_failed(const char* message, const char* file, int line)
{
    std::cerr << "assertion: " << message << " at: " << file << " line: " << line << "\n"
              << std::flush;
    std::exit(-1);
}
