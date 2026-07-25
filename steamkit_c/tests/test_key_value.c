#include "steamkit/types/key_value.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    sk_key_value_t* root = sk_key_value_create("root");
    assert(root);

    sk_key_value_t* child1 = sk_key_value_create("appid");
    sk_key_value_set_uint32(child1, 730);
    sk_key_value_add_child(root, child1);

    sk_key_value_t* child2 = sk_key_value_create("name");
    sk_key_value_set_string(child2, "CS2");
    sk_key_value_add_child(root, child2);

    sk_key_value_t* child3 = sk_key_value_create("enabled");
    sk_key_value_set_bool(child3, true);
    sk_key_value_add_child(root, child3);

    sk_key_value_t* child4 = sk_key_value_create("size");
    sk_key_value_set_int64(child4, 15000000000);
    sk_key_value_add_child(root, child4);

    sk_key_value_t* child5 = sk_key_value_create("blob");
    uint8_t blob[] = {0xDE, 0xAD, 0xBE, 0xEF};
    sk_key_value_set_binary(child5, blob, sizeof(blob));
    sk_key_value_add_child(root, child5);

    uint8_t buf[1024];
    size_t written = sk_key_value_serialize(root, buf, sizeof(buf));
    assert(written > 0 && written < sizeof(buf));

    sk_key_value_t* parsed = sk_key_value_create("parsed");
    size_t consumed = sk_key_value_deserialize(parsed, buf, written);
    fprintf(stderr, "written=%zu consumed=%zu\n", written, consumed);
    assert(consumed == written);

    assert(sk_key_value_child_count(parsed) == 5);
    assert(strcmp(sk_key_value_name(sk_key_value_child(parsed, 0)), "appid") == 0);
    assert(sk_key_value_uint32(sk_key_value_child(parsed, 0)) == 730);
    assert(strcmp(sk_key_value_name(sk_key_value_child(parsed, 1)), "name") == 0);
    assert(strcmp(sk_key_value_string(sk_key_value_child(parsed, 1)), "CS2") == 0);
    assert(sk_key_value_bool(sk_key_value_child(parsed, 2)) == true);
    assert(sk_key_value_int64(sk_key_value_child(parsed, 3)) == 15000000000);

    size_t blob_len = 0;
    const uint8_t* blob_data = sk_key_value_binary(sk_key_value_child(parsed, 4), &blob_len);
    assert(blob_len == 4);
    assert(memcmp(blob_data, blob, 4) == 0);

    sk_key_value_destroy(parsed);
    sk_key_value_destroy(root);

    printf("KeyValue tests passed\n");
    return 0;
}
