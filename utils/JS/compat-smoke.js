/**
 * Offline compatibility smoke for utils/JS Dependabot bumps.
 * Exercises the axios + ethers surface used by test_eth_sig.js (no daemon required).
 *
 * Exit 0 = OK. Exit 1 = incompatible / broken.
 */
'use strict';

const assert = require('assert');
const axios = require('axios');
const { ethers } = require('ethers');

function main() {
  // axios — HTTP client used by callJsonRpc
  assert.strictEqual(typeof axios.post, 'function', 'axios.post missing');
  assert.ok(axios.VERSION || axios.default?.VERSION, 'axios VERSION missing');

  // ethers — Wallet + signingKey + getBytes used by ETH asset deploy/sign flow
  const pk =
    '0x17a938099954cee510d7fc9eb2366f0762b093d9be547acabf8be85f774ef154';
  const wallet = new ethers.Wallet(pk);
  assert.ok(wallet.address && wallet.address.startsWith('0x'), 'Wallet.address');
  assert.strictEqual(
    wallet.address.toLowerCase(),
    '0x0886ba9f5b117d2a3c1ce18106f2ce759f5d34c8',
    'Wallet address drift (breaking ethers change?)'
  );
  assert.ok(
    wallet.signingKey?.compressedPublicKey?.startsWith('0x'),
    'signingKey.compressedPublicKey'
  );
  const pubHex = wallet.signingKey.compressedPublicKey.substring(2);
  assert.strictEqual(pubHex.length, 66, 'compressed pubkey hex length');

  const digest = ethers.getBytes(
    '0x' + 'ab'.repeat(32)
  );
  assert.strictEqual(digest.length, 32, 'getBytes length');
  const sig = wallet.signingKey.sign(digest);
  assert.ok(sig.serialized && sig.serialized.startsWith('0x'), 'sign().serialized');
  const without0x = sig.serialized.substring(2);
  const stripped = without0x.slice(0, -2);
  assert.strictEqual(stripped.length, 128, '65-byte sig minus recovery → 64 bytes hex');

  const axiosVer = axios.VERSION || axios.default.VERSION;
  console.log(
    JSON.stringify(
      {
        ok: true,
        axios: axiosVer,
        ethers: ethers.version,
        address: wallet.address,
      },
      null,
      2
    )
  );
}

try {
  main();
} catch (err) {
  console.error('COMPAT SMOKE FAILED:', err);
  process.exit(1);
}
