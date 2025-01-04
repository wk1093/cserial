#pragma once
#ifndef DTC_DYNTYPC_H
#define DTC_DYNTYPC_H
#include "Type.h"
#include "Variable.h"
#include "VarMath.h"

void printType(const Type& t);

Variable sanitizePointers(Context& ctx, Variable v);

template<typename T>
T primitive(Context& ctx, Variable v, size_t index=0) {
    v = sanitizePointers(ctx, v);

    if (sizeof(T) != t_sizeof(v.type)) {
        printType(v.type);
        std::cout << std::endl;
        throw std::runtime_error("primitive type size mismatch");
    }
    // if the data is stored as a value, we can just cast it
    T res = *(T*)v.getdata(index);
    return res;


//    if (v.type.deref_count == 0) {
//    } else {
//        // we want to get the data as a pointer, because that is what it is
//        // so we must allocate memory for the data, and put it in
//        Type t = v.type;
//        t.deref_count--;
//        uint64_t size = t_sizeof(t);
//        void* ptr = malloc(size); // TODO: free this memory in a cleanup function somewhere
//        if (ptr == nullptr) {
//            throw std::runtime_error("malloc failed");
//        }
//        memcpy(ptr, v.getdata(index), size);
//        // now we have the data as a pointer, so we can cast it to the desired type
//        T res = *(T*)ptr;
//        return res;
//    }
}

void printVariable(Context& ctx, Variable v, bool done=true);

#endif //DTC_DYNTYPC_H