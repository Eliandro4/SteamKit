#include "steamkit/types/workshop_file_type.h"

bool sk_workshop_file_type_is_supported(sk_workshop_file_type_t type) {
    switch (type) {
        case SK_WORKSHOP_FILE_TYPE_COMMUNITY:
        case SK_WORKSHOP_FILE_TYPE_ART:
        case SK_WORKSHOP_FILE_TYPE_SCREENSHOT:
        case SK_WORKSHOP_FILE_TYPE_MERCH:
        case SK_WORKSHOP_FILE_TYPE_INTEGRATED_GUIDE:
        case SK_WORKSHOP_FILE_TYPE_CONTROLLER_BINDING:
            return true;
        default:
            return false;
    }
}