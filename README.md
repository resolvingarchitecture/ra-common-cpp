# Resolving Architecture Common Library (C++)

A C++20 port of
[`ra-common-java`](https://github.com/resolvingarchitecture/ra-common-java) —
the foundational types for the Resolving Architecture / 1M5 ecosystem,
alongside the [`ra-common-ts`](https://github.com/resolvingarchitecture/ra-common-ts),
[`ra-common-python`](https://github.com/resolvingarchitecture/ra-common-python), and
[`ra-common-cs`](https://github.com/resolvingarchitecture/ra-common-cs) ports.

**Header-only.** Include `ra_common/ra_common.hpp` for everything, or pick
individual headers under `ra_common/` for just what you need.

This package provides:

- **`ra::common::Envelope`** — the universal message wrapper passed between services.
- **`ra::common::messaging`** — `Message` (`DocumentMessage` / `CommandMessage` /
  `EventMessage` / `TextMessage`), producer/consumer/channel/bus interfaces.
- **`ra::common::route`** — routing slips (`DynamicRoutingSlip`) and external/relayed routes.
- **`ra::common::service`** — the `Service` / `ILifeCycle` interface and `ServiceCore` shared state.
- **`ra::common::identity`** — `Did`, `PublicKey`, `Signature`.
- **`ra::common::crypto`** — `Hash`, `Multihash`, `HashCash`, password hashing
  (self-contained SHA-1/256/512 + PBKDF2 — see below).
- **`ra::common::Content`** — typed content (text / html / json / image / audio / video / binary).
- **`ra::common::tasks`** — `ITask` + a `std::thread`-based `TaskRunner`.
- **`ra::common::Config`** / **`system_settings`** — `.properties` loading + XDG directory resolution.
- utilities: base32/58/64, version comparison, replay `Nonce`, `UniqueId`, byte packing.

Serialization is JSON-based (`nlohmann::json`) and **not** wire-compatible with
the Java version.

## Two vendored dependencies (unlike every other port here)

C++ has no standard library JSON, base64, or cryptographic hashing — every
other language port gets those from its runtime for free. Rather than
hand-roll a JSON parser or trust hand-rolled crypto everywhere, this package:

- Vendors [`nlohmann::json`](https://github.com/nlohmann/json) (single header,
  MIT) under `third_party/nlohmann/` for the JSON tree.
- **Implements SHA-1/256/512 and PBKDF2-HMAC-SHA1 directly** (`crypto/sha.hpp`,
  `crypto/pbkdf2.hpp`) rather than vendoring a third crypto dependency —
  verified against `sha1sum`/`sha256sum`/`sha512sum` and the RFC 6070 PBKDF2
  test vectors. Not constant-time-hardened beyond the password-hash comparison;
  see `DESIGN.md`.
- Vendors [`doctest`](https://github.com/doctest/doctest) (single header, MIT)
  under `third_party/doctest/` for the test suite only (not a dependency of
  the library itself).

Base32/58/64 are hand-rolled here too, same as in every other port (no
language's standard library has those either).

## Platform: POSIX only

Random bytes come from `/dev/urandom`, environment access uses POSIX
`environ`, and directory resolution reads `$HOME` directly — no Windows
backend. See `DESIGN.md`.

## Use

```cpp
#include "ra_common/ra_common.hpp"

int main() {
    auto e = ra::common::Envelope::Document();
    e.AddRoute("ra.http.HttpService", "SEND");
    e.AddContent(nlohmann::json{{"hello", "world"}});
    e.Ratchet();

    assert(e.GetRoute()->service() == "ra.http.HttpService");
    assert(e.Content()["hello"] == "world");

    auto back = ra::common::Envelope::FromJsonString(e.ToJsonString());
    assert(back.Equals(e));
}
```

## Develop

```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
```

## Status

Phase 1 (core), matching the other ports' scope. Deferred: currency,
locale/i18n, the full network service layer, `Protocol`, shell/file/browser
utilities, `InfoVault`, a Windows backend. See `TODO.md`.

## Reference

- [`DESIGN.md`](DESIGN.md)
- [`TODO.md`](TODO.md)

## License

MIT — see `LICENSE` (which also covers the two vendored third-party headers).
