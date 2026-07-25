#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <steamkit/types/steam_id.h>

int main(void) {
    printf("Running steam_id tests...\n");

    sk_steam_id_t* sid = sk_steam_id_create(0x0110000100000001ULL);
    assert(sid != NULL);
    assert(sk_steam_id_to_uint64(sid) == 0x0110000100000001ULL);

    assert(sk_steam_id_account_id(sid) == 1);
    assert(sk_steam_id_equals(sid, sid));
    assert(!sk_steam_id_equals(sid, NULL));

    sk_steam_id_t* from_str = sk_steam_id_from_steam3("[U:1:1]");
    assert(from_str != NULL);
    assert(sk_steam_id_equals(sid, from_str));
    sk_steam_id_destroy(from_str);

    char* steam3 = sk_steam_id_render_steam3(sid);
    assert(steam3 != NULL);
    free(steam3);

    sk_steam_id_destroy(sid);

    printf("All steam_id tests passed!\n");
    return 0;
}
