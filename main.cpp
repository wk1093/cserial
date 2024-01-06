#include "dtc/Serial.h"

// disable packing (the compiler will add padding to structs, where my `Variable` doesn't)
#pragma pack(1)
struct SomeStuff {
    const char* str; // 8
    int a; // 4
};
const Type SomeStuff_t = new_struct_type({t_str, t_i32});

struct TestStruct {
    int a;
    float b;
    SomeStuff stuff;
};
const Type TestStruct_t = new_struct_type({t_i32, t_f32, SomeStuff_t});


int main() {
    TestStruct test_struct{1, 2.4f, {"hello", 5}};

    Bytes serialized = serialize(test_struct, TestStruct_t);

    auto deserialized_struct = deserialize<TestStruct>(serialized);

    std::cout << deserialized_struct.a << std::endl;
    std::cout << deserialized_struct.b << std::endl;
    std::cout << deserialized_struct.stuff.str << std::endl;
    std::cout << deserialized_struct.stuff.a << std::endl;

    return 0;
}
