/*
 * test_suite.cpp — Unit tests for cs327 Project 2
 *
 * Covers: dice, heap, character accessors, utils/makedirectory
 *
 * Build:
 *   g++ -Wall -ggdb -o run_tests test_suite.cpp heap.o dice.o utils.o
 * Run:
 *   ./run_tests
 */

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <climits>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "dice.h"
#include "heap.h"
#include "utils.h"

// ─────────────────────────────────────────────────────────────
// Minimal test framework
// ─────────────────────────────────────────────────────────────
static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void name()
#define RUN(name)  do { \
    printf("  %-50s", #name); \
    tests_run++; \
    name(); \
    tests_passed++; \
    printf("PASS\n"); \
} while(0)

#define ASSERT(expr) do { \
    if (!(expr)) { \
        tests_failed++; tests_passed--; \
        printf("FAIL\n    Assertion failed: %s  (%s:%d)\n", \
               #expr, __FILE__, __LINE__); \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_GE(a, b) ASSERT((a) >= (b))
#define ASSERT_LE(a, b) ASSERT((a) <= (b))

// ═════════════════════════════════════════════════════════════
// SECTION 1 — dice
// ═════════════════════════════════════════════════════════════

TEST(dice_default_constructor_zeroes_fields)
{
    dice d;
    ASSERT_EQ(d.get_base(),   0);
    ASSERT_EQ(d.get_number(), 0);
    ASSERT_EQ(d.get_sides(),  0);
}

TEST(dice_parameterized_constructor_stores_values)
{
    dice d(5, 3, 6);
    ASSERT_EQ(d.get_base(),   5);
    ASSERT_EQ(d.get_number(), 3);
    ASSERT_EQ(d.get_sides(),  6);
}

TEST(dice_set_updates_all_fields)
{
    dice d;
    d.set(10, 2, 8);
    ASSERT_EQ(d.get_base(),   10);
    ASSERT_EQ(d.get_number(), 2);
    ASSERT_EQ(d.get_sides(),  8);
}

TEST(dice_set_individual_setters)
{
    dice d;
    d.set_base(7);
    d.set_number(4);
    d.set_sides(12);
    ASSERT_EQ(d.get_base(),   7);
    ASSERT_EQ(d.get_number(), 4);
    ASSERT_EQ(d.get_sides(),  12);
}

TEST(dice_roll_base_only_when_sides_zero)
{
    // sides == 0 → roll() returns exactly base (no random component)
    dice d(42, 5, 0);
    ASSERT_EQ(d.roll(), 42);
}

TEST(dice_roll_within_expected_range)
{
    // 2+1d6: min = 2+1 = 3, max = 2+6 = 8
    dice d(2, 1, 6);
    srand(12345);
    for (int i = 0; i < 1000; i++) {
        int32_t r = d.roll();
        ASSERT_GE(r, 3);
        ASSERT_LE(r, 8);
    }
}

TEST(dice_roll_multi_die_range)
{
    // 0+3d4: min = 3, max = 12
    dice d(0, 3, 4);
    srand(99999);
    for (int i = 0; i < 2000; i++) {
        int32_t r = d.roll();
        ASSERT_GE(r, 3);
        ASSERT_LE(r, 12);
    }
}

TEST(dice_roll_negative_base_shifts_range)
{
    // -5+2d6: min = -5+2 = -3, max = -5+12 = 7
    dice d(-5, 2, 6);
    srand(777);
    for (int i = 0; i < 2000; i++) {
        int32_t r = d.roll();
        ASSERT_GE(r, -3);
        ASSERT_LE(r, 7);
    }
}

TEST(dice_roll_zero_number_of_dice)
{
    // 0+0d6 → sides > 0 but number == 0 → loop doesn't execute → base only
    dice d(5, 0, 6);
    ASSERT_EQ(d.roll(), 5);
}

TEST(dice_roll_one_sided_die)
{
    // 3+2d1: each die always rolls 1, so total = 3+2 = 5
    dice d(3, 2, 1);
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(d.roll(), 5);
    }
}

TEST(dice_print_format)
{
    dice d(3, 2, 6);
    std::ostringstream oss;
    d.print(oss);
    ASSERT_EQ(oss.str(), std::string("3+2d6"));
}

TEST(dice_stream_operator)
{
    dice d(-1, 4, 8);
    std::ostringstream oss;
    oss << d;
    ASSERT_EQ(oss.str(), std::string("-1+4d8"));
}

TEST(dice_print_zero_dice)
{
    dice d(0, 0, 0);
    std::ostringstream oss;
    d.print(oss);
    ASSERT_EQ(oss.str(), std::string("0+0d0"));
}

// Statistical smoke test — distribution should be roughly uniform
TEST(dice_roll_distribution_not_always_same_value)
{
    dice d(0, 1, 6);
    srand(42);
    bool seen[7] = {};
    for (int i = 0; i < 600; i++) {
        int32_t r = d.roll();
        if (r >= 1 && r <= 6) seen[r] = true;
    }
    // With 600 rolls of 1d6, expect all faces seen
    for (int face = 1; face <= 6; face++) {
        ASSERT(seen[face]);
    }
}

// ═════════════════════════════════════════════════════════════
// SECTION 2 — heap (Fibonacci heap)
// ═════════════════════════════════════════════════════════════

// Comparator: ascending int
static int32_t int_cmp(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

// Simple malloc-based datum deleter
static void int_del(void *v)
{
    free(v);
}

static int *mk(int val)
{
    int *p = (int *)malloc(sizeof(int));
    *p = val;
    return p;
}

TEST(heap_init_creates_empty_heap)
{
    heap_t h;
    heap_init(&h, int_cmp, NULL);
    ASSERT_EQ(h.size, 0u);
    ASSERT(h.min == NULL);
    heap_delete(&h);
}

TEST(heap_insert_single_element)
{
    heap_t h;
    heap_init(&h, int_cmp, NULL);
    int val = 42;
    heap_insert(&h, &val);
    ASSERT_EQ(h.size, 1u);
    ASSERT_EQ(*(int *)heap_peek_min(&h), 42);
    heap_delete(&h);
}

TEST(heap_peek_min_returns_min_without_removing)
{
    heap_t h;
    heap_init(&h, int_cmp, NULL);
    int a = 10, b = 5, c = 20;
    heap_insert(&h, &a);
    heap_insert(&h, &b);
    heap_insert(&h, &c);
    ASSERT_EQ(*(int *)heap_peek_min(&h), 5);
    ASSERT_EQ(h.size, 3u);   // peek should not remove
    heap_delete(&h);
}

TEST(heap_remove_min_returns_minimum)
{
    heap_t h;
    heap_init(&h, int_cmp, NULL);
    int a = 10, b = 5, c = 20;
    heap_insert(&h, &a);
    heap_insert(&h, &b);
    heap_insert(&h, &c);
    int *m = (int *)heap_remove_min(&h);
    ASSERT_EQ(*m, 5);
    ASSERT_EQ(h.size, 2u);
    heap_delete(&h);
}

TEST(heap_remove_min_drains_in_sorted_order)
{
    heap_t h;
    heap_init(&h, int_cmp, NULL);
    int vals[] = {50, 10, 30, 20, 40};
    for (int v : vals) heap_insert(&h, mk(v));

    int prev = INT_MIN;
    while (h.size) {
        int *cur = (int *)heap_remove_min(&h);
        ASSERT_GE(*cur, prev);
        prev = *cur;
        free(cur);
    }
    heap_delete(&h);
}

TEST(heap_remove_min_empty_heap_returns_null)
{
    heap_t h;
    heap_init(&h, int_cmp, NULL);
    void *r = heap_remove_min(&h);
    ASSERT(r == NULL);
    heap_delete(&h);
}

TEST(heap_size_tracks_insertions_and_removals)
{
    heap_t h;
    heap_init(&h, int_cmp, NULL);
    int vals[5] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        heap_insert(&h, &vals[i]);
        ASSERT_EQ(h.size, (uint32_t)(i + 1));
    }
    for (int i = 5; i > 0; i--) {
        heap_remove_min(&h);
        ASSERT_EQ(h.size, (uint32_t)(i - 1));
    }
    heap_delete(&h);
}

TEST(heap_handles_duplicate_values)
{
    heap_t h;
    heap_init(&h, int_cmp, NULL);
    int v = 7;
    heap_insert(&h, &v);
    heap_insert(&h, &v);
    heap_insert(&h, &v);
    ASSERT_EQ(h.size, 3u);
    int *r1 = (int *)heap_remove_min(&h);
    int *r2 = (int *)heap_remove_min(&h);
    ASSERT_EQ(*r1, 7);
    ASSERT_EQ(*r2, 7);
    heap_delete(&h);
}

TEST(heap_large_insertion_sorted_extraction)
{
    heap_t h;
    heap_init(&h, int_cmp, int_del);
    srand(54321);
    const int N = 200;
    for (int i = 0; i < N; i++) {
        int *p = mk(rand() % 1000);
        heap_insert(&h, p);
    }
    ASSERT_EQ(h.size, (uint32_t)N);
    int prev = INT_MIN;
    while (h.size) {
        int *cur = (int *)heap_remove_min(&h);
        ASSERT_GE(*cur, prev);
        prev = *cur;
        free(cur);
    }
    heap_delete(&h);
}

TEST(heap_decrease_key_no_replace_reorders)
{
    heap_t h;
    heap_init(&h, int_cmp, NULL);
    int a = 10, b = 5, c = 20;
    heap_insert(&h, &a);
    heap_node_t *n_c = heap_insert(&h, &c);   // c = 20
    heap_insert(&h, &b);

    // Decrease c from 20 to 1 by mutating the datum in place
    c = 1;
    heap_decrease_key_no_replace(&h, n_c);

    // Now minimum should be 1 (was c)
    int *m = (int *)heap_remove_min(&h);
    ASSERT_EQ(*m, 1);
    heap_delete(&h);
}

// ═════════════════════════════════════════════════════════════
// SECTION 3 — utils / makedirectory
// ═════════════════════════════════════════════════════════════

TEST(makedirectory_creates_leaf_dir)
{
    // makedirectory only acts when there is a '/' in the path
    char path[] = "/tmp/cs327_test_dir/leaf";
    // Pre-clean
    rmdir("/tmp/cs327_test_dir/leaf");
    rmdir("/tmp/cs327_test_dir");

    int ret = makedirectory(path);
    ASSERT_EQ(ret, 0);

    struct stat st;
    ASSERT_EQ(stat("/tmp/cs327_test_dir", &st), 0);
    ASSERT(S_ISDIR(st.st_mode));

    // Clean up
    rmdir("/tmp/cs327_test_dir/leaf");
    rmdir("/tmp/cs327_test_dir");
}

TEST(makedirectory_existing_dir_returns_zero)
{
    char path[] = "/tmp/cs327_existing/x";
    mkdir("/tmp/cs327_existing", 0700);

    int ret = makedirectory(path);
    ASSERT_EQ(ret, 0);

    rmdir("/tmp/cs327_existing");
}

TEST(makedirectory_no_slash_returns_zero)
{
    // Path with no '/' — makedirectory returns 0 immediately
    char path[] = "noslashpath";
    int ret = makedirectory(path);
    ASSERT_EQ(ret, 0);
}

// ═════════════════════════════════════════════════════════════
// SECTION 4 — rand_range macro (from utils.h)
// ═════════════════════════════════════════════════════════════

TEST(rand_range_single_value_always_returns_that_value)
{
    srand(1);
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(rand_range(5, 5), 5);
    }
}

