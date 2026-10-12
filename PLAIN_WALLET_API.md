# PDC Plain Wallet API Specification

**Version:** 0.1.0-draft
**Status:** Draft for review
**License:** MIT with PDC attribution

---

## Overview

The Plain Wallet API is a minimal, stable C interface that abstracts the complexity of the PDC wallet core (wallet2, currency_core, crypto) into simple functions suitable for:
- Mobile apps (iOS Swift, Android Kotlin/JNI)
- Browser extensions (WebAssembly or native messaging)
- TypeScript SDK (`pdc_web3`) native module
- Third-party integrations

**Design Principles:**
- Opaque handles (`pdc_wallet_t*`) - no exposure of internal structures
- C ABI compatible (extern "C") - usable from any language
- Explicit error codes - no exceptions across FFI boundary
- Caller-allocated output buffers where practical
- Thread-safe for read operations, caller must serialize writes

---

## Data Types

```c
// Opaque wallet handle
typedef struct pdc_wallet pdc_wallet_t;

// Configuration for wallet opening
typedef struct {
    const char* wallet_path;        // Path to wallet file (.wallet)
    const char* password;           // Wallet password (UTF-8)
    const char* daemon_address;     // Daemon RPC address (e.g., "http://127.0.0.1:19211")
    uint32_t flags;                 // Reserved, must be 0
} pdc_wallet_config_t;

// Asset balance
typedef struct {
    char asset_id[65];              // Hex-encoded asset ID (32 bytes = 64 hex chars + null)
    uint64_t unlocked;              // Unlocked balance in atomic units
    uint64_t locked;                // Locked balance in atomic units
    uint8_t decimal_point;          // Decimal places for display
    char name[64];                  // Asset name (UTF-8)
    char ticker[16];                // Asset ticker (UTF-8)
} pdc_asset_balance_t;

// Transfer request
typedef struct {
    const char* destination_address; // PDC address or @alias
    const char* asset_id;           // Hex asset ID (native = PDC_ASSET_INFO.asset_id)
    uint64_t amount;                // Amount in atomic units
    uint64_t fee;                   // Fee in atomic units (0 = auto)
    uint32_t mixin;                 // Mixin count (0 = network default)
    const char* payment_id;         // Optional payment ID (hex, 8 or 32 bytes)
    uint32_t flags;                 // Reserved
} pdc_transfer_request_t;

// Transfer result
typedef struct {
    char tx_hash[65];               // Transaction hash (hex)
    uint64_t fee_paid;              // Actual fee paid
} pdc_transfer_result_t;

// Swap proposal info (for display)
typedef struct {
    char proposal_hex[4096];        // Hex-encoded proposal blob
    char proposer_address[106];     // Proposer's PDC address
    pdc_asset_funds_t to_finalizer[16];  // What proposer sends
    pdc_asset_funds_t to_initiator[16];  // What proposer receives
    uint32_t to_finalizer_count;
    uint32_t to_initiator_count;
    uint64_t fee_paid_by_a;         // Fee paid by proposer
    uint64_t expiration_time;       // Unix timestamp (0 = no expiration)
} pdc_swap_proposal_info_t;

// Asset funds (for swap)
typedef struct {
    char asset_id[65];
    uint64_t amount;
} pdc_asset_funds_t;

// Alias info
typedef struct {
    char alias[64];                 // Without @ prefix
    char address[106];              // Linked PDC address
    char comment[256];              // User comment
    uint64_t cost;                  // Registration cost in atomic units
} pdc_alias_info_t;

// Staking info
typedef struct {
    bool active;                    // Is staking enabled
    uint64_t estimated_time_to_block; // Seconds (0 = unknown)
    uint64_t stake_amount;          // Currently staked amount
} pdc_staking_info_t;

// Error codes
typedef enum {
    PDC_OK = 0,
    PDC_ERR_INVALID_ARG = -1,
    PDC_ERR_WALLET_NOT_FOUND = -2,
    PDC_ERR_WRONG_PASSWORD = -3,
    PDC_ERR_WALLET_CORRUPT = -4,
    PDC_ERR_DAEMON_CONNECTION = -5,
    PDC_ERR_INSUFFICIENT_FUNDS = -6,
    PDC_ERR_INVALID_ADDRESS = -7,
    PDC_ERR_INVALID_ASSET_ID = -8,
    PDC_ERR_TRANSACTION_FAILED = -9,
    PDC_ERR_SWAP_INVALID_PROPOSAL = -10,
    PDC_ERR_SWAP_EXPIRED = -11,
    PDC_ERR_SWAP_INSUFFICIENT_FUNDS = -12,
    PDC_ERR_ALIAS_TAKEN = -13,
    PDC_ERR_ALIAS_INVALID = -14,
    PDC_ERR_STAKING_NOT_AVAILABLE = -15,
    PDC_ERR_NETWORK_ERROR = -16,
    PDC_ERR_INTERNAL = -100
} pdc_error_t;
```

---

## Core Functions

### Wallet Lifecycle

```c
// Open existing wallet or create new one
// Returns PDC_OK on success, sets *out_wallet
// Caller must call pdc_wallet_close() when done
int pdc_wallet_open(const pdc_wallet_config_t* config, pdc_wallet_t** out_wallet);

// Close wallet and free resources
// Invalidates the handle
int pdc_wallet_close(pdc_wallet_t* wallet);

// Get API version string (e.g., "0.1.0")
const char* pdc_api_version(void);
```

### Balance & Address

```c
// Get all asset balances for wallet
// Returns count of balances written to output array (max max_count)
// If balances is NULL, returns total count needed
size_t pdc_wallet_get_balances(pdc_wallet_t* wallet, pdc_asset_balance_t* balances, size_t max_count);

// Get primary address (for receive)
// Returns PDC_OK, writes null-terminated string to out_address (caller allocates, min 106 bytes)
int pdc_wallet_get_address(pdc_wallet_t* wallet, char* out_address, size_t max_len);
```

### Transfers

```c
// Send transfer (PDC or Confidential Asset)
// Returns PDC_OK on success, fills result
// Validates: sufficient unlocked balance, valid address/alias, valid asset_id
int pdc_wallet_transfer(pdc_wallet_t* wallet, const pdc_transfer_request_t* request, pdc_transfer_result_t* result);

// Estimate fee for transfer without sending
// Returns estimated fee in atomic units
int pdc_wallet_estimate_fee(pdc_wallet_t* wallet, const pdc_transfer_request_t* request, uint64_t* out_fee);
```

### Confidential Assets

```c
// Register custom asset ID in local whitelist (for display)
// Returns PDC_OK, fills asset_descriptor (name, ticker, decimal_point, etc.)
int pdc_wallet_add_custom_asset(pdc_wallet_t* wallet, const char* asset_id_hex, pdc_asset_balance_t* out_descriptor);

// Remove from local whitelist
int pdc_wallet_remove_custom_asset(pdc_wallet_t* wallet, const char* asset_id_hex);

// Deploy new Confidential Asset (emit initial supply)
// Returns tx_hash of deployment transaction
int pdc_wallet_deploy_asset(pdc_wallet_t* wallet, const pdc_asset_descriptor_t* descriptor, const pdc_transfer_request_t* initial_distribution[], size_t dist_count, char* out_tx_hash, size_t max_len);
```

### Ionic Swaps

```c
// Create swap proposal
// proposal_info: what I send (to_finalizer) and what I want (to_initiator)
// Returns hex-encoded proposal blob in out_proposal_hex (caller allocates, min 4096 bytes)
int pdc_wallet_swap_create(pdc_wallet_t* wallet, const pdc_swap_proposal_request_t* proposal_info, char* out_proposal_hex, size_t max_len);

// Decode and validate swap proposal (for display before accept)
// Returns proposal details including decrypted amounts
int pdc_wallet_swap_get_info(pdc_wallet_t* wallet, const char* proposal_hex, pdc_swap_proposal_info_t* out_info);

// Accept swap proposal
// Returns transaction hash of acceptance
int pdc_wallet_swap_accept(pdc_wallet_t* wallet, const char* proposal_hex, char* out_tx_hash, size_t max_len);
```

