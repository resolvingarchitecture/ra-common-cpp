# TODO

## P0 — done

- [x] Core types ported: exception, encoding (base32/58/64), util (bytes/
      strings/version/random/UniqueId/Nonce), crypto (self-contained SHA-1/
      256/512 + PBKDF2, Hash/HashUtil/Multihash/HashCash/EncryptionAlgorithm),
      identity (Signature/PublicKey/Did), network, route (Route/SimpleRoute/
      SimpleExternalRoute/RelayedExternalRoute/DynamicRoutingSlip), messaging
      (Message/DocumentMessage/CommandMessage/EventMessage/TextMessage/Email/
      channel interfaces), lifecycle, service (ServiceStatus/ServiceCore/
      Service), content, multipart, tasks (ITask/TaskRunner over std::thread),
      config, envelope.
- [x] 17 doctest test cases (98 assertions) ported from `ra-common-ts`'s
      `core.test.ts` + `envelope.test.ts`, all passing.
- [x] Hand-rolled SHA-1/256/512 verified against `sha1sum`/`sha256sum`/
      `sha512sum`; PBKDF2-HMAC-SHA1 verified against RFC 6070 test vectors.
- [x] `TaskRunner` concurrency bugs (dangling pointers, unsynchronized field
      access, destroying a joinable thread) found and fixed via the actual
      test suite - see DESIGN.md.

## P1 — hardening

- [ ] `TaskRunner::Shutdown()` isn't perfectly linearizable against a
      concurrent `AddTask`/poll-tick race (see DESIGN.md) - revisit if this
      ever matters in practice, e.g. once a real service host uses it.
- [ ] Fuzz/property-test the SHA implementations against more than the
      standard test vectors (empty input, very large input, all block-
      boundary edge cases) before trusting them for anything beyond this
      package's own hash formats.
- [ ] `HashUtil::ConstantTimeEquals` is constant-time; the rest of the crypto
      here (SHA compression, PBKDF2) is not hardened against timing/cache
      side channels - fine for this package's stated non-interoperable scope,
      not fine if anything here is ever asked to guard a real secret against
      a serious adversary.

## Phase 2 (deferred, mirrors the other ra-common-* ports)

- [ ] `currency/*` (~72 Java files) → enum + table
- [ ] `locale/*` + `LocaleUtil` / `LanguageUtil` / `Resources`
- [ ] `Scrubber`, `RegExGen` / `IntegerRangeRegex`
- [ ] full `network` service layer
- [ ] `Protocol` (multiaddr)
- [ ] `ShellCommand`, `BrowserUtil`, `FileUtil`
- [ ] `InfoVault*`, `SimpleByteCache`, `OrderedProperties`
- [ ] `social/*`
- [ ] Windows backend for `util/random_util.hpp` (`BCryptGenRandom`) and
      `config.hpp` (`%USERPROFILE%`, `_wenviron`)

## Packaging

- [ ] A CMake `install()`/config-package setup for `find_package(ra_common)`
      once a C++ consumer (e.g. `i2p-cpp`, `service-bus-cpp`) actually needs it.
- [ ] Consider vcpkg/Conan packaging once there's more than one consumer.
