#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <steamkit/types/game_id.h>

int main(void) {
    printf("Running game_id tests...\n");

    sk_game_id_t* gid = sk_game_id_create(0xDEADBEEF);
    assert(gid != NULL);
    assert(sk_game_id_to_uint64(gid) == 0xDEADBEEF);

    sk_game_id_set_app_id(gid, 730);
    assert(sk_game_id_app_id(gid) == 730);

    assert(sk_game_id_equals(gid, gid));
    assert(!sk_game_id_equals(gid, NULL));

    sk_game_id_set_app_type(gid, SK_GAME_TYPE_GAME_MOD);
    assert(sk_game_id_is_mod(gid));

    char* str = sk_game_id_to_string(gid);
    assert(str != NULL);
    free(str);

    sk_game_id_destroy(gid);

    printf("All game_id tests passed!\n");
    return 0;
}
