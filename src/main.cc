#include "minirpc/protocol/Encoder.h"
#include "minirpc/protocol/Decoder.h"

#include "minirpc/common/utils.h"
#include "minirpc/common/logger.h"

using namespace minirpc;

int main(int argc, char const *argv[])
{

    // ENABLE_ASYNC_LOGING();

    LOG_INFO("log write");

    std::string srvNmae = "test";
    std::string body_str = "body";
    Bytes encode_bytes = Encoder::Encode(srvNmae, body_str);

    uint8_t crcNum = simple_crc32(reinterpret_cast<const uint8_t*>(body_str.data()), body_str.size());

    ProtocolHeader header;

    std::string body;
    std::string recv_srv_name;
    bool is_success = Decoder::Decode(encode_bytes, header, recv_srv_name, body);
    std::cout << "un_package_success: " << is_success << std::endl;

    std::cout << "recv_srv_name: " << recv_srv_name << std::endl;
    std::cout << "body: " << body << std::endl;
    
    return 0;
}