### Aliases

```c
// Register new alias @name
// cost: registration fee (use pdc_wallet_alias_get_cost first)
int pdc_wallet_alias_register(pdc_wallet_t* wallet, const char* alias_name, const char* comment, uint64_t fee, char* out_tx_hash, size_t max_len);

// Update alias comment
int pdc_wallet_alias_update(pdc_wallet_t* wallet, const char* alias_name, const char* new_comment, uint64_t fee, char* out_tx_hash, size_t max_len);

// Transfer alias to another address
int pdc_wallet_alias_transfer(pdc_wallet_t* wallet, const char* alias_name, const char* new_address, uint64_t fee, char* out_tx_hash, size_t max_len);

// Get alias cost
int pdc_wallet_alias_get_cost(pdc_wallet_t* wallet, const char* alias_name, uint64_t* out_cost);

// Lookup alias by name prefix (for autocomplete)
// Returns array of alias names (without @), max max_count
size_t pdc_wallet_alias_lookup(pdc_wallet_t* wallet, const char* prefix, char** out_aliases, size_t max_count);
```

### Staking (Zarcanum)

```c
// Enable staking
int pdc_wallet_staking_start(pdc_wallet_t* wallet);

// Disable staking
int pdc_wallet_staking_stop(pdc_wallet_t* wallet);

// Get staking status
int pdc_wallet_staking_get_info(pdc_wallet_t* wallet, pdc_staking_info_t* out_info);
```

### Utility

```c
// Convert error code to human-readable string (English)
const char* pdc_strerror(int error_code);

// Free array allocated by lookup functions
void pdc_free_string_array(char** array, size_t count);
```

---

