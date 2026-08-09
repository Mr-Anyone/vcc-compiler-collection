#ifndef ADT_OPTIONAL_H
#define ADT_OPTIONAL_H

#include <cassert>

#include "adt/assert.h"

/// Optional is the same as std::optional with the difference being
/// naming convention. I like Camel case so hence the creation of this class
template <typename T>
class Optional
{
   public:
    Optional(T val) : m_is_empty(false), m_value(val) {}

    Optional() : m_is_empty(true), m_value() {}

    T value()
    {
        VCC_ASSERT((!this->m_is_empty && "Option<T>::value() must not be empty"));
        return m_value;
    }

    bool isEmtpy()
    {
        return m_is_empty;
    }

   private:
    bool m_is_empty;
    T m_value;
};

#endif
