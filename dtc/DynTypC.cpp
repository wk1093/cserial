#include "DynTypC.h"

void printType(const Type &t) {
    // if basicType
    if (auto b = std::get_if<BasicType>(&t.type)) {
        if (b->floating) {
            std::cout << std::dec <<  "f" << (b->bytes * 8);
        } else {
            std::cout << std::dec << (b->sign ? "i" : "u") << (b->bytes * 8);
        }
    } else if (auto s = std::get_if<StructType>(&t.type)) {
        std::cout << "struct {";
        for (size_t i = 0; i < s->types.size(); i++) {
            printType(s->types[i]);
            if (i != s->types.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "}";
    }
    for (uint64_t i = 0; i < t.deref_count; i++) {
        std::cout << "*";
    }
}

void printVariable(Context& ctx, Variable v, bool done) {
    if (v.type.deref_count > 0) {
        if (v.data.size() != sizeof(void*)) {
            throw std::runtime_error("pointer size mismatch");
        }
        printType(v.type);
        std::cout << "(0x" << std::hex << *(void**)v.data.data() << ")";
        if (done) {
            std::cout << std::endl;
        }
        return;
    }
    if (v.is_basic()) {
        printType(v.type);
        std::cout << "(";
        if (auto s = std::get_if<BasicType>(&v.type.type)) {
            if (s->floating) {
                if (s->bytes == 4) {
                    std::cout << std::dec << primitive<f32>(ctx, v);
                } else if (s->bytes == 8) {
                    std::cout << std::dec << primitive<f64>(ctx, v);
                }
            } else {
                if (s->sign) {
                    if (s->bytes == 1) {
                        std::cout << (int)primitive<i8>(ctx, v);
                    } else if (s->bytes == 2) {
                        std::cout << primitive<i16>(ctx, v);
                    } else if (s->bytes == 4) {
                        std::cout << primitive<i32>(ctx, v);
                    } else if (s->bytes == 8) {
                        std::cout << primitive<i64>(ctx, v);
                    }
                } else {
                    if (s->bytes == 1) {
                        std::cout << (int)primitive<u8>(ctx, v);
                    } else if (s->bytes == 2) {
                        std::cout << primitive<u16>(ctx, v);
                    } else if (s->bytes == 4) {
                        std::cout << primitive<u32>(ctx, v);
                    } else if (s->bytes == 8) {
                        std::cout << primitive<u64>(ctx, v);
                    }
                }
            }
        } else {
            throw std::runtime_error("variable is not basic type");
        }
        std::cout << ")";
    } else {
        if (auto s = std::get_if<StructType>(&v.type.type)) {
            std::cout << "struct";
            for (uint64_t i = 0; i < v.type.deref_count; i++) {
                std::cout << "*";
            }
            std::cout << " {";
            for (size_t i = 0; i < s->types.size(); i++) {
                Variable sub = v.getsub(i);
                printVariable(ctx, sub, false);
                if (i != s->types.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << "}";
        } else {
            throw std::runtime_error("variable is not struct type");
        }
    }
    if (done) {
        std::cout << std::endl;
    }
}

Variable sanitizePointers(Context& ctx, Variable v) {
    // if the type is a pointer, we must sanitize it by allocating memory for the data and copying it from virtual memory
    // we need to recurse for structs
    if (auto s = std::get_if<StructType>(&v.type.type)) {
        Variable v2 = v;
        v2.data.clear();
        for (size_t i = 0; i < s->types.size(); i++) {
            Variable sub = v.getsub(ctx, i);
            Variable sanitized = sanitizePointers(ctx, sub);
            v2.data.insert(v2.data.end(), sanitized.data.begin(), sanitized.data.end());
        }
        return v2;
    } else if (v.type.deref_count > 0 && !v.type.sanitized) {
        uint64_t ptr = 0;
        for (size_t i = 0; i < sizeof(void*); i++) {
            ptr |= std::to_integer<uint64_t>(v.data[i]) << (i * 8);
        }
        auto ptr2 = (size_t)(ptr+ctx.data());
        v.data.clear();
        v.data.resize(sizeof(void*));
        for (size_t i = 0; i < sizeof(void*); i++) {
            v.data[i] = std::byte((ptr2 >> (i * 8)) & 0xFF);
        }
        v.type.sanitized = true;
        return v;
    } else {
        return v;
    }
}
