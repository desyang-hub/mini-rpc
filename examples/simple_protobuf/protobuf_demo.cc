#include <iostream>
#include <fstream>
#include "person.pb.h"

#include "minirpc/protocol/Serialize.h"

using namespace minirpc;

int main() {
    // 创建并填充对象
    example::Person person;
    person.set_id(123);
    person.set_name("张三");
    person.set_email("zhangsan@example.com");

    // Serialize::SetSerializeType(SERIALIZE_PROTOBUF);
    std::string bytes = Serialize::SerializationProtobuf(person);
    std::cout << "protobuf string: " << bytes << std::endl;

    auto obj = Serialize::DeserializationProtobuf<example::Person>(bytes);
    std::cout << obj.id() << std::endl;
    std::cout << obj.name() << std::endl;
    std::cout << obj.email() << std::endl;


    // 序列化为字符串
    std::string buffer;
    person.SerializeToString(&buffer);

    // 反序列化
    example::Person new_person;
    new_person.ParseFromString(buffer);

    // 输出结果验证
    std::cout << "ID: " << new_person.id() << "\n";
    std::cout << "Name: " << new_person.name() << "\n";
    std::cout << "Email: " << new_person.email() << "\n";

    return 0;
}