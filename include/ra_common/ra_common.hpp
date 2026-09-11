#pragma once

// ra-common: foundational types for the Resolving Architecture / 1M5
// ecosystem. A C++ port of ra-common-java. Serialization is JSON-based
// (nlohmann::json) and NOT wire-compatible with the Java library. Include
// this umbrella header for everything, or the individual headers under
// ra_common/ for just what you need (this is a header-only library, so
// there's no compiled-size cost either way - only compile-time cost).

#include "ra_common/config.hpp"
#include "ra_common/content.hpp"
#include "ra_common/encoding.hpp"
#include "ra_common/envelope.hpp"
#include "ra_common/exception.hpp"
#include "ra_common/lifecycle.hpp"
#include "ra_common/multipart.hpp"
#include "ra_common/network.hpp"

#include "ra_common/crypto/addressable.hpp"
#include "ra_common/crypto/encryption_algorithm.hpp"
#include "ra_common/crypto/hash.hpp"
#include "ra_common/crypto/hash_algorithm.hpp"
#include "ra_common/crypto/hash_cash.hpp"
#include "ra_common/crypto/hash_util.hpp"
#include "ra_common/crypto/multihash.hpp"

#include "ra_common/identity/did.hpp"
#include "ra_common/identity/pii_clearable.hpp"
#include "ra_common/identity/public_key.hpp"
#include "ra_common/identity/signature.hpp"

#include "ra_common/messaging/channel.hpp"
#include "ra_common/messaging/command_message.hpp"
#include "ra_common/messaging/document_message.hpp"
#include "ra_common/messaging/email.hpp"
#include "ra_common/messaging/event_message.hpp"
#include "ra_common/messaging/message.hpp"
#include "ra_common/messaging/message_json.hpp"
#include "ra_common/messaging/text_message.hpp"

#include "ra_common/route/dynamic_routing_slip.hpp"
#include "ra_common/route/external_route.hpp"
#include "ra_common/route/route.hpp"
#include "ra_common/route/route_meta.hpp"

#include "ra_common/service/service.hpp"
#include "ra_common/service/service_core.hpp"
#include "ra_common/service/service_status.hpp"

#include "ra_common/tasks/task_config.hpp"
#include "ra_common/tasks/task_runner.hpp"

#include "ra_common/util/bytes_util.hpp"
#include "ra_common/util/nonce.hpp"
#include "ra_common/util/random_util.hpp"
#include "ra_common/util/string_util.hpp"
#include "ra_common/util/unique_id.hpp"
#include "ra_common/util/version_comparator.hpp"

namespace ra::common {
inline constexpr const char* kVersion = "0.1.0";
}
