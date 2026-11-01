#include "library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SMOKE_BOOK_COUNT 128U

int main(void)
{
    Library lib = {0};
    unsigned char *matches;
    size_t i;
    size_t found = 0U;
    int valid = 1;

    if (!ensure_book_capacity(&lib, SMOKE_BOOK_COUNT)) {
        (void)fprintf(stderr, "FAIL: unable to allocate book records\n");
        return EXIT_FAILURE;
    }

    for (i = 0U; i < SMOKE_BOOK_COUNT; i++) {
        Book *book = &lib.books[i];
        const char *title = (i % 19U == 0U)
                                ? "Parallel Search Needle"
                                : "Ordinary Reference Book";

        (void)memset(book, 0, sizeof *book);
        book->id = 1000 + (int)i;
        (void)snprintf(book->title, sizeof book->title, "%s", title);
        (void)snprintf(book->author, sizeof book->author, "Smoke Test Author");
        (void)snprintf(book->category, sizeof book->category, "Test Category");
    }
    lib.book_count = SMOKE_BOOK_COUNT;

    matches = (unsigned char *)calloc(lib.book_count, sizeof *matches);
    if (matches == NULL) {
        (void)fprintf(stderr, "FAIL: unable to allocate search results\n");
        free_library(&lib);
        return EXIT_FAILURE;
    }

    if (!find_books_parallel(&lib, "needle", 0, matches, lib.book_count)) {
        (void)fprintf(stderr, "FAIL: parallel search rejected valid input\n");
        free(matches);
        free_library(&lib);
        return EXIT_FAILURE;
    }

    for (i = 0U; i < lib.book_count; i++) {
        if (matches[i] != 0U) {
            ++found;
            if (i % 19U != 0U) {
                valid = 0;
            }
        }
    }

    if (found != 7U || valid == 0) {
        (void)fprintf(stderr, "FAIL: found %zu matches; expected 7\n", found);
        free(matches);
        free_library(&lib);
        return EXIT_FAILURE;
    }

    (void)printf("PASS: parallel search scanned %u records and found %zu expected matches.\n",
                 (unsigned int)SMOKE_BOOK_COUNT, found);
    free(matches);
    free_library(&lib);
    return EXIT_SUCCESS;
}
