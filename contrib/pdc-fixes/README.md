# PDC in-tree fixes for vendored submodules

These files are copied onto `contrib/randomx`, `contrib/tor-connect`, and
`contrib/miniupnp` at CMake configure time (see `contrib/CMakeLists.txt`).

They fix real compiler warnings and bugs without relying on `-Wno-*` /
`/wd*` suppressions, and without requiring pushes to upstream submodule
repositories.

| Area | Fix |
|------|-----|
| randomx | Initialize NEON temps; `struct randomx_vm`; `const char **` for error strings; check blake2b results |
| tor-connect | OpenSSL 3 via EVP (SHA1/AES); virtual dtor on transport; remove pessimizing `move`; explicit casts; `NOMINMAX` for MSVC |
| miniupnp | Explicit `socklen_t` / `int` casts for pointer diffs and socket APIs |
