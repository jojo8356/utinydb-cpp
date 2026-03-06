#ifndef UTEST_H
#define UTEST_H

#include <prism/prism.hpp>
#include <cstring>

/* ============================================================
 * utest — Mini test framework
 * ============================================================ */

static int utest_pass_count = 0;
static int utest_fail_count = 0;
static int utest_current_failed = 0;

#define ASSERT(expr, msg)                                               \
    do {                                                                \
        if (!(expr)) {                                                  \
            prism::println("  FAIL {}:{}: {}", __FILE__, __LINE__,      \
                           (msg));                                      \
            utest_current_failed = 1;                                   \
        }                                                               \
    } while (0)

#define ASSERT_EQ(a, b, msg)                                            \
    do {                                                                \
        if ((a) != (b)) {                                               \
            prism::println("  FAIL {}:{}: {} (got {}, expected {})",     \
                           __FILE__, __LINE__, (msg),                   \
                           (int)(a), (int)(b));                         \
            utest_current_failed = 1;                                   \
        }                                                               \
    } while (0)

#define ASSERT_NOT_NULL(ptr, msg)                                       \
    do {                                                                \
        if ((ptr) == nullptr) {                                         \
            prism::println("  FAIL {}:{}: {} (got NULL)",               \
                           __FILE__, __LINE__, (msg));                  \
            utest_current_failed = 1;                                   \
        }                                                               \
    } while (0)

#define TEST(name)                                                      \
    do {                                                                \
        utest_current_failed = 0;                                       \
        prism::println("  RUN  {}", #name);                             \
        name();                                                         \
        if (utest_current_failed) {                                     \
            utest_fail_count++;                                         \
        } else {                                                        \
            utest_pass_count++;                                         \
            prism::println("  OK   {}", #name);                         \
        }                                                               \
    } while (0)

#define TEST_REPORT()                                                   \
    do {                                                                \
        prism::println("\n========================================");    \
        prism::println("  {} passed, {} failed",                        \
               utest_pass_count, utest_fail_count);                     \
        prism::println("========================================");      \
    } while (0)

#define TEST_EXIT_CODE() (utest_fail_count > 0 ? 1 : 0)

#endif /* UTEST_H */
