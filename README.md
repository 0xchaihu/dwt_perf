# DWT Perf — ARM Cortex-M DWT Cycle Counter Performance Measurement

Header-only module for measuring code execution time using the DWT (Data Watchpoint and Trace) cycle counter on ARM Cortex-M3+ processors.

## Quick Start

```c
#include "fsl_debug_console.h"
#include "board.h"

#define DWT_PERF_PRINTF DbgConsole_Printf
#include "dwt_perf.h"

int main(void)
{
    BOARD_InitHardware();
    dwt_perf_init();

    DWT_PERF_START(mypiece);
    /* ... code to measure ... */
    DWT_PERF_END(mypiece);
}
```

`dwt_perf.h` first uses `__CORTEX_M` when a device/CMSIS header was already included, then falls back to unambiguous compiler architecture macros (`__ARM_ARCH_*`). For ambiguous families such as Cortex-M4/M7 and Cortex-M55/M85, include your device header first or define `DWT_PERF_CORE_HEADER` manually.

`DWT_PERF_END` prints:

```
[DWT] main.c:25 1234 cycles, 15 us, 0 ms
```

## API

| Name | Type | Description |
|------|------|-------------|
| `dwt_perf_init()` | function | Enable DWT cycle counter. Call once after hardware init. |
| `dwt_perf_is_available()` | function | Return non-zero when the core implements `DWT->CYCCNT`. |
| `dwt_perf_get_cycles()` | function | Read current 32-bit cycle count. |
| `DWT_PERF_START(label)` | macro | Capture start cycle count into a local variable. |
| `DWT_PERF_END(label)` | macro | Compute delta, print result, return elapsed cycles (`uint32_t`). |

## Usage Patterns

### Sequential Measurements

Each label creates a unique variable; different labels can coexist in the same scope.

```c
DWT_PERF_START(hash);    hash_data();     DWT_PERF_END(hash);
DWT_PERF_START(verify);  verify_result(); DWT_PERF_END(verify);
```

### Nested Measurements

You can use `dwt_perf` not only inside a single function, but also inside sub-functions called from the measured code.

This makes it possible to measure:

- the total time of a larger operation in the caller
- the time of a smaller step inside a callee

In other words, an outer measurement can cover the whole workflow, while inner measurements in sub-functions can be used to profile specific parts of that workflow.

Each `DWT_PERF_START()` / `DWT_PERF_END()` pair must still use a matching label in the same local scope.
If nested measurements are written in the same function, wrap the inner pair in braces.
If the inner measurement is placed in a sub-function, the function scope already separates the local variables naturally.

```c
static void process_item(void)
{
    DWT_PERF_START(process_item_inner);

    /* ... sub-function work ... */

    DWT_PERF_END(process_item_inner);
}

void run_workflow(void)
{
    DWT_PERF_START(workflow_total);

    prepare_data();
    process_item();
    finalize();

    DWT_PERF_END(workflow_total);
}
```

This allows you to see both the total execution time of `run_workflow()` and the internal execution time of `process_item()`.

### Using the Return Value

`DWT_PERF_END` returns the elapsed cycle count, which can be used for conditional logic.

```c
static uint32_t measure_algorithm(void)
{
    DWT_PERF_START(algo);
    run_algorithm();
    return DWT_PERF_END(algo);
}

uint32_t cycles = measure_algorithm();
if (cycles > BUDGET_CYCLES) {
    /* handle budget overrun */
}
```

## Configuration Macros

Define these **before** `#include "dwt_perf.h"`.

### `DWT_PERF_PRINTF` — Output Backend

Overrides the print function used by `DWT_PERF_END`.

```c
#define DWT_PERF_PRINTF SEGGER_RTT_Printf
#include "dwt_perf.h"
```

Default: `printf` from `<stdio.h>`.

### `DWT_PERF_CORE_HEADER` — CMSIS Core Header Override

By default, `dwt_perf.h` auto-selects the CMSIS core header in this order:

| Detection Source | Core | Header |
|---|---|---|
| `__CORTEX_M == 85` | Cortex-M85 | `core_cm85.h` |
| `__CORTEX_M == 55` | Cortex-M55 | `core_cm55.h` |
| `__CORTEX_M == 33` | Cortex-M33 / M35P | `core_cm33.h` |
| `__CORTEX_M == 7` | Cortex-M7 | `core_cm7.h` |
| `__CORTEX_M == 4` | Cortex-M4 | `core_cm4.h` |
| `__CORTEX_M == 3` | Cortex-M3 | `core_cm3.h` |
| `__ARM_ARCH_8M_MAIN__` | Cortex-M33 / M35P | `core_cm33.h` |
| `__ARM_ARCH_7M__` | Cortex-M3 | `core_cm3.h` |

`__ARM_ARCH_7EM__` and `__ARM_ARCH_8_1M_MAIN__` are intentionally treated as ambiguous, because those architecture macros do not uniquely identify M4 vs M7 or M55 vs M85. On those families, include your device header first so `__CORTEX_M` is already defined, or set `DWT_PERF_CORE_HEADER` yourself.

Cortex-M0 / M0+ / M23 are **not supported** — their DWT does not implement CYCCNT. Including this header on those cores will produce a compile-time error.

To override, define `DWT_PERF_CORE_HEADER` before including:

```c
#define DWT_PERF_CORE_HEADER "fsl_device_registers.h"
#include "dwt_perf.h"
```

### `DWT_PERF_SILENT` — Suppress All Output

Define to disable printing. The cycle counter still runs and `DWT_PERF_END` still returns cycles.

```c
#define DWT_PERF_SILENT
#include "dwt_perf.h"
```

Useful for production builds or when you only need the numeric return value.

### `DWT_PERF_ASSERT` — Availability Failure Hook

Overrides the assertion used when the core exposes DWT but does not implement `CYCCNT`.

```c
#define DWT_PERF_ASSERT(expr) configASSERT(expr)
#include "dwt_perf.h"
```

`dwt_perf_init()` asserts first, then returns without enabling the counter if `CYCCNT` is unavailable. `dwt_perf_get_cycles()` returns `0` in that case.

## Limitations

- **Not supported on Cortex-M0 / M0+ / M23** — these cores' DWT unit does not implement the cycle counter (CYCCNT). A compile-time error will be emitted if included on an unsupported core.
- `__ARM_ARCH_7EM__` and `__ARM_ARCH_8_1M_MAIN__` are architecture-level macros, not exact core IDs. Use `DWT_PERF_CORE_HEADER` or include your device header first on those families.
- Some DWT implementations expose the block but not `CYCCNT`. `dwt_perf_init()` asserts and then returns without enabling the counter; `dwt_perf_get_cycles()` returns `0` if the counter is unavailable.
- The DWT cycle counter is 32-bit. At 200 MHz it wraps around in ~21 seconds; at 480 MHz in ~9 seconds. For long-running measurements, poll `dwt_perf_get_cycles()` periodically.
- Requires ARM Cortex-M3 or later (DWT unit with CYCCNT).
