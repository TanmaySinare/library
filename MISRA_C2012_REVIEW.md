# MISRA C:2012 screening and compliance review

## Current status

This package preserves the existing menu-driven C Library Management System, its text-file storage format, member-registration timestamp, and read-only parallel book search. It has received a MISRA-oriented improvement pass. **It is not claimed to be fully MISRA C:2012 compliant or certified.**

MISRA compliance is a project-level process: identify the applicable MISRA edition and amendments, configure a qualified checker, review all required and advisory rule findings, record approved deviations, and obtain the required independent review. A successful compiler build or a zero-finding general Cppcheck scan is not equivalent to MISRA compliance.

## Findings in the latest supplied MISRA report

The user-supplied report for this source family contained 210 MISRA addon findings and four `misra-config` errors for `ENOENT`. The rule counts in that report were:

| Rule | Findings | General subject |
|---|---:|---|
| 15.5 | 126 | Single point of exit recommendation |
| 12.1 | 52 | Explicit operator precedence |
| 21.3 | 9 | Dynamic memory allocation/deallocation |
| 21.6 | 7 | Standard input/output library |
| 22.10 | 4 | Correct use of `errno` |
| 21.10 | 2 | Time/date library |
| 15.7 | 2 | Final `else` clause in an `if`/`else if` chain |
| 10.8 | 2 | Cast of a composite expression |
| 11.5 | 1 | Conversion from pointer to void |
| 14.4 | 1 | Essentially Boolean controlling expression |
| 22.8 | 1 | `strtol` error handling |
| 15.6 | 1 | Compound statement / braces |
| 10.6 | 1 | Assignment to a wider essential type |
| 8.7 | 1 | External linkage visibility |

These are counts from the supplied report, not results of a new scan performed in this package-preparation environment. Re-run `make misra` after installation of Cppcheck to obtain current results.

## Improvements retained in the source

- The three general Cppcheck `constVariablePointer` suggestions are addressed:
  - `Src/members.c`: `const struct tm *time_info`
  - `Src/util.c`: both `const struct tm *local` declarations
- The logger uses the `LOG_FILE` macro.
- The transaction module's private helper has a unique internal name.
- Several post-increment side effects in array operations are separated into distinct statements.
- Selected integer-valued conditions are made explicit.
- `strtol` parsing resets and checks `errno` and handles `ERANGE`.
- `sizeof *pointer` is used for allocated object sizes to keep the type tied to the pointed-to object.
- The sanitizer build compiles C modules as C with GCC before linking the C++ test runner.
- GCov is invoked once for each `.gcno` file.

## MISRA addon configuration improvement

The Makefile derives the host platform's `ENOENT` macro value using the selected C compiler's `<errno.h>` preprocessor output and passes that value to Cppcheck **for analysis only**. The application source continues to include `<errno.h>` and use the platform's actual `ENOENT` definition. No hard-coded replacement is made in `storage.c`.

This addresses the specific parser/configuration problem seen in the supplied report on Linux-like hosts. It does not suppress MISRA rule findings and does not make the source automatically compliant.

## Findings requiring design decisions or deviations

- **Rule 21.3:** the current model uses `calloc`, `realloc`, and `free` for growable arrays. Strict treatment may require bounded static arrays or a formally approved deviation with memory-budget and allocation-failure controls.
- **Rule 21.6:** the console UI and text-file persistence use standard I/O. A strict project may require an approved I/O abstraction or a formal deviation.
- **Rule 21.10:** local date/time operations use `<time.h>`. A stricter target may require a platform time-service abstraction or a deviation.
- **Rule 15.5:** many functions use early returns for validation/error paths. Refactoring all of these into a single exit path can alter cleanup and state handling; do it function-by-function with regression tests.
- **Rule 12.1:** parentheses must be reviewed against intended logic. Do not add parentheses mechanically without understanding precedence and side effects.
- **POSIX threads:** the worker entry point has the `void *` argument required by `pthread_create`; review and document the applicable deviation for pointer conversion and POSIX APIs if your project profile requires it.
- **Rule 22.10:** verify `errno` is reset before each library call whose failure path is inspected via `errno`.

## Reproducible checks

```sh
make clean
make
make thread-smoke
make test
make cppcheck
make misra-config-check
make misra
```

Install GoogleTest development files for `make test`, and install Cppcheck with the MISRA addon for the analysis targets. The reports are generated under `Coverage/`.

## Compliance statement

The accurate description of this package is: **MISRA-screened and improved; full MISRA C:2012 compliance remains unverified, with known findings and design deviations listed above.** Do not submit or represent it as a certified MISRA-compliant product without completing the project's formal compliance process.
