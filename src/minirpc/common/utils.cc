#include "minirpc/common/utils.h"

#include <memory>
#include <assert.h>

namespace minirpc
{

uint32_t simple_crc32(const uint8_t *data, size_t len)
{
    assert(data != nullptr && "simple_crc32 input data is nullptr");
    
    // 只会被初始化一次
    static std::unique_ptr<uint32_t[]> crc32_table([]() {
        auto table = std::make_unique<uint32_t[]>(256);
        const uint32_t POLY = 0xEDB88320;
        for (uint32_t i = 0; i < 256; i++)
        {
            uint32_t crc = i;
            for (int j = 0; j < 8; j++)
            {
                if (crc & 1)
                    crc = (crc >> 1) ^ POLY;
                else
                    crc >>= 1;
            }
            table[i] = crc;
        }
        return table;
    }());

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++)
    {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }
    return ~crc; // 按标准取反输出
}

} // namespace minirpc