#include "library.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int book_index_by_id(const Library *lib, int id)
{
    size_t i;
    for (i = 0U; i < lib->book_count; i++) {
        if (lib->books[i].id == id) return (int)i;
    }
    return -1;
}

static int book_has_active_transaction(const Library *lib, int id)
{
    size_t i;
    for (i = 0U; i < lib->transaction_count; i++) {
        if (lib->transactions[i].book_id == id &&
            lib->transactions[i].returned == 0) return 1;
    }
    return 0;
}

static int get_required_field(const char *prompt, char *buffer, size_t size)
{
    read_line(prompt, buffer, size);
    if (!text_field_valid(buffer, size)) {
        (void)puts("Value cannot be empty, too long, or contain the | character.");
        return 0;
    }
    return 1;
}

void add_book(Library *lib)
{
    Book book;
    size_t i;

    book.id = read_int("Book ID (positive number): ", 1, 2147483647);
    for (i = 0U; i < lib->book_count; i++) {
        if (lib->books[i].id == book.id) {
            (void)puts("That Book ID already exists.");
            return;
        }
    }

    if (!get_required_field("Title: ", book.title, sizeof book.title) ||
        !get_required_field("Author: ", book.author, sizeof book.author) ||
        !get_required_field("Category: ", book.category, sizeof book.category)) {
        return;
    }

    book.total_copies = read_int("Total copies: ", 1, 1000000);
    book.available_copies = book.total_copies;

    if (!ensure_book_capacity(lib, lib->book_count + 1U)) {
        (void)puts("Not enough memory to add the book.");
        return;
    }

    lib->books[lib->book_count] = book;
    lib->book_count++;
    if (!save_books(lib)) {
        lib->book_count--;
        (void)puts("Book was not added because storage failed.");
        return;
    }

    log_event(lib->username, "book-added");
    (void)puts("Book added successfully.");
}

void list_books(const Library *lib)
{
    size_t i;

    (void)puts("\n================ ALL BOOKS ================");
    (void)printf("%-8s %-25s %-20s %-15s %-8s %-10s\n",
           "Book ID", "Title", "Author", "Category", "Total", "Available");
    (void)puts("----------------------------------------------------------------------------------------");

    if (lib->book_count == 0U) {
        (void)puts("No books found. Please add a book first.");
        return;
    }

    for (i = 0U; i < lib->book_count; i++) {
        const Book *b = &lib->books[i];
        (void)printf("%-8d %-25.25s %-20.20s %-15.15s %-8d %-10d\n",
               b->id, b->title, b->author, b->category,
               b->total_copies, b->available_copies);
    }
}

#define BOOK_SEARCH_MAX_THREADS 4U

typedef struct {
    const Library *lib;
    const char *query;
    int numeric_id;
    unsigned char *matches;
    size_t start;
    size_t end;
} BookSearchTask;

static int book_matches_query(const Book *book, const char *query, int numeric_id)
{
    return (numeric_id > 0 && book->id == numeric_id) ||
           contains_ignore_case(book->title, query) ||
           contains_ignore_case(book->author, query) ||
           contains_ignore_case(book->category, query);
}

static void *book_search_worker(void *argument)
{
    BookSearchTask *task = (BookSearchTask *)argument;
    size_t i;

    for (i = task->start; i < task->end; i++) {
        task->matches[i] = (unsigned char)book_matches_query(
            &task->lib->books[i], task->query, task->numeric_id);
    }
    return NULL;
}

int find_books_parallel(const Library *lib, const char *query, int numeric_id,
                        unsigned char *matches, size_t matches_capacity)
{
    pthread_t threads[BOOK_SEARCH_MAX_THREADS];
    BookSearchTask tasks[BOOK_SEARCH_MAX_THREADS];
    unsigned char thread_started[BOOK_SEARCH_MAX_THREADS] = {0U, 0U, 0U, 0U};
    size_t worker_count;
    size_t base_items;
    size_t extra_items;
    size_t start = 0U;
    size_t worker;

    if (lib == NULL || query == NULL ||
        (lib->book_count > 0U && (lib->books == NULL || matches == NULL)) ||
        matches_capacity < lib->book_count) {
        return 0;
    }
    if (lib->book_count == 0U) {
        return 1;
    }

    worker_count = lib->book_count < BOOK_SEARCH_MAX_THREADS
                       ? lib->book_count : BOOK_SEARCH_MAX_THREADS;
    if (worker_count == 1U) {
        BookSearchTask single_task;
        single_task.lib = lib;
        single_task.query = query;
        single_task.numeric_id = numeric_id;
        single_task.matches = matches;
        single_task.start = 0U;
        single_task.end = lib->book_count;
        (void)book_search_worker(&single_task);
        return 1;
    }

    base_items = lib->book_count / worker_count;
    extra_items = lib->book_count % worker_count;

    for (worker = 0U; worker < worker_count; worker++) {
        size_t items = base_items + ((worker < extra_items) ? 1U : 0U);
        int create_status;

        tasks[worker].lib = lib;
        tasks[worker].query = query;
        tasks[worker].numeric_id = numeric_id;
        tasks[worker].matches = matches;
        tasks[worker].start = start;
        tasks[worker].end = start + items;
        start += items;

        create_status = pthread_create(&threads[worker], NULL,
                                       book_search_worker, &tasks[worker]);
        if (create_status == 0) {
            thread_started[worker] = 1U;
        } else {
            /* Preserve correct search results if the OS cannot create a thread. */
            (void)book_search_worker(&tasks[worker]);
        }
    }

    for (worker = 0U; worker < worker_count; worker++) {
        if (thread_started[worker] != 0U) {
            (void)pthread_join(threads[worker], NULL);
        }
    }
    return 1;
}

void search_books(const Library *lib)
{
    char query[100];
    unsigned char *matches;
    size_t i;
    int found = 0;
    int numeric_id = 0;
    char *end = NULL;
    long maybe_id;

    if (lib == NULL || lib->book_count == 0U) {
        (void)puts("No books are stored yet.");
        return;
    }

    read_line("Search by Book ID, title, author, or category: ", query, sizeof query);
    maybe_id = strtol(query, &end, 10);
    if (query[0] != '\0' && end != query && *end == '\0' &&
        maybe_id > 0 && maybe_id <= 2147483647L) {
        numeric_id = (int)maybe_id;
    }

    matches = (unsigned char *)calloc(lib->book_count, sizeof *matches);
    if (matches == NULL ||
        !find_books_parallel(lib, query, numeric_id, matches, lib->book_count)) {
        free(matches);
        (void)puts("Unable to allocate or prepare the book search.");
        return;
    }

    (void)puts("\nSearch results:");
    for (i = 0U; i < lib->book_count; i++) {
        if (matches[i] != 0U) {
            const Book *b = &lib->books[i];
            (void)printf("ID: %d | Title: %s | Author: %s | Category: %s | Available: %d/%d\n",
                   b->id, b->title, b->author, b->category,
                   b->available_copies, b->total_copies);
            found = 1;
        }
    }
    if (!found) {
        (void)puts("No matching book found.");
    }
    free(matches);
}

void update_book(Library *lib)
{
    int id = read_int("Book ID to update: ", 1, 2147483647);
    int index = book_index_by_id(lib, id);
    Book old_book;
    Book updated;
    int issued;

    if (index < 0) {
        (void)puts("Book not found.");
        return;
    }

    old_book = lib->books[index];
    updated = old_book;
    (void)printf("Current title: %s\n", old_book.title);

    if (!get_required_field("New title: ", updated.title, sizeof updated.title) ||
        !get_required_field("New author: ", updated.author, sizeof updated.author) ||
        !get_required_field("New category: ", updated.category, sizeof updated.category)) {
        return;
    }

    issued = old_book.total_copies - old_book.available_copies;
    updated.total_copies = read_int("New total copies: ", 1, 1000000);
    if (updated.total_copies < issued) {
        (void)puts("Total copies cannot be less than the number currently issued.");
        return;
    }
    updated.available_copies = updated.total_copies - issued;

    lib->books[index] = updated;
    if (!save_books(lib)) {
        lib->books[index] = old_book;
        (void)puts("Update cancelled because storage failed.");
        return;
    }
    log_event(lib->username, "book-updated");
    (void)puts("Book updated successfully.");
}

void delete_book(Library *lib)
{
    int id = read_int("Book ID to delete: ", 1, 2147483647);
    int index = book_index_by_id(lib, id);
    Book old_book;
    size_t i;

    if (index < 0) {
        (void)puts("Book not found.");
        return;
    }
    if (book_has_active_transaction(lib, id)) {
        (void)puts("Cannot delete this book while it has an active issue transaction.");
        return;
    }
    if (!confirm_action("Delete this book? (y/n): ")) {
        (void)puts("Delete cancelled.");
        return;
    }

    old_book = lib->books[index];
    for (i = (size_t)index; i + 1U < lib->book_count; i++) {
        lib->books[i] = lib->books[i + 1U];
    }
    lib->book_count--;

    if (!save_books(lib)) {
        for (i = lib->book_count; i > (size_t)index; i--) {
            lib->books[i] = lib->books[i - 1U];
        }
        lib->books[index] = old_book;
        lib->book_count++;
        (void)puts("Delete cancelled because storage failed.");
        return;
    }
    log_event(lib->username, "book-deleted");
    (void)puts("Book deleted successfully.");
}
