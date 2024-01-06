#pragma once
#ifndef DTC_DYNTYPC_H
#define DTC_DYNTYPC_H
#include "Type.h"
#include "Variable.h"
#include "VarMath.h"

void printType(const Type& t);

template<typename T>
T primitive(Variable v, size_t index=0) {
    // if template type size is not the same as the variable type size
    // err
    if (sizeof(T) != t_sizeof(v.type)) {
        printType(v.type);
        std::cout << std::endl;
        throw std::runtime_error("primitive type size mismatch");
    }
    return *(T*)(v.getdata(index));
}

void printVariable(Variable v, bool done=true);

#endif //DTC_DYNTYPC_H