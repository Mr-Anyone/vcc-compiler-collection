#ifndef ADT_ASSERT_H
#define ADT_ASSERT_H

namespace vcc
{
[[noreturn]] void assert_failed(const char* message, const char* file, int line);
};

// We use VCC_ASSERT instead of the regular assert because we want assertion
// to be enabled in release build as well
#define VCC_ASSERT(expr)                                                                 \
    (static_cast<bool>(expr)) ? (void)0 : vcc::assert_failed(#expr, __FILE__, __LINE__)

// Better style than assert(false)
#define VCC_UNREACHABLE(message) VCC_ASSERT(false && #message)

#endif
