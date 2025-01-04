#pragma once
#ifndef DTC_SERIAL_H_
#define DTC_SERIAL_H_

#include <iostream>
#include <utility>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <variant>
#include <algorithm>
#include "DynTypC.h"

// FORMAT SPECS
// little endian
// Type:
//  - deref_count: u64
//  - typetype: u8 (0 for basic, 1 for struct)
//  - data for the respective type

// BasicType:
//  - flags: u8 (0 for unsigned int, 1 for signed int, 2 for float)
//  - bytes: u64

// StructType:
//  - num_types: u64
//  - types: Type[num_types]

// Example:

// struct {
//     int a;
//     int b;
//     struct {
//         int c;
//         char* d;
//     } e;
// } f;
// ->
// Type:
// - deref_count: 0
// - typetype: 1
// - StructType:
//    - num_types: 3
//    - types: [
//        Type:
//        - deref_count: 0
//        - typetype: 0
//        - BasicType:
//           - flags: 1
//           - bytes: 4
//        Type:
//        - deref_count: 0
//        - typetype: 0
//        - BasicType:
//           - flags: 1
//           - bytes: 4
//        Type:
//        - deref_count: 0
//        - typetype: 1
//        - StructType:
//           - num_types: 2
//           - types: [
//               Type:
//               - deref_count: 0
//               - typetype: 0
//               - BasicType:
//                  - flags: 1
//                  - bytes: 4
//               Type:
//               - deref_count: 1
//               - typetype: 0
//               - BasicType:
//                  - flags: 0
//                  - bytes: 0
//           ]
//    ]

// to make it easier
struct ByteStream {
    std::vector<std::byte> bytes;

    // basically just a wrapper around std::vector<std::byte>
    // mostly the same, but with like 1 extra functions for appending a vector of bytes easily

    ByteStream() = default;
    explicit ByteStream(std::vector<std::byte> bytes);
    ByteStream(std::initializer_list<std::byte> bytes);

    void append(std::vector<std::byte> bytes_in);

    void append(std::initializer_list<std::byte> bytes_in);

    void append(ByteStream& other);

    void append(std::byte byte);

    // reading functions

    std::byte read_byte();

    uint8_t read_u8();

    std::vector<std::byte> read_bytes(size_t count);
};

std::vector<std::byte> serialize_u64(uint64_t u64);

std::vector<std::byte> serialize_basic_type(BasicType t);

std::vector<std::byte> serialize_type(const Type& t);

std::vector<std::byte> serialize_struct_type(const StructType& t);

std::vector<std::byte> serialize_type(const Type& t);

std::vector<std::byte> serialize_variable(const Variable& v);

uint64_t deserialize_u64(ByteStream& bytes);

BasicType deserialize_basic_type(ByteStream& bytes);

Type deserialize_type(ByteStream& bytes);

StructType deserialize_struct_type(ByteStream& bytes);

Type deserialize_type(ByteStream& bytes);

Variable deserialize_variable(ByteStream& bytes);

template<typename T>
T deserialize(Context& ctx, std::vector<std::byte> bytes) {
    ByteStream stream(std::move(bytes));
    return primitive<T>(ctx, deserialize_variable(stream));
}

template<typename T>
std::vector<std::byte> serialize(Context& ctx, T val, const Type &t) {
    return serialize_variable(construct_variable(ctx, val, t));
}

template<typename T>
Variable construct_variable(Context& ctx, T val, const Type &t) {
    std::vector<std::byte> bytes;
    uint64_t size = sizeof(T);
    // bytes of the value
    bytes.resize(size);
    std::memcpy(bytes.data(), &val, size);
    Variable v = Variable(ctx, t, bytes);
    return v;
}

typedef std::vector<std::byte> Bytes;

template<typename T>
Bytes final_serialize(T val, const Type &t) {
    Context ctx;
    Bytes b = serialize(ctx, val, t);
    // format: serialized size, serialized data, context
    Bytes final;
    final = serialize_u64(b.size());
    final.insert(final.end(), b.begin(), b.end());
    final.insert(final.end(), ctx.begin(), ctx.end());
    return final;
}

template<typename T>
struct Deserialized {
    T val;
    Context* ctx;
};

template<typename T>
Deserialized<T> final_deserialize(Bytes bytes) {
    ByteStream stream(bytes);
    uint64_t size = deserialize_u64(stream);
    Bytes b = Bytes(bytes.begin() + 8, bytes.begin() + 8 + size);
    Bytes ctx_bytes(bytes.begin() + 8 + size, bytes.end());
    // when we make this context, it must outlive this function, because all the data returned will point to the context
    Context* ctx = new Context();
    ctx->resize(ctx_bytes.size());
    std::copy(ctx_bytes.begin(), ctx_bytes.end(), ctx->begin());
    return Deserialized<T>{deserialize<T>(*ctx, b), ctx};
}


#endif  // DTC_SERIAL_H_