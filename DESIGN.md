# ra-common-cpp — design notes

A C++20 port of [`ra-common-java`](https://github.com/resolvingarchitecture/ra-common-java),
tracking the same Phase 1 scope as
[`ra-common-rust`](https://github.com/resolvingarchitecture/ra-common-rust),
[`ra-common-python`](https://github.com/resolvingarchitecture/ra-common-python),
[`ra-common-ts`](https://github.com/resolvingarchitecture/ra-common-ts), and
[`ra-common-cs`](https://github.com/resolvingarchitecture/ra-common-cs).
First C++ repo in this ecosystem.

## Decisions

- **Header-only.** No `.cpp`/build step for the library itself — every method
  is defined inline in its class body (implicitly `inline`) or marked
  `inline` as a free function. This made porting ~40 small, tightly-coupled
  types dramatically faster than maintaining separate declarations and
  definitions would have, at the cost of consumers recompiling more when they
  touch these headers — an acceptable trade for a foundational-types library
  that changes rarely once stable.
- **`nlohmann::json` for the wire format**, vendored as a single header. C++
  has no standard JSON; every `ToJson()`/`FromJson()` pair builds/reads a
  `nlohmann::json` tree the same way the other ports build/read a plain
  object (TS) / dict (Python) / `JsonObject` (C#) — it's the most direct
  structural equivalent available.
- **Hand-rolled SHA-1/256/512 + PBKDF2-HMAC-SHA1** rather than vendoring
  OpenSSL or a crypto single-header. OpenSSL means a real external dependency
  (not just a vendored file) with build-system and linking implications; a
  vendored crypto header from GitHub is code this project would ship without
  having verified it. Implementing directly and checking the result against
  `sha1sum`/`sha256sum`/`sha512sum` and the RFC 6070 PBKDF2 test vectors
  gives a stronger correctness guarantee for what it needs (this package's
  own non-interoperable hash/password formats) than trusting either
  alternative blindly would.
- **`std::thread`-based `TaskRunner`**, not an async runtime — C++ has no
  standard event loop or async/await. Closer to `ra-common-python`'s
  threading design than `ra-common-ts`'s single-threaded async one. See
  "Concurrency bugs caught" below — this file went through several real
  fixes before its tests passed.
- **POSIX-only.** `/dev/urandom` for randomness (`util/random_util.hpp`),
  `environ` for environment access (`config.hpp`), `$HOME` read directly for
  directory resolution — no `BCryptGenRandom`/`%USERPROFILE%` Windows
  backends. Matches this ecosystem's Linux-first posture elsewhere (e.g.
  `i2p-rust`'s Redox gaps); a Windows backend is `TODO.md`, not attempted here.
- **`std::optional<T>`** for every nullable field, matching `?`/`| undefined`
  (TS), `Optional`/`| None` (Python), `?`/nullable (C#) exactly in intent.
- **`std::unique_ptr<Route>`/`std::unique_ptr<Message>`** for the two
  polymorphic hierarchies, with a `*FromJson` free-function dispatcher on the
  wire's `type`/`kind` tag — same shape as every other port's dispatcher,
  translated to C++'s ownership model. `Envelope` is therefore **move-only**
  (copy constructor/assignment deleted) since it holds both.
- **Namespaces are lowercase, types are `PascalCase`** (`ra::common::route`
  containing class `Route`, `ra::common::service` containing class
  `Service`). This is standard C++/Python-style naming, and it happens to
  sidestep a real bug class entirely: `ra-common-cs` needed to rename its
  `Route`/`Service` namespaces to `Routing`/`Services` because C# convention
  capitalizes namespaces the same way as types, so `namespace Ra.Common.Route
  { class Route }` breaks unqualified lookup from the parent namespace.
  Because C++ (like Python) conventionally differentiates namespace case from
  type case, `ra::common::route::Route` has no equivalent problem — worth
  knowing why, not just that it doesn't happen here.

## Idiom mapping

| Java | C++ |
|---|---|
| `JSONSerializable` | `ToJson() const` / `static FromJson(const nlohmann::json&)` |
| abstract base + `Class.forName("type")` | abstract class (pure virtual) + a free-function `*FromJson` dispatcher returning `std::unique_ptr<Base>` |
| abstract base w/ shared fields (`BaseRoute`) | a `RouteMeta` struct held by each route |
| abstract class w/ behaviour (`BaseService`) | abstract class methods + a `ServiceCore` the impl holds |
| static utility class (`HashUtil`) | a namespace of free functions (`hash_util::GenerateHash`, ...) |
| `enum X { A("a") }` + `value()` | plain C++ `enum class`, wire string from a `ToString()` overload / `FromString` parser |
| checked `*Exception` | `RaException` with a `RaErrorKind kind()` |
| `Stack<T>` / `DequeStack` | `std::deque<std::unique_ptr<Route>>`, `push_front`/`pop_front` for LIFO |
| `Properties` | `std::map<std::string, std::string>` |
| `null` | `std::optional<T>` or (for polymorphic/owned types) `nullptr` on a `unique_ptr` |

## Concurrency bugs caught while building `TaskRunner`

Writing C++ concurrency code without running it is a bad idea; three separate
bugs surfaced only once the code actually compiled and ran under threads:

1. **Dangling pointers into a growing `std::vector`.** The first draft took
   `Managed&` references into a `std::vector<Managed>` and captured raw
   pointers to those elements in worker-thread lambdas. `AddTask` pushing a
   new entry can reallocate the vector's backing storage, invalidating every
   pointer a running worker thread was still holding. Fixed by making each
   entry a `std::shared_ptr<Managed>`, captured by value in its worker's
   lambda — the entry's lifetime is then independent of the tracking
   vector's storage.
2. **Unsynchronized field access across threads.** The same draft read/wrote
   `Managed::status`/`stop` from worker threads without a lock while
   `Pass()`/`Shutdown()` read them under `mutex_` — a data race regardless of
   the pointer issue. Fixed by making those fields `std::atomic`.
3. **Destroying a joinable `std::thread`.** `Pass()`'s cleanup erases
   entries whose task has completed; the first draft stored each entry's
   `std::thread` inline and erased it directly — but destroying a
   `std::thread` that hasn't been joined or detached calls
   `std::terminate()`, so any plain one-shot task would crash the process on
   its *next* poll tick. Fixed by detaching worker threads instead of storing
   them, with a separate atomic active-worker counter + condition variable
   for `Shutdown()` to wait on instead of `.join()`.

None of these were hypothetical — each one reproduced against the ported
`TaskRunner` test (one-shot + periodic tasks) before being fixed. `Shutdown()`
is not perfectly linearizable against a concurrent `AddTask`/poll-tick race
(a task scheduled in the same instant as shutdown could start after
`Shutdown()` observes zero active workers) - this is a narrow, non-corrupting
edge case shared in similar form by the other ports' shutdown paths (a
snapshot-based `Promise.all`/thread-join list has the same gap), not
something worth a bigger design to close here.

## Bugs fixed during the port (from the Java original, inherited by every port)

Same fixes as `ra-common-ts`/`ra-common-python`/`ra-common-cs`:

- `Signature` (de)serialization was an empty stub in Java — implemented fully here.
- `BaseRoute.fromMap` read the key `"routedId"` instead of `"routeId"` — corrected.
- `Nonce` prune computed `max * (pct / 100)` → 0 in integer division — now
  `max_size_ * prune_percent_ / 100`, with an O(1) `unordered_set` membership
  check plus a `deque` for eviction order.
- `DID.getPassphraseHashAlgorithm()` could NPE — `EffectivePassphraseHashAlgorithm()`
  falls back to the stored field.

## C++-specific pitfalls hit during this port

- A free function used inside a class's inline method body must already be
  *declared* earlier in the same translation unit — the "complete-class
  context" deferred-lookup rule only covers names inside the same class, not
  free functions elsewhere in the file. `HashCash::Sha1Bits` calling
  `LeadingZeroBits` (originally defined after the class) failed to compile
  until the helper was moved above the class.
- `Convert`-style hex/base64 helpers don't exist in the standard library at
  all (not even from C++23) — fully hand-rolled in `encoding.hpp`, unlike
  `ra-common-cs`, which only needed to work around one missing overload
  (`Convert.ToHexStringLower`).
- `extern char** environ;` needs `<unistd.h>` included first on glibc, or the
  compiler sees a C++-linkage declaration clash against libc's own
  C-linkage one further down the include chain (nlohmann/json.hpp pulls in
  `<memory>` → ... → `<unistd.h>` transitively, so the conflict can appear
  from an `#include` that looks unrelated to `environ` at all).
- `std::make_shared<T>(T{...})` constructs a temporary `T` and then
  move/copy-constructs it into the shared allocation — for a `T` with
  `std::atomic` members (non-movable, non-copyable by design), that move is
  ill-formed. `std::make_shared<T>(ctor_args...)` (constructing in place,
  forwarding to `T`'s constructor directly) is required instead whenever a
  type has non-movable members.

## Not here (Phase 2, deferred)

`currency/*`, `locale/*`, `Scrubber`, `RegExGen`, the full network service
layer, `Protocol` (multiaddr), `ShellCommand`, `BrowserUtil`, `FileUtil`,
`InfoVault*`, `social/*`, a Windows backend for randomness/env/directories.
`DLC` is folded into `Envelope` methods (not ported as a class).
