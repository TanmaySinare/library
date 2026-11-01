# Library Management System (C)

## Folder structure

- `Src/`: C implementation files.
- `include/`: public header (`library.h`).
- `Test/`: GoogleTest unit tests (`library_all_function_unit_tests.cpp`).
- `Built/`: generated application, test executable and object files.
- `Coverage/`: generated Cppcheck, MISRA-screening, Valgrind and gcov reports.
- `books.txt`, `member.txt`, `transaction.txt`: canonical application data files. `library.log` stores application events. The application uses the singular names `member.txt` and `transaction.txt`; duplicate plural aliases are intentionally not included.
- `library.log`: application event log.
- `Makefile`: build and analysis commands.

There is intentionally no `Source code/` folder. The Makefile is in the project root.

## Build and run

```sh
make
make run
```

Demo credentials (educational use only): `admin` / `Admin@123` or `librarian` / `Librarian@123`.

## Member registration timestamp

When a member is registered, the local system date/time is stored as `YYYY-MM-DD HH:MM:SS`. The `Added At` column is displayed in **View All Members**. The timestamp is persisted as the fourth field in `member.txt`. Legacy three-field member rows are still loaded, and their timestamp is shown as `Unknown (legacy record)` because the original time cannot be reconstructed.

## Multithreaded book search

Book searching uses up to four POSIX worker threads to scan separate ranges of the
book array. Each worker writes only to its own result indexes; the calling thread
joins all workers and prints results in their original order. The search is read-only.
Do not modify or free the `Library` while `find_books_parallel()` is running. Book,
member, transaction, and text-file updates remain serialized by the command-line
workflow; this project does not claim that concurrent writes are supported.

The test suite includes a multi-range parallel-search test. `make helgrind` runs
the GoogleTest suite under Valgrind Helgrind to look for thread synchronization
problems when those tools are installed.

## Tests and analysis

```sh
make test             # GoogleTest; requires g++ and GoogleTest development package
make valgrind         # full GoogleTest suite under Valgrind Memcheck
make helgrind         # full GoogleTest suite under Valgrind Helgrind
make thread-smoke     # standalone test of the threaded search (no GoogleTest needed)
make valgrind-smoke   # threaded-search smoke test under Memcheck
make helgrind-smoke   # threaded-search smoke test under Helgrind
make cppcheck         # requires Cppcheck
make misra            # requires Cppcheck with its MISRA addon installed
make coverage         # runs tests and writes gcov coverage data
make clean
```

Reports are written to `Coverage/`: `valgrind_report.txt`, `helgrind_report.txt`,
`valgrind_thread_search_report.txt`, and `helgrind_thread_search_report.txt` are created
by their respective targets. These reports are generated only when the tools are
installed and the targets have actually been run.

## Important quality/compliance note

These commands provide build, unit-test, memory-analysis, static-analysis and coverage workflows. They do **not** guarantee full MISRA C:2012 compliance or 100% statement/branch/function coverage. MISRA compliance requires reviewing every applicable rule, compiler/tool versions, project configuration and documented deviations. Coverage is measured only for code executed by the available tests; additional tests are needed for uncovered lines and branches. The test and analysis targets must be run in an environment where the required tools are installed.


## GoogleTest coverage

Run `make coverage` to cleanly rebuild the coverage-instrumented test executable, run the GoogleTest suite, and write the gcov summary to `Coverage/gcov_report.txt`. Annotated `*.gcov` source files are placed in `Coverage/`. The test translation unit includes `Src/main.c` under a renamed entry-point symbol so the application menu can also be exercised by integration-style GoogleTests. Coverage percentages are measured results; a passing test suite does not guarantee a 95% or 100% score.

## MISRA review status

This version includes a MISRA-oriented improvement pass and fixes the three `constVariablePointer` suggestions in `Src/members.c` and `Src/util.c`. It is **not certified or fully MISRA C:2012 compliant**. Current design choices such as dynamic memory, standard console/file I/O, and the C time library require architectural changes or formally approved deviations for strict MISRA use. See [MISRA_C2012_REVIEW.md](MISRA_C2012_REVIEW.md) for the review/deviation register and [VERIFICATION_STATUS.md](VERIFICATION_STATUS.md) for the checks actually run.

## Preserving existing library data

This ZIP includes only the canonical data files `books.txt`, `member.txt`, `transaction.txt`, and `library.log`. The source uses `member.txt` and `transaction.txt` (singular), so separate plural copies are unnecessary. Back up your own data before running the application because add, update, issue and return operations can modify these files.


## MISRA configuration note

`make misra` derives the Linux host's `ENOENT` macro value using GCC and passes it to Cppcheck's MISRA addon as an analysis-only preprocessor definition. This addresses the addon's unknown-`ENOENT` parser diagnostic in the reported Linux setup without changing the program's use of `<errno.h>`. It does not remove MISRA rule findings or certify compliance. Read [MISRA_C2012_REVIEW.md](MISRA_C2012_REVIEW.md) before making any compliance claim.