## JSON Schema (for code generation)

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "PDC Plain Wallet API",
  "version": "0.1.0",
  "types": {
    "pdc_asset_balance": {
      "type": "object",
      "properties": {
        "asset_id": { "type": "string", "pattern": "^[0-9a-f]{64}$" },
        "unlocked": { "type": "integer", "minimum": 0 },
        "locked": { "type": "integer", "minimum": 0 },
        "decimal_point": { "type": "integer", "minimum": 0, "maximum": 18 },
        "name": { "type": "string", "maxLength": 63 },
        "ticker": { "type": "string", "maxLength": 15 }
      },
      "required": ["asset_id", "unlocked", "locked", "decimal_point"]
    },
    "pdc_transfer_request": {
      "type": "object",
      "properties": {
        "destination_address": { "type": "string" },
        "asset_id": { "type": "string", "pattern": "^[0-9a-f]{64}$" },
        "amount": { "type": "integer", "minimum": 1 },
        "fee": { "type": "integer", "minimum": 0 },
        "mixin": { "type": "integer", "minimum": 0, "maximum": 100 },
        "payment_id": { "type": "string", "pattern": "^([0-9a-f]{16}|[0-9a-f]{64})?$" }
      },
      "required": ["destination_address", "asset_id", "amount"]
    },
    "pdc_swap_proposal_request": {
      "type": "object",
      "properties": {
        "to_finalizer": {
          "type": "array",
          "items": { "$ref": "#/types/pdc_asset_funds" },
          "minItems": 1,
          "maxItems": 16
        },
        "to_initiator": {
          "type": "array",
          "items": { "$ref": "#/types/pdc_asset_funds" },
          "minItems": 1,
          "maxItems": 16
        },
        "fee_paid_by_a": { "type": "integer", "minimum": 0 },
        "expiration_time": { "type": "integer", "minimum": 0 },
        "mixin": { "type": "integer", "minimum": 0, "maximum": 100 }
      },
      "required": ["to_finalizer", "to_initiator"]
    },
    "pdc_asset_funds": {
      "type": "object",
      "properties": {
        "asset_id": { "type": "string", "pattern": "^[0-9a-f]{64}$" },
        "amount": { "type": "integer", "minimum": 1 }
      },
      "required": ["asset_id", "amount"]
    }
  }
}
```

---

## Implementation Notes

### Memory Management
- All output strings are caller-allocated (fixed buffers)
- `pdc_free_string_array` must be called for arrays returned by lookup functions
- No hidden allocations - deterministic memory usage

### Thread Safety
- `pdc_wallet_get_balances`, `pdc_wallet_get_address`, `pdc_wallet_swap_get_info`, `pdc_wallet_staking_get_info` - thread-safe (read-only)
- All mutating functions (`transfer`, `swap_create`, `alias_register`, `staking_start/stop`) - caller must serialize

### Error Handling
- All functions return `pdc_error_t` (0 = success, negative = error)
- Use `pdc_strerror` for user-facing messages
- Detailed errors logged to wallet log (configurable via daemon)

### Daemon Connection
- Wallet maintains persistent connection to daemon
- Auto-reconnect on network interruption
- RPC timeouts: 30s default, configurable in config

### Asset ID Format
- Always 32-byte (64 hex char) lowercase hex string
- Native PDC asset ID: defined in `PDC_ASSET_INFO.asset_id` (compile-time constant)

---

## Versioning & Compatibility

- **Major version** (X.0.0): Breaking ABI changes
- **Minor version** (0.Y.0): New functions added, existing unchanged
- **Patch version** (0.0.Z): Bug fixes only

Library exports `pdc_api_version()` for runtime version detection.

---

## Build Requirements

- C++17 compiler (for core implementation)
- CMake 3.20+
- Boost 1.75+ (system, filesystem, thread, program_options, serialization)
- OpenSSL 1.1.1+ or 3.0+
- libiconv (for Windows/macOS)
- Platform SDKs: iOS 14+, Android NDK r25+, macOS 11+, Windows 10 SDK, Linux glibc 2.28+

---

## Testing

Unit tests cover:
- Wallet open/close with various configs
- Balance queries (native + custom assets)
- Transfers: valid, insufficient funds, invalid address, invalid asset
- Swap: create → get_info → accept roundtrip
- Alias: register, update, transfer, lookup, cost
- Staking: start, stop, get_info
- Error code propagation
- Fuzzing: malformed inputs, oversized buffers

Run with: `ctest --output-on-failure -j$(nproc)`

---

## Example Usage (C)

```c
#include "pdc_wallet.h"
#include <stdio.h>

int main() {
    pdc_wallet_config_t config = {
        .wallet_path = "/path/to/wallet.wallet",
        .password = "secret",
        .daemon_address = "http://127.0.0.1:19211"
    };
    
    pdc_wallet_t* wallet = NULL;
    int err = pdc_wallet_open(&config, &wallet);
    if (err != PDC_OK) {
        fprintf(stderr, "Open failed: %s\n", pdc_strerror(err));
        return 1;
    }
    
    char address[106];
    pdc_wallet_get_address(wallet, address, sizeof(address));
    printf("Address: %s\n", address);
    
    pdc_asset_balance_t balances[16];
    size_t count = pdc_wallet_get_balances(wallet, balances, 16);
    for (size_t i = 0; i < count; i++) {
        printf("Asset: %s, Unlocked: %llu, Locked: %llu\n", 
               balances[i].ticker, balances[i].unlocked, balances[i].locked);
    }
    
    pdc_wallet_close(wallet);
    return 0;
}
```

---

## Next Steps (for v0.2.0)

- [ ] Multi-wallet support (handle array)
- [ ] Async/callback variants for long operations
- [ ] Watch-only wallet mode
- [ ] Hardware wallet integration (Ledger/Trezor)
- [ ] Transaction history pagination
- [ ] Subaddress support
- [ ] Multisig support