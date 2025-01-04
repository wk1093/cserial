#include <fstream>
#include <filesystem>
#include "dtc/Serial.h"

// disable packing (the compiler will add padding to structs, where my `Variable` doesn't)
#pragma pack(1)


struct SomeStuff {
    const char* str; // 8
    int a; // 4
};
const Type SomeStuff_t = new_struct_type({t_str, t_i32});

struct TestStruct {
    int* a;
    float b;
    SomeStuff stuff;
};
const Type TestStruct_t = new_struct_type({t_i32.ptr(), t_f32, SomeStuff_t});


int main() {
    static const bool do_serialize = false;
    static const bool do_deserialize = !do_serialize;

    if (do_serialize) {
        int a = 10;
        TestStruct test_struct{&a, 2.4f, {"test", 5}};

        Bytes serialized = final_serialize(test_struct, TestStruct_t);

        // write to file
        std::ofstream file("serialized.bin", std::ios::binary);
        file.write(reinterpret_cast<const char *>(serialized.data()), serialized.size());
        file.close();
    }

    if (do_deserialize) {
        std::ifstream read_file("serialized.bin", std::ios::binary);
        auto length = std::filesystem::file_size("serialized.bin");
        Bytes serialized_from_file(length);
        read_file.read(reinterpret_cast<char *>(serialized_from_file.data()), (long) length);

        Deserialized<TestStruct> deserialized = final_deserialize<TestStruct>(serialized_from_file);
        TestStruct deserialized_struct = deserialized.val;

        std::cout << *deserialized_struct.a << std::endl;
        std::cout << deserialized_struct.b << std::endl;
        std::cout << deserialized_struct.stuff.str << std::endl;
        std::cout << deserialized_struct.stuff.a << std::endl;
    }


    return 0;
}