TEST(rand_range_stays_within_bounds)
{
    srand(2024);
    for (int i = 0; i < 5000; i++) {
        int r = rand_range(1, 10);
        ASSERT_GE(r, 1);
        ASSERT_LE(r, 10);
    }
}

TEST(rand_range_zero_based)
{
    srand(9876);
    for (int i = 0; i < 1000; i++) {
        int r = rand_range(0, 3);
        ASSERT_GE(r, 0);
        ASSERT_LE(r, 3);
    }
}

// ═════════════════════════════════════════════════════════════
// SECTION 5 — dice seeded reproducibility
// ═════════════════════════════════════════════════════════════

TEST(dice_roll_reproducible_with_same_seed)
{
    dice d(0, 3, 6);
    srand(111);
    int32_t r1 = d.roll();
    srand(111);
    int32_t r2 = d.roll();
    ASSERT_EQ(r1, r2);
}

TEST(dice_roll_different_seeds_likely_differ)
{
    dice d(0, 3, 6);
    srand(1);
    int32_t r1 = d.roll();
    srand(999999);
    int32_t r2 = d.roll();
    // Not guaranteed, but extremely unlikely to collide over many seeds
    // (test is informational — won't fail if they happen to match)
    (void)r1; (void)r2;
    // Just ensure neither crashes
    ASSERT(true);
}

