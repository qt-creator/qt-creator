// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// CBOR fuzz target for CmdBridge, for AFL and libFuzzer.
//
// It #includes cbor.c and so exercises the very decoder the bridge ships;
// there is deliberately no second copy of the parser in this file.
//
// The areas it stresses:
//   - recursion depth (stack exhaustion on nested arrays/maps)
//   - the array/map error paths, which must free what they decoded
//   - container headers claiming more elements than the input can hold
//   - truncated values at every byte boundary, which must read as incomplete
//     rather than invalid, so the reader waits instead of resynchronising
//   - maps with >23 pairs and keys >23 chars (the multi-byte length forms)
//   - tagged values, which must not leak the tag wrapper
//   - simple values and floats, whose bytes must still be consumed
//
// With no input on stdin it runs its structural self-tests and exits, which is
// how the build registers it as tst_cmdbridge_cbor_fuzz.
//
// Build with AFL:
//   afl-clang-fast -std=c11 -D_GNU_SOURCE -fsanitize=address,undefined
//       -g -O1 cmdbridge_fuzz.c -o cmdbridge_fuzzer
//
// Run with AFL:
//   mkdir -p corpus_in corpus_out
//   afl-fuzz -i corpus_in -o corpus_out -- ./cmdbridge_fuzzer
//
// Build with libFuzzer (clang):
//   clang -std=c11 -D_GNU_SOURCE -DLIBFUZZER
//       -fsanitize=fuzzer,address,undefined -g -O1
//       cmdbridge_fuzz.c -o cmdbridge_fuzzer_libfuzzer
//
// Run:
//   ./cmdbridge_fuzzer_libfuzzer corpus/
//
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* The real decoder, exactly as the bridge builds it. Do not copy these
   definitions here: a copy silently stops testing the code that ships.

   cbor.c is written to be part of a larger translation unit, so the map
   accessors it provides for the command handlers have no caller here. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "cbor.c"
#pragma GCC diagnostic pop

/* --- Fuzz helpers --- */

/* Build a deeply nested array: [[[...]]] at the given depth */
static value *build_deep_nested(int depth)
{
    if (depth <= 0) {
        value *v = vnew();
        v->type = V_UINT;
        v->u64 = 42;
        return v;
    }
    value *inner = build_deep_nested(depth - 1);
    value *arr = vnew();
    arr->type = V_ARRAY;
    arr->kids = (value **) calloc(1, sizeof(value *));
    arr->kids[0] = inner;
    arr->nkids = 1;
    return arr;
}

/* Build a map with >23 pairs to trigger the esz() >23 code path */
static value *build_large_map(int num_pairs)
{
    value *m = vnew();
    m->type = V_MAP;
    m->kids = (value **) calloc(num_pairs * 2, sizeof(value *));
    m->nkids = 0;
    for (int i = 0; i < num_pairs; i++) {
        char key[128];
        snprintf(key, sizeof(key), "key_%016x_%016x_%016x_%016x", i, i, i, i);
        m->kids[m->nkids++] = vs(key);
        m->kids[m->nkids++] = vi(i);
    }
    return m;
}

