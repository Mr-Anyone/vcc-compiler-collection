#ifndef CORE_RTTI_H
#define CORE_RTTI_H

namespace vcc{
template <typename T, typename U>
bool isa(U a)
{
    if (dynamic_cast<T*>(a))
        return true;

    return false;
}

template <typename T, typename U>
T* dyncast(U a)
{
    if (T* casted = dynamic_cast<T*>(a))
        return casted;

    return nullptr;
}
};

#endif
