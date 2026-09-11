#pragma once

namespace ra::common::identity {

/// Anything carrying personally-identifiable information that can be scrubbed.
class PiiClearable {
public:
    virtual ~PiiClearable() = default;
    virtual void ClearSensitive() = 0;
};

}  // namespace ra::common::identity