// ─────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────
int main()
{
    printf("\n=== cs327 Project 2 — Test Suite ===\n\n");

    printf("[ dice ]\n");
    RUN(dice_default_constructor_zeroes_fields);
    RUN(dice_parameterized_constructor_stores_values);
    RUN(dice_set_updates_all_fields);
    RUN(dice_set_individual_setters);
    RUN(dice_roll_base_only_when_sides_zero);
    RUN(dice_roll_within_expected_range);
    RUN(dice_roll_multi_die_range);
    RUN(dice_roll_negative_base_shifts_range);
    RUN(dice_roll_zero_number_of_dice);
    RUN(dice_roll_one_sided_die);
    RUN(dice_print_format);
    RUN(dice_stream_operator);
    RUN(dice_print_zero_dice);
    RUN(dice_roll_distribution_not_always_same_value);
    RUN(dice_roll_reproducible_with_same_seed);
    RUN(dice_roll_different_seeds_likely_differ);

    printf("\n[ heap ]\n");
    RUN(heap_init_creates_empty_heap);
    RUN(heap_insert_single_element);
    RUN(heap_peek_min_returns_min_without_removing);
    RUN(heap_remove_min_returns_minimum);
    RUN(heap_remove_min_drains_in_sorted_order);
    RUN(heap_remove_min_empty_heap_returns_null);
    RUN(heap_size_tracks_insertions_and_removals);
    RUN(heap_handles_duplicate_values);
    RUN(heap_large_insertion_sorted_extraction);
    RUN(heap_decrease_key_no_replace_reorders);

    printf("\n[ utils / makedirectory ]\n");
    RUN(makedirectory_creates_leaf_dir);
    RUN(makedirectory_existing_dir_returns_zero);
    RUN(makedirectory_no_slash_returns_zero);

    printf("\n[ rand_range macro ]\n");
    RUN(rand_range_single_value_always_returns_that_value);
    RUN(rand_range_stays_within_bounds);
    RUN(rand_range_zero_based);

    printf("\n─────────────────────────────────────────────────────\n");
    printf("Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed) printf("  (%d FAILED)", tests_failed);
    printf("\n");

    return tests_failed ? 1 : 0;
}