#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}
int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    uint32_t hi  = (uint32_t)(challenge >> 32);
    uint32_t lo  = (uint32_t)challenge;
    uint32_t mid = (uint32_t)(challenge >> 16);

    uint32_t t = (5u * hi) ^ (lo + 0xbae6u);
    uint32_t c = 3u * (lo - mid) + 0xa42cu;
    uint32_t answer = (c + 8u * t) ^ 0xa57u;

    values[0] = answer;
    count = 1;

    return write_answer(out, cap, values, count);
}
int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    values[0] = (uint32_t)((challenge >> 9) & 0xFF);

    for (int i = 1; i <= 5; i++) {
        uint32_t cl = 13u + 4u * (uint32_t)(i - 1);
        uint32_t k  = (uint32_t)((challenge >> cl) & 0xF);
        values[i] = (values[i - 1] + k) ^ (uint32_t)(3 * i);
    }
    count = 6;

    return write_answer(out, cap, values, count);
}
static uint32_t p3_case(uint32_t idx, uint32_t x) {
    switch (idx & 7) {
    case 0: return 3u * x + 0x4a34u;              // p3_case_6
    case 1: return (x ^ 0xe699u) + (x >> 2);      // p3_case_7
    case 2: return (2u * x) ^ 0x7c1bu;            // p3_case_2
    case 3: return (x ^ 0x2c4bu) + (x >> 1);      // p3_case_0
    case 4: return 5u * x + 0x8e70u;              // p3_case_1
    case 5: return (2u * x) ^ 0xe13cu;            // p3_case_5
    case 6: return 5u * x + 0x19627u;             // p3_case_4
    case 7: return 5u * x + 0x1699au;             // p3_case_3
    }
    return 0;
}
int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    uint32_t idx = (uint32_t)(challenge & 7);
    uint32_t x   = (uint32_t)((challenge >> 32) ^ (challenge >> 3));

    values[0] = idx;
    values[1] = p3_case(idx, x);
    count = 2;

    return write_answer(out, cap, values, count);
}
static uint32_t p4_walk(uint32_t target, uint32_t lo, uint32_t hi, uint32_t key) {
    uint32_t mid = lo + (hi - lo) / 2;
    if (mid == target) {
        return (mid + 0x6f) ^ key;
    }
    if (target < mid) {
        uint32_t r = p4_walk(target, lo, mid - 1, key);
        return (5 * r) ^ (mid + key) ^ 0x16;
    } else {
        uint32_t r = p4_walk(target, mid + 1, hi, key);
        return (5 * r) ^ (mid + key) ^ 0x7;
    }
}
int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    uint32_t target = (uint32_t)(challenge & 0x7f);
    uint32_t lo = 0;
    uint32_t hi = 0x7f;
    uint32_t key = (uint32_t)((challenge >> 32) ^ (challenge >> 7));

    values[0] = p4_walk(target, lo, hi, key);
    count = 1;

    return write_answer(out, cap, values, count);
}

struct p5_node {
    uint32_t f0;
    uint32_t f1;
    const struct p5_node *next;
};

static const struct p5_node n_ba0 = { 0x6f6b, 0x774,  NULL  };
static const struct p5_node n_c20 = { 0xebca, 0x62a,  &n_ba0 };
static const struct p5_node n_c40 = { 0xbb,   0x56a,  &n_c20 };
static const struct p5_node n_bc0 = { 0x8bb7, 0xa77,  &n_c40 };
static const struct p5_node n_bd0 = { 0xb5c,  0x910,  &n_bc0 };
static const struct p5_node n_bb0 = { 0x8cfb, 0xb6b,  &n_bd0 };
static const struct p5_node n_c10 = { 0xaef1, 0x77,   &n_bb0 };
static const struct p5_node n_be0 = { 0x57f8, 0x310,  &n_c10 };
static const struct p5_node n_c00 = { 0xe905, 0x228,  &n_be0 };
static const struct p5_node n_c50 = { 0x6822, 0x81e,  &n_c00 };
static const struct p5_node n_c30 = { 0xbd31, 0x139,  &n_c50 };
static const struct p5_node n_bf0 = { 0xe308, 0x435,  &n_c30 };

