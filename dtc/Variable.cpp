#include "Variable.h"


Variable::Variable(const Type &t, std::vector<std::byte> data) {
    type = t;
    if (type.deref_count > 0) {
        // we assume the data has already been converted to a pointer into virtual ram
        this->data = std::move(data);
        return;
    }
    if (type.is_basic()) {
        this->data = std::move(data);
        return;
    }
    // we need to iterate for structs, because all sub-data should be Variables, not just bytes
    // this allows sub-structs and pointers in those to be created properly
    if (auto s = std::get_if<StructType>(&type.type)) {
        size_t offset = 0;
        for (auto &t: s->types) {
            size_t size = t_sizeof(t);
            std::vector<std::byte> subdata(data.begin() + offset, data.begin() + offset + size);
            offset += size;
            Variable v(t, subdata);
            this->data.insert(this->data.end(), v.data.begin(), v.data.end());
        }
    }
}

Variable::Variable(Context& ctx, const Type &t, std::vector<std::byte> data) {
    type = t;
    if (type.deref_count == 0) {
        if (type.is_basic()) {
            this->data = std::move(data);
            return;
        }
        if (auto s = std::get_if<StructType>(&type.type)) {
            size_t offset = 0;
            for (auto &t: s->types) {
                size_t size = t_sizeof(t);
                std::vector<std::byte> subdata(data.begin() + offset, data.begin() + offset + size);
                offset += size;
                Variable v(ctx, t, subdata);
                this->data.insert(this->data.end(), v.data.begin(), v.data.end());
            }
        }
        return;
    }
    uint64_t ptr = 0;
    for (size_t i = 0; i < sizeof(void*); i++) {
        ptr |= std::to_integer<uint64_t>(data[i]) << (i * 8);
    }
    Type t2 = t;
    t2.deref_count--;
    Variable v = new_ptr(reinterpret_cast<void*>(ptr), ctx, t2);
    this->data = v.data;

}

bool Variable::is_basic() const {
    return std::holds_alternative<BasicType>(type.type);
}

Variable::Variable(int8_t a) {
    *this = new_i8(a);
}
Variable::Variable(uint8_t a) {
    *this = new_u8(a);
}
Variable::Variable(int16_t a) {
    *this = new_i16(a);
}
Variable::Variable(uint16_t a) {
    *this = new_u16(a);
}
Variable::Variable(int32_t a) {
    *this = new_i32(a);
}
Variable::Variable(uint32_t a) {
    *this = new_u32(a);
}
Variable::Variable(int64_t a) {
    *this = new_i64(a);
}
Variable::Variable(uint64_t a) {
    *this = new_u64(a);
}
Variable::Variable(float a) {
    *this = new_f32(a);
}
Variable::Variable(double a) {
    *this = new_f64(a);
}
Variable::Variable(void* a, Context& ctx, Type t) {
    *this = new_ptr(a, ctx, t);
}
Variable::Variable(void* a, Context& ctx, uint64_t size) {
    *this = new_ptr(a, ctx, size);
}
Variable::Variable(Variable* a, Context& ctx) {
    *this = new_varptr(a, ctx);
}
Variable::Variable(std::vector<Variable> vars) {
    *this = new_struct(std::move(vars));
}
Variable::Variable(std::initializer_list<Variable> vars) {
    *this = new_struct(std::vector<Variable>(vars));
}


std::byte* Variable::getdata(size_t i) {
    // if not a struct type, just return the data
    // if a struct type, return the data of the ith element in struct
    // if i is out of bounds, throw error
    if (auto s = std::get_if<StructType>(&type.type)) {
        if (i >= s->types.size()) {
            throw std::runtime_error("Index out of bounds");
        }
        size_t offset = 0;
        for (size_t j = 0; j < i; j++) {
            offset += t_sizeof(s->types[j]);
        }
        return data.data() + offset;
    }
    return data.data();
}

Variable Variable::getsub(size_t i) {
    if (auto s = std::get_if<StructType>(&type.type)) {
        if (i >= s->types.size()) {
            throw std::runtime_error("Index out of bounds");
        }
        size_t offset = 0;
        for (size_t j = 0; j < i; j++) {
            offset += t_sizeof(s->types[j]);
        }
        return Variable{s->types[i], std::vector<std::byte>(data.begin() + offset, data.begin() + offset + t_sizeof(s->types[i]))};
    }
    return *this;
}

Variable Variable::getsub(Context& ctx, size_t i) {
    if (auto s = std::get_if<StructType>(&type.type)) {
        if (i >= s->types.size()) {
            throw std::runtime_error("Index out of bounds");
        }
        size_t offset = 0;
        for (size_t j = 0; j < i; j++) {
            offset += t_sizeof(s->types[j]);
        }
        return Variable{s->types[i], std::vector<std::byte>(data.begin() + offset, data.begin() + offset + t_sizeof(s->types[i]))};
    }
    return *this;
}

Variable new_i8(int8_t val) {
    Variable v;
    v.type = t_i8;
    v.data = std::vector<std::byte>(1);
    v.data[0] = std::byte(val);
    return v;
}

Variable new_u8(uint8_t val) {
    Variable v;
    v.type = t_u8;
    v.data = std::vector<std::byte>(1);
    v.data[0] = std::byte(val);
    return v;
}

Variable new_i16(int16_t val) {
    Variable v;
    v.type = t_i16;
    v.data = std::vector<std::byte>(2);
    v.data[0] = std::byte(val);
    v.data[1] = std::byte(val >> 8);
    return v;
}

