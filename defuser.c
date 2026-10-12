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

/* ---------- 第 1 关：p1_compute = p1_step2(p1_step0(lo, hi), p1_step1(hi, mid)) ---------- */

static uint32_t p1_step0(uint32_t a, uint32_t b) { return (a * 5u + b) ^ 0x7866u; }
static uint32_t p1_step1(uint32_t a, uint32_t b) { return (b + 0x2317u) ^ a; }
static uint32_t p1_step2(uint32_t a, uint32_t b) { return (a - b) * 3u + 0x69a1u; }

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t lo = (uint32_t)challenge;
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t t1 = p1_step1(hi, mid);
    uint32_t t0 = p1_step0(lo, hi);
    values[0] = p1_step2(t0, t1);
    return write_answer(out, cap, values, count);
}

/* ---------- 第 2 关：6 项递推 a[0] = bits ^ 1，a[i] = 3 * ((bits_i + i) ^ a[i-1]) ---------- */

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;
    values[0] = (uint32_t)((challenge >> 16) & 0x3f) ^ 1u;
    for (unsigned i = 1; i < 6; ++i) {
        uint32_t bits = (uint32_t)((challenge >> (13 + i)) & 0x3f);
        values[i] = ((bits + i) ^ values[i - 1]) * 3u;
    }
    return write_answer(out, cap, values, count);
}

/* ---------- 第 3 关：跳转表 0x4057e0，下标 challenge & 7 ---------- */

static uint32_t p3_case(unsigned which, uint32_t x) {
    switch (which) {
    case 0: return x * 5u + 0x3594eu;
    case 1: return (x * 4u) ^ 0xe841u;
    case 2: return (x ^ 0x4d29u) + (x >> 1);
    case 3: return x * 3u + 0xff0eu;
    case 4: return x * 3u + 0x2ba0u;
    case 5: return (x * 4u) ^ 0xb6b7u;
    case 6: return (x ^ 0xe534u) + (x >> 2);
    default: return x * 5u + 0x4e29bu;
    }
}

/* 跳转表第 i 项实际跳到的 p3_case_N */
static const unsigned p3_jump_table[8] = {4, 2, 6, 1, 5, 3, 7, 0};

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    uint32_t index = (uint32_t)challenge & 7u;
    uint32_t x = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    values[0] = index;
    values[1] = p3_case(p3_jump_table[index], x);
    return write_answer(out, cap, values, count);
}

/* ---------- 第 4 关：在 [0, 127] 上二分查找 target 的递归 p4_walk ---------- */

static uint32_t p4_walk(uint32_t target, uint32_t lo, uint32_t hi, uint32_t key) {
    uint32_t mid = lo + ((hi - lo) >> 1);
    if (mid == target) return (mid + 0xc4u) ^ key;
    uint32_t here = mid + key;
    if (target < mid) return ((p4_walk(target, lo, mid - 1, key) * 3u) ^ here) ^ 0x17u;
    return ((p4_walk(target, mid + 1, hi, key) * 3u) ^ here) ^ 0x4u;
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t target = (uint32_t)challenge & 0x7fu;
    uint32_t key = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    values[0] = p4_walk(target, 0, 0x7f, key);
    return write_answer(out, cap, values, count);
}

/* ---------- 第 5 关：链表节点 { next(+0x0), value(+0x8), key(+0xc) } ---------- */

struct p5_node {
    uint32_t value;
    uint32_t key;
};

/* 炸弹中按 next 指针串起来的 12 个节点（0x405c10 → 0x405b70 → … → 0x405ba0 → NULL） */
static const struct p5_node p5_chain[12] = {
    {0x47a, 0xf12c}, {0x856, 0x5cbb}, {0x037, 0x8afc}, {0x36c, 0x1290},
    {0x12b, 0x7a9e}, {0xb7b, 0x2550}, {0x60b, 0xab7f}, {0xa8a, 0x5e22},
    {0x713, 0x78e8}, {0x913, 0x7401}, {0x219, 0x9ac1}, {0x530, 0x5d83},
};

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    /* 表 0x405b20 的第 i 项恰好指向链上第 i 个节点 */
    unsigned pos = (unsigned)(challenge & 7);
    unsigned steps = (unsigned)((challenge >> 3) & 3);
    uint32_t acc = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    acc = (acc ^ p5_chain[pos].key ^ 0x8b6fu) + p5_chain[pos].value;
    for (unsigned i = 0; i < steps; ++i) {
        ++pos;
        acc = (acc ^ p5_chain[pos].key) + p5_chain[pos].value;
    }
    values[0] = acc;
    values[1] = p5_chain[pos].value;
    return write_answer(out, cap, values, count);
}

/* ---------- 第 6 关：函数指针表 0x405dc0，两次间接调用 ---------- */

static uint32_t rotr32(uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }
static uint32_t rotl32(uint32_t x, unsigned n) { return (x << n) | (x >> (32 - n)); }

static uint32_t p6_op_0(uint32_t x) { return (x + 0x3feu) ^ (x >> 3); }
static uint32_t p6_op_1(uint32_t x) { return (x ^ 0x4051u) + x * 8u; }
static uint32_t p6_op_2(uint32_t x) { return rotr32(x, 3); }
static uint32_t p6_op_3(uint32_t x) { return rotl32(x, 7) ^ 0xc374u; }
static uint32_t p6_op_4(uint32_t x) { return ((x << 7) + 0xc55u) ^ (x * 7u); }
static uint32_t p6_op_5(uint32_t x) { return ((x >> 5) ^ x) + 0x6aa1u; }

static uint32_t (*const p6_table[8])(uint32_t) = {
    p6_op_5, p6_op_3, p6_op_0, p6_op_4, p6_op_4, p6_op_1, p6_op_1, p6_op_2,
};

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t first = p6_table[challenge & 7](x) ^ 0x709bu;
    values[0] = p6_table[(challenge >> 3) & 7](first) + 0x793cu;
    return write_answer(out, cap, values, count);
}

/* ---------- Honor：secret_gate 把整行复制进局部结构体，偏移 32 处须为 "OPEN" ---------- */

static uint32_t secret_mix(uint32_t x) { return rotr32(x, 5) ^ 0x7175u; }

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t first = (uint32_t)context & 3u;
    uint32_t c = (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;
    uint32_t k = (uint32_t)(context >> 32) ^ (uint32_t)context;
    uint32_t salt = (first * 0x4854u) ^ (uint32_t)(context >> 16);
    uint32_t second = secret_mix(c ^ k) ^ salt;
    uint32_t third = second ^ (k + first + 0x3921u + (c >> 5));
    /* 前 32 个字节填满结构体中的第一个字段，紧接着的 4 字节覆盖成 "OPEN"，偏移 36 起是三个整数 */
    int size = snprintf(out, cap, "%s%s%" PRIu32 " %" PRIu32 " %" PRIu32,
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", "OPEN", first, second, third);
    return size < 0 || (size_t)size >= cap;
}
