#ifndef STEAMKIT_H
#define STEAMKIT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Core types
#include "steamkit/types/game_id.h"
#include "steamkit/types/steam_id.h"
#include "steamkit/types/job_id.h"
#include "steamkit/types/global_id.h"
#include "steamkit/types/key_value.h"
#include "steamkit/types/msg_object.h"

// Base message system
#include "steamkit/base/emsg.h"
#include "steamkit/base/msg_hdr.h"
#include "steamkit/base/packet_base.h"
#include "steamkit/base/client_msg.h"

// Networking
#include "steamkit/networking/connection.h"
#include "steamkit/networking/tcp_connection.h"
#include "steamkit/networking/websocket_connection.h"

// Steam client
#include "steamkit/steam/steam_client/configuration/steam_configuration.h"
#include "steamkit/steam/cm_client.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"

// Handlers
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/steam_friends.h"
#include "steamkit/steam/handlers/steam_user.h"
#include "steamkit/steam/handlers/steam_apps.h"
#include "steamkit/steam/handlers/steam_game_coordinator.h"
#include "steamkit/steam/handlers/steam_game_server.h"
#include "steamkit/steam/handlers/steam_user_stats.h"
#include "steamkit/steam/handlers/steam_master_server.h"
#include "steamkit/steam/handlers/steam_cloud.h"
#include "steamkit/steam/handlers/steam_workshop.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/handlers/steam_screenshots.h"
#include "steamkit/steam/handlers/steam_matchmaking.h"
#include "steamkit/steam/handlers/steam_networking.h"
#include "steamkit/steam/handlers/steam_content.h"
#include "steamkit/steam/handlers/steam_auth_ticket.h"

// Utilities
#include "steamkit/utils/crypto_helper.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/net_helpers.h"
#include "steamkit/utils/msg_util.h"

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_H