Variable new_u16(uint16_t val) {
    Variable v;
    v.type = t_u16;
    v.data = std::vector<std::byte>(2);
    v.data[0] = std::byte(val);
    v.data[1] = std::byte(val >> 8);
    return v;
}

Variable new_i32(int32_t val) {
    Variable v;
    v.type = t_i32;
    v.data = std::vector<std::byte>(4);
    v.data[0] = std::byte(val);
    v.data[1] = std::byte(val >> 8);
    v.data[2] = std::byte(val >> 16);
    v.data[3] = std::byte(val >> 24);
    return v;
}

Variable new_u32(uint32_t val) {
    Variable v;
    v.type = t_u32;
    v.data = std::vector<std::byte>(4);
    v.data[0] = std::byte(val);
    v.data[1] = std::byte(val >> 8);
    v.data[2] = std::byte(val >> 16);
    v.data[3] = std::byte(val >> 24);
    return v;
}

Variable new_i64(int64_t val) {
    Variable v;
    v.type = t_i64;
    v.data = std::vector<std::byte>(8);
    v.data[0] = std::byte(val);
    v.data[1] = std::byte(val >> 8);
    v.data[2] = std::byte(val >> 16);
    v.data[3] = std::byte(val >> 24);
    v.data[4] = std::byte(val >> 32);
    v.data[5] = std::byte(val >> 40);
    v.data[6] = std::byte(val >> 48);
    v.data[7] = std::byte(val >> 56);
    return v;
}

Variable new_u64(uint64_t val) {
    Variable v;
    v.type = t_u64;
    v.data = std::vector<std::byte>(8);
    v.data[0] = std::byte(val);
    v.data[1] = std::byte(val >> 8);
    v.data[2] = std::byte(val >> 16);
    v.data[3] = std::byte(val >> 24);
    v.data[4] = std::byte(val >> 32);
    v.data[5] = std::byte(val >> 40);
    v.data[6] = std::byte(val >> 48);
    v.data[7] = std::byte(val >> 56);
    return v;
}

Variable new_f32(float val) {
    Variable v;
    v.type = t_f32;
    v.data = std::vector<std::byte>(4);
    uint32_t i = *reinterpret_cast<uint32_t*>(&val);
    v.data[0] = std::byte(i);
    v.data[1] = std::byte(i >> 8);
    v.data[2] = std::byte(i >> 16);
    v.data[3] = std::byte(i >> 24);
    return v;
}

Variable new_f64(double val) {
    Variable v;
    v.type = t_f64;
    v.data = std::vector<std::byte>(8);
    uint64_t i = *reinterpret_cast<uint64_t*>(&val);
    v.data[0] = std::byte(i);
    v.data[1] = std::byte(i >> 8);
    v.data[2] = std::byte(i >> 16);
    v.data[3] = std::byte(i >> 24);
    v.data[4] = std::byte(i >> 32);
    v.data[5] = std::byte(i >> 40);
    v.data[6] = std::byte(i >> 48);
    v.data[7] = std::byte(i >> 56);
    return v;
}

Variable new_bool(bool val) {
    Variable v;
    v.type = t_bool;
    v.data = std::vector<std::byte>(1);
    v.data[0] = std::byte(val);
    return v;
}

Variable new_struct(std::vector<Variable> vars) {
    std::vector<std::byte> data{};
    std::vector<Type> types{};
    types.reserve(vars.size());
    for (auto& v : vars) {
        data.insert(data.end(), v.data.begin(), v.data.end());
        types.push_back(v.type);
    }
    return Variable{new_struct_type(types), data};
}

Variable new_ptr(void *p, Context &ctx, const Type& t) {
    size_t arrlen = 1;
    if (t.is_basic() && std::get<BasicType>(t.type).bytes == 1 && !std::get<BasicType>(t.type).sign) {
        // we assume it's a null-terminated string
        char* c = reinterpret_cast<char*>(p);
        arrlen = 1;
        while (*c != '\0') {
            arrlen++;
            c++;
        }
    }
    uint64_t size = t_sizeof(t)*arrlen;
    size_t index = ctx.size();
    ctx.resize(index + size);
    std::memcpy(ctx.data() + index, p, size);
    Variable v;
    v.type = t;
    v.data = std::vector<std::byte>(sizeof(void*));
    uint64_t i = index;
    for (size_t j = 0; j < sizeof(void*); j++) {
        v.data[j] = std::byte(i >> (j * 8));
    }
    v.type.deref_count++;

    return v;
}
Variable new_ptr(void *p, Context &ctx, uint64_t size) {
    size_t index = ctx.size();
    ctx.resize(index + size);
    std::memcpy(ctx.data() + index, p, size);
    Variable v;
    v.type = t_voidptr;
    v.data = std::vector<std::byte>(sizeof(void *));
    uint64_t i = index;
    for (size_t j = 0; j < sizeof(void *); j++) {
        v.data[j] = std::byte(i >> (j * 8));
    }
    return v;
}
Variable new_varptr(Variable *p, Context &ctx) {
    Variable v;
    v.type = p->type;
    v.type.deref_count++;
    v.data = std::vector<std::byte>(sizeof(void *));
    size_t index = ctx.size();
    uint64_t size = p->data.size();
    ctx.resize(index + size);
    std::memcpy(ctx.data() + index, p->data.data(), size);
    uint64_t i = index;
    for (size_t j = 0; j < sizeof(void *); j++) {
        v.data[j] = std::byte(i >> (j * 8));
    }
    return v;
}
