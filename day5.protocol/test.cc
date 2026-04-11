#include <iostream>

#include "protocol.h"
#include "Encoder.h"
#include "Decoder.h"
#include "nlohmann/json.hpp"
#include "JsonSerialize.h"

using json = nlohmann::json;

struct User
{
    int id;
    std::string name;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(User, id, name);
};

int main(int argc, char const *argv[])
{
    std::cout << sizeof(ProtocolHeader) << std::endl;


    std::string msg = "Hello rpc";

    auto packet = Encoder::Encode(MSG_HEARTABAT, msg);

    std::cout << (packet.data() + sizeof(ProtocolHeader)) << std::endl;

    ProtocolHeader header;
    std::string body;

    Decoder::Decode(packet, header, body);
    std::cout << header.body_len << " " <<  body << std::endl;

    User u = {103, "desyang"};

    std::cout << static_cast<int>(SERIALIZE_JSON) << std::endl;

    auto serializer = new JsonSerialize();

    auto u_str = serializer->serialization(u);
    User u1 = serializer->deserialization<User>(u_str);

    std::cout << u1.id << " " << u1.name << std::endl;

    delete serializer;

    return 0;
}
