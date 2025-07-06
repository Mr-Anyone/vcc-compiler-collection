#ifndef SEMA_H
#define SEMA_H

#include <string>

class Sema {
public:
    Sema();

private:
    friend class Parser;

    void checkLocalFunctionName(const std::string& name); 
};

#endif