static const struct p5_node *const p5_heads[8] = {
    &n_bf0,  /* idx 0 */
    &n_c30,  /* idx 1 */
    &n_c50,  /* idx 2 */
    &n_c00,  /* idx 3 */
    &n_be0,  /* idx 4 */
    &n_c10,  /* idx 5 */
    &n_bb0,  /* idx 6 */
    &n_bd0,  /* idx 7 */
};

static uint32_t p5_traverse(uint64_t challenge, uint32_t *out) {
    uint32_t idx = (uint32_t)(challenge & 7);
    const struct p5_node *p = p5_heads[idx];

    uint32_t acc = (uint32_t)((challenge >> 6) ^ (challenge >> 32))
                 ^ p->f0 ^ 0x8574u;
    acc += p->f1;
    *out = p->f1;

    uint32_t steps = (uint32_t)((challenge >> 3) & 3);
    for (uint32_t i = 1; i <= steps; i++) {
        p = p->next;
        acc ^= p->f0;
        acc += p->f1;
        *out = p->f1;
    }
    return acc;
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    uint32_t v1 = 0;
    uint32_t v0 = p5_traverse(challenge, &v1);

    values[0] = v0;
    values[1] = v1;
    count = 2;

    return write_answer(out, cap, values, count);
}

static uint32_t p6_rol(uint32_t x, unsigned n) {
    n &= 31;
    return n ? (x << n) | (x >> (32 - n)) : x;
}
static uint32_t p6_ror(uint32_t x, unsigned n) {
    n &= 31;
    return n ? (x >> n) | (x << (32 - n)) : x;
}

static uint32_t p6_op(uint32_t idx, uint32_t x) {
    switch (idx & 7) {
    case 0: return (x ^ 0xb034u) + (x << 7);            /* p6_op_5 */
    case 1: return (x ^ 0xb034u) + (x << 7);            /* p6_op_5 */
    case 2: return (x ^ 0xb034u) + (x << 7);            /* p6_op_5 */
    case 3: return p6_rol(x, 7) ^ 0xe9f0u;              /* p6_op_3 */
    case 4: return p6_ror(x, 1);                        /* p6_op_4 */
    case 5: return ((x >> 5) ^ x) + 0x2150u;            /* p6_op_0 */
    case 6: return (x + 0x8aeu) ^ (x >> 1);             /* p6_op_2 */
    case 7: return (2u * x + 0x5fb2u) ^ (3u * x);       /* p6_op_1 */
    }
    return 0;
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    uint32_t idx1 = (uint32_t)(challenge & 7);
    uint32_t idx2 = (uint32_t)((challenge & 0x38) >> 3);
    uint32_t x    = (uint32_t)((challenge >> 6) ^ (challenge >> 32));

    uint32_t r = p6_op(idx1, x);
    r ^= 0xe194u;
    r = p6_op(idx2, r);
    r += 0xcae4u;

    values[0] = r;
    count = 1;

    return write_answer(out, cap, values, count);
}
static uint32_t s_ror32(uint32_t x, unsigned n) {
    n &= 31;
    return n ? (x >> n) | (x << (32 - n)) : x;
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t c0 = (uint32_t)(context & 3);
    uint32_t x  = (uint32_t)(challenge ^ (challenge >> 32));
    uint32_t y  = (uint32_t)(context ^ (context >> 32));
    uint32_t z  = (uint32_t)(context >> 16);
    uint32_t w  = (0x863cu * c0) ^ z;

    uint32_t mix_in = x ^ y;
    uint32_t b = (s_ror32(mix_in, 5) ^ 0x7416u) ^ w;
    uint32_t c2 = b ^ (y + c0 + 0xf22au + (x >> 5));

    /* 前面 "OPEN" + 填充，保证总长度 >= 36 */
    int n = snprintf(out, cap, "OPEN%*s%u %u %u", 32, "", c0, b, c2);
    return n < 0 || (size_t)n >= cap;
}