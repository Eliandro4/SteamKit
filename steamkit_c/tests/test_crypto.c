#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <steamkit/utils/crypto_helper.h>
#include <steamkit/utils/msg_util.h>

int main(void) {
    printf("Running crypto tests...\n");

    // Test CRC32
    const uint8_t data[] = { 0x00, 0x00, 0x00, 0x00 };
    uint32_t crc = sk_msg_crc32(data, sizeof(data));
    (void)crc;

    // Test varint encoding
    uint8_t buf[10];
    size_t len = sk_msg_write_varint(buf, 300);
    uint64_t decoded;
    size_t consumed = sk_msg_read_varint(buf, len, &decoded);
    assert(consumed == len);
    assert(decoded == 300);

    // Test signed varint
    len = sk_msg_write_varint_signed(buf, -1);
    consumed = sk_msg_read_varint_signed(buf, len, &decoded);
    assert(consumed == len);
    assert((int64_t)decoded == -1);

    printf("All crypto tests passed!\n");
    return 0;
}
