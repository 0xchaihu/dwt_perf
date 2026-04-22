#ifndef __DWT_PERF_H__
#define __DWT_PERF_H__

#include <assert.h>
#include <stdint.h>

/*
 * Portable DWT cycle counter performance measurement — header-only.
 *
 * CMSIS core header is auto-selected from __CORTEX_M when available,
 * otherwise from unambiguous compiler architecture macros.
 * To override, define DWT_PERF_CORE_HEADER before including this header.
 *
 * Print hook (override before including this header):
 *   #define DWT_PERF_PRINTF  my_printf    // e.g. DbgConsole_Printf, SEGGER_RTT_printf, ...
 *   Default: standard printf from <stdio.h>
 *
 * DWT_PERF_SILENT: define to suppress all [DWT] prints.
 *   #define DWT_PERF_SILENT
 * Benchmarks still run and DWT_PERF_END still returns cycle count.
 */

//#define DWT_PERF_PRINTF DbgConsole_Printf
//#define DWT_PERF_SILENT

/* ---- CMSIS core header (auto-detected from __CORTEX_M / __ARM_ARCH_* macros) ---- */
#if defined(DWT_PERF_CORE_HEADER)
  /* User manual override */
  #include DWT_PERF_CORE_HEADER
#elif defined(__CORTEX_M) && (__CORTEX_M == 85U)
  #include "core_cm85.h"
#elif defined(__CORTEX_M) && (__CORTEX_M == 55U)
  #include "core_cm55.h"
#elif defined(__CORTEX_M) && (__CORTEX_M == 33U)
  #include "core_cm33.h"
#elif defined(__CORTEX_M) && (__CORTEX_M == 7U)
  #include "core_cm7.h"
#elif defined(__CORTEX_M) && (__CORTEX_M == 4U)
  #include "core_cm4.h"
#elif defined(__CORTEX_M) && (__CORTEX_M == 3U)
  #include "core_cm3.h"
#elif defined(__ARM_ARCH_8_1M_MAIN__)
  #error "DWT Perf: __ARM_ARCH_8_1M_MAIN__ is ambiguous between Cortex-M55 and Cortex-M85. Include your device header first or define DWT_PERF_CORE_HEADER manually."
#elif defined(__ARM_ARCH_8M_MAIN__)
  #include "core_cm33.h"
#elif defined(__ARM_ARCH_7EM__)
  #error "DWT Perf: __ARM_ARCH_7EM__ is ambiguous between Cortex-M4 and Cortex-M7. Include your device header first or define DWT_PERF_CORE_HEADER manually."
#elif defined(__ARM_ARCH_7M__)
  #include "core_cm3.h"
#elif defined(__ARM_ARCH_8M_BASE__) || defined(__ARM_ARCH_6M__)
  #error "DWT Perf: DWT CYCCNT is not available on Cortex-M0/M0+/M23."
#else
  #error "DWT Perf: unable to detect ARM Cortex-M core. Define DWT_PERF_CORE_HEADER manually."
#endif

/* ---- Print hook ---- */
#ifndef DWT_PERF_PRINTF
#include <stdio.h>
#define DWT_PERF_PRINTF printf
#endif

#ifndef DWT_PERF_ASSERT
#define DWT_PERF_ASSERT(expr) assert(expr)
#endif

extern uint32_t SystemCoreClock;

/* ---- DWT hardware access (CMSIS Core, Cortex-M3+) ---- */

static inline int dwt_perf_is_available(void)
{
#ifdef DWT_CTRL_NOCYCCNT_Msk
    return ((DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) == 0U);
#else
    return 1;
#endif
}

static inline void dwt_perf_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT_PERF_ASSERT(dwt_perf_is_available());
    if (!dwt_perf_is_available())
    {
        return;
    }

    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline uint32_t dwt_perf_get_cycles(void)
{
    return dwt_perf_is_available() ? DWT->CYCCNT : 0U;
}

/* ---- Macros ---- */

/*
 * Usage:
 *   DWT_PERF_START(init);
 *   ... code to measure ...
 *   DWT_PERF_END(init);
 *
 * Sequential benchmarks use unique labels:
 *   DWT_PERF_START(hash);    foo();    DWT_PERF_END(hash);
 *   DWT_PERF_START(verify);  bar();    DWT_PERF_END(verify);
 *
 * Nested benchmarks — wrap inner pair in a block:
 *   DWT_PERF_START(outer);
 *   outer_code();
 *   { DWT_PERF_START(inner); inner_code(); DWT_PERF_END(inner); }
 *   DWT_PERF_END(outer);
 *
 * DWT_PERF_END returns the elapsed cycle count:
 *   uint32_t cyc = DWT_PERF_END(label);
 */

#define _DWT_PERF_CONCAT_(a, b) a##b
#define _DWT_PERF_CONCAT(a, b)  _DWT_PERF_CONCAT_(a, b)

#define DWT_PERF_START(label) \
    uint32_t _DWT_PERF_CONCAT(_dwt_perf_t0_, label) = dwt_perf_get_cycles()

#ifdef DWT_PERF_SILENT
#define DWT_PERF_END(label)                                                      \
    ({                                                                             \
        dwt_perf_get_cycles() - _DWT_PERF_CONCAT(_dwt_perf_t0_, label);            \
    })
#else
#define DWT_PERF_END(label)                                                      \
    ({                                                                             \
        uint32_t _dwt_dt_ = dwt_perf_get_cycles() - _DWT_PERF_CONCAT(_dwt_perf_t0_, label); \
        uint32_t _dwt_us_  = (uint32_t)(((uint64_t)_dwt_dt_ * 1000000ULL) / SystemCoreClock); \
        uint32_t _dwt_ms_  = (uint32_t)(((uint64_t)_dwt_dt_ * 1000ULL) / SystemCoreClock);   \
        DWT_PERF_PRINTF("[DWT] %s:%u %u cycles, %u us, %u ms\r\n",               \
               __FILE__, __LINE__, _dwt_dt_, _dwt_us_, _dwt_ms_);                 \
        _dwt_dt_;                                                                  \
    })
#endif

#endif /* __DWT_PERF_H__ */