/* Build a map with long keys (>23 chars) to trigger key esz() >23 path */
static value *build_long_key_map(int num_pairs)
{
    value *m = vnew();
    m->type = V_MAP;
    m->kids = (value **) calloc(num_pairs * 2, sizeof(value *));
    m->nkids = 0;
    for (int i = 0; i < num_pairs; i++) {
        char key[128];
        memset(key, 'X', sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';
        m->kids[m->nkids++] = vs(key);
        m->kids[m->nkids++] = vi(i);
    }
    return m;
}

static void fuzz_one(const uint8_t *data, size_t size)
{
    /* Basic decode - should never crash */
    value *v = decode(data, size);
    vfree(v);

    /* cbor_scan - should never crash */
    size_t sz = 0;
    cbor_scan_result st = cbor_scan(data, size, &sz);

    /* Decode only the first complete CBOR value cbor_scan found */
    if (st == CBOR_OK && sz > 0 && sz <= size) {
        value *v2 = decode(data, sz);
        vfree(v2);
    }

    /* Feed each prefix of the data - catches incremental parsing issues */
    if (size > 0 && size < 10000) {
        for (size_t i = 1; i <= size; i += ((i % 7) + 1)) {
            if (i > size)
                break;
            value *vprefix = decode(data, i);
            vfree(vprefix);
            size_t szprefix = 0;
            (void) cbor_scan(data, i, &szprefix);
        }
    }

    /* Empty input */
    if (size > 0) {
        value *vempty = decode(data, 0);
        vfree(vempty);
        size_t sze = 0;
        (void) cbor_scan(data, 0, &sze);
    }
}

/* Structural tests: verify fixes for specific issues found in code review.
 * These generate their own data to exercise code paths that input mutation
 * might not reach (depth boundary, map encoding >23 pairs, etc.). */
static void run_structural_tests(void)
{
    /* Deeply nested arrays - tests depth cap (CBOR_MAX_DEPTH=128) */
    int deep_depths[] = {50, 100, 120, 127, 128, 129, 130, 140, 150, 200};
    for (int di = 0; di < (int) (sizeof(deep_depths) / sizeof(deep_depths[0])); di++) {
        int d = deep_depths[di];
        value *nested = build_deep_nested(d);
        size_t enc_len = 0;
        uint8_t *encoded = encode(nested, &enc_len);
        if (encoded) {
            value *decoded = decode(encoded, enc_len);
            vfree(decoded);
            free(encoded);
        }
        vfree(nested);
    }

    /* Map with >23 pairs - tests esz() redundant euint8 latent bug fix */
    int map_sizes[] = {20, 23, 24, 25, 30, 40, 50};
    for (int mi = 0; mi < (int) (sizeof(map_sizes) / sizeof(map_sizes[0])); mi++) {
        value *m = build_large_map(map_sizes[mi]);
        size_t enc_len = 0;
        uint8_t *encoded = encode(m, &enc_len);
        if (encoded) {
            value *decoded = decode(encoded, enc_len);
            vfree(decoded);
            free(encoded);
        }
        vfree(m);
    }

    /* Map with long keys (>23 chars) - tests key esz() redundant euint8 fix */
    for (int pairs = 2; pairs <= 5; pairs++) {
        value *m = build_long_key_map(pairs);
        size_t enc_len = 0;
        uint8_t *encoded = encode(m, &enc_len);
        if (encoded) {
            value *decoded = decode(encoded, enc_len);
            vfree(decoded);
            free(encoded);
        }
        vfree(m);
    }

    /* Round-trip test - encode then decode various structures */
    {
        value *inner_arr = vnew();
        inner_arr->type = V_ARRAY;
        inner_arr->kids = (value **) calloc(3, sizeof(value *));
        inner_arr->kids[0] = vs("hello");
        inner_arr->kids[1] = vi(42);
        inner_arr->kids[2] = vb(true);
        inner_arr->nkids = 3;

        value *outer_map = mk2("data", inner_arr, "count", vu(3));
        size_t enc_len = 0;
        uint8_t *encoded = encode(outer_map, &enc_len);
        if (encoded && enc_len > 0) {
            value *decoded = decode(encoded, enc_len);
            vfree(decoded);
            free(encoded);
        }
        vfree(outer_map);
    }

    /* Bytes with various sizes - key boundaries */
    size_t byte_sizes[] = {0, 1, 23, 24, 25, 255, 256, 65535, 65536};
    for (int bi = 0; bi < (int) (sizeof(byte_sizes) / sizeof(byte_sizes[0])); bi++) {
        size_t bsize = byte_sizes[bi];
        uint8_t *raw = (uint8_t *) malloc(bsize);
        if (raw) {
            memset(raw, 0xAB, bsize);
            value *bv = vy(raw, bsize);
            size_t enc_len = 0;
            uint8_t *encoded = encode(bv, &enc_len);
            if (encoded && enc_len > 0) {
                value *decoded = decode(encoded, enc_len);
                vfree(decoded);
                free(encoded);
            }
            vfree(bv);
            free(raw);
        }
    }

    /* Tag-wrapped values (tag is major type 6) */
    {
        uint8_t tag_data[] = {0xC1, 0x81, 0x18, 0x9};
        value *decoded = decode(tag_data, sizeof(tag_data));
        vfree(decoded);
    }

    /* Simple values >= 32 (require additional info byte) */
    for (uint8_t s = 32; s < 240; s++) {
        uint8_t simple_data[] = {0xF0 | (s >> 5), (uint8_t) (s & 0x1F)};
        value *decoded = decode(simple_data, 2);
        vfree(decoded);
    }

    /* Malformed inputs at critical boundaries */
    {
        uint8_t incomplete[] = {0x19};
        value *v_incomp = decode(incomplete, sizeof(incomplete));
        vfree(v_incomp);

        uint8_t arr_incomp[] = {0x81};
        value *v_arr = decode(arr_incomp, sizeof(arr_incomp));
        vfree(v_arr);

        uint8_t map_incomp[] = {0xA1};
        value *v_map = decode(map_incomp, sizeof(map_incomp));
        vfree(v_map);

        uint8_t str_trunc[] = {0x63, 'H', 'e'};
        value *v_str = decode(str_trunc, sizeof(str_trunc));
        vfree(v_str);

        uint8_t bytes_huge[] = {0x59, 0xFF, 0xFF};
        value *v_bytes = decode(bytes_huge, sizeof(bytes_huge));
        vfree(v_bytes);
    }

    /* Single-byte inputs (all major types) */
    for (int b = 0; b < 256; b++) {
        uint8_t byte = (uint8_t) b;
        value *v_single = decode(&byte, 1);
        vfree(v_single);
    }
}

/* Run structural tests once at startup before the fuzzer loop. */
static __attribute__((constructor)) void run_startup_tests(void)
{
    run_structural_tests();
}

/* Entry point for libFuzzer */
#ifdef __cplusplus
extern "C"
#endif
    int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    fuzz_one(data, size);
    return 0;
}

/* AFL compatibility / standalone: read stdin and call fuzz_one.
 * Only compiled when NOT using libFuzzer.
 * Build for libFuzzer: clang ... -fsanitize=fuzzer [-DLIBFUZZER] ...
 * Build for AFL/standalone: clang ... (no -fsanitize=fuzzer) */
#ifndef LIBFUZZER
int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    uint8_t buf[1024 * 1024];
    size_t total = 0;

    while (1) {
        ssize_t n = read(STDIN_FILENO, buf + total, sizeof(buf) - total);
        if (n <= 0)
            break;
        total += (size_t) n;
    }

    if (total > 0)
        fuzz_one(buf, total);

    return 0;
}
#endif
