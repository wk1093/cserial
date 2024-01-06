#include "Serial.h"

ByteStream::ByteStream(std::vector<std::byte> bytes) : bytes(std::move(bytes)) {}

ByteStream::ByteStream(std::initializer_list<std::byte> bytes) : bytes(bytes) {}

void ByteStream::append(std::vector<std::byte> bytes_in) {
    this->bytes.insert(this->bytes.end(), bytes_in.begin(), bytes_in.end());
}

void ByteStream::append(std::initializer_list<std::byte> bytes_in) {
    this->bytes.insert(this->bytes.end(), bytes_in.begin(), bytes_in.end());
}

void ByteStream::append(ByteStream &other) {
    this->bytes.insert(this->bytes.end(), other.bytes.begin(), other.bytes.end());
}

void ByteStream::append(std::byte byte) {
    this->bytes.push_back(byte);
}

std::byte ByteStream::read_byte() {
    std::byte byte = this->bytes[0];
    this->bytes.erase(this->bytes.begin());
    return byte;
}

uint8_t ByteStream::read_u8() {
    auto u8 = std::to_integer<uint8_t>(this->bytes[0]);
    this->bytes.erase(this->bytes.begin());
    return u8;
}

std::vector<std::byte> ByteStream::read_bytes(size_t count) {
    std::vector<std::byte> bytes_ret;
    bytes_ret.reserve(count);
    for (size_t i = 0; i < count; i++) {
        bytes_ret.push_back(this->bytes[0]);
        this->bytes.erase(this->bytes.begin());
    }

    return bytes_ret;
}

std::vector<std::byte> serialize_u64(uint64_t u64) {
    std::vector<std::byte> bytes;
    for (int i = 0; i < 8; i++) {
        bytes.push_back(std::byte(u64 & 0xFF));
        u64 >>= 8;
    }
    return bytes;
}

std::vector<std::byte> serialize_basic_type(BasicType t) {
    ByteStream bytes;
    bytes.append(std::byte(t.floating ? 2 : (t.sign ? 1 : 0)));
    bytes.append(serialize_u64(t.bytes));
    return bytes.bytes;
}

std::vector<std::byte> serialize_struct_type(const StructType& t) {
    ByteStream bytes;
    bytes.append(serialize_u64(t.types.size()));
    for (auto& type : t.types) {
        bytes.append(serialize_type(type));
    }
    return bytes.bytes;
}

std::vector<std::byte> serialize_type(const Type &t) {
    ByteStream bytes;
    bytes.append(serialize_u64(t.deref_count));
    if (t.type.index() == 0) {
        bytes.append(std::byte(0));
        bytes.append(serialize_basic_type(std::get<BasicType>(t.type)));
    } else {
        bytes.append(std::byte(1));
        bytes.append(serialize_struct_type(std::get<StructType>(t.type)));
    }
    return bytes.bytes;
}

StructType deserialize_struct_type(ByteStream &bytes) {
    StructType t;
    uint64_t num_types = deserialize_u64(bytes);
    for (size_t i = 0; i < num_types; i++) {
        t.types.push_back(deserialize_type(bytes));
    }
    return t;
}

Type deserialize_type(ByteStream &bytes) {
    Type t;
    t.deref_count = deserialize_u64(bytes);
    if (bytes.read_u8() == 0) {
        t.type = deserialize_basic_type(bytes);
    } else {
        t.type = deserialize_struct_type(bytes);
    }
    return t;
}

Variable deserialize_variable(ByteStream &bytes) {
    Variable v;
    v.type = deserialize_type(bytes);
    v.data = bytes.read_bytes(t_sizeof(v.type));
    return v;
}

BasicType deserialize_basic_type(ByteStream &bytes) {
    BasicType t;
    uint8_t flags = bytes.read_u8();
    t.floating = flags == 2;
    t.sign = flags == 1;
    t.bytes = deserialize_u64(bytes);
    return t;
}

uint64_t deserialize_u64(ByteStream &bytes) {
    uint64_t u64 = 0;
    for (int i = 0; i < 8; i++) {
        u64 |= std::to_integer<uint64_t>(bytes.read_byte()) << (i * 8);
    }
    return u64;
}

std::vector<std::byte> serialize_variable(const Variable &v) {
    ByteStream bytes;
    bytes.append(serialize_type(v.type));
    bytes.append(v.data);
    return bytes.bytes;
}

//template<typename T>
//T deserialize(std::vector<std::byte> bytes) {
//    ByteStream stream(std::move(bytes));
//    return primitive<T>(deserialize_variable(stream));
//}
//
//template<typename T>
//std::vector<std::byte> serialize(T val, const Type &t) {
//    return serialize_variable(construct_variable(val, t));
//}
//
//template<typename T>
//Variable construct_variable(T val, const Type &t) {
//    Variable v = Variable(t, std::vector<std::byte>(sizeof(T)));
//    std::memcpy(v.data.data(), &val, sizeof(T));
//    return v;
//}
