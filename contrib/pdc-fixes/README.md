# PDC fixes for vendored submodules (reference copy)

These files document the warning/correctness patches that are **committed into
the PrivacyDataCoin-Project submodule forks**. CMake must not copy them into
submodule working trees at configure time — that made `git describe --dirty`
append `-dirty` and polluted CI package tags.

| Submodule | Fork | Branch |
|-----------|------|--------|
| `contrib/randomx` | [PrivacyDataCoin-Project/RandomARQ](https://github.com/PrivacyDataCoin-Project/RandomARQ) | `master` |
| `contrib/tor-connect` | [PrivacyDataCoin-Project/tor-connect](https://github.com/PrivacyDataCoin-Project/tor-connect) | `pdc` |
| `contrib/miniupnp` | [PrivacyDataCoin-Project/miniupnp](https://github.com/PrivacyDataCoin-Project/miniupnp) | `pdc` |

| Area | Fix |
|------|-----|
| randomx | Initialize NEON temps; `struct randomx_vm`; `const char **` for error strings; check blake2b results |
| tor-connect | OpenSSL 3 via EVP (SHA1/AES); virtual dtor on transport; remove pessimizing `move`; explicit casts; `NOMINMAX` for MSVC |
| miniupnp | Explicit `socklen_t` / `int` casts for pointer diffs and socket APIs |

When updating a fix, edit the fork, bump the submodule SHA in PDC, and keep
this tree in sync as a readable reference (not applied by CMake).
