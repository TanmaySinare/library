#include "library.h"

#include <stdio.h>
#include <string.h>

static void print_menu(Role role)
{
    (void)puts("\n========== LIBRARY MANAGEMENT SYSTEM ==========");
    (void)puts("1.  Add book");
    (void)puts("2.  View all books");
    (void)puts("3.  Search books");
    (void)puts("4.  Update book");
    if (role == ROLE_ADMIN) {
        (void)puts("5.  Delete book [Administrator only]");
    } else {
        (void)puts("5.  Delete book [Administrator only - restricted]");
    }
    (void)puts("6.  Register member");
    (void)puts("7.  View all members");
    (void)puts("8.  Search members");
    (void)puts("9.  Update member");
    if (role == ROLE_ADMIN) {
        (void)puts("10. Delete member [Administrator only]");
    } else {
        (void)puts("10. Delete member [Administrator only - restricted]");
    }
    (void)puts("11. Issue book");
    (void)puts("12. Return book");
    (void)puts("13. View transactions");
    (void)puts("14. View fine details");
    (void)puts("0.  Exit");
}

int main(void)
{
    Library library;
    int choice;
    Role role;

    (void)memset(&library, 0, sizeof library);

    (void)puts("Library Management System");
    (void)puts("Data is stored in text files in the current project directory.");

    if (!load_data(&library)) {
        (void)puts("Could not load stored data. Check the data files before continuing.");
        free_library(&library);
        return 1;
    }

    role = authenticate_user(library.username);
    if (role == ROLE_NONE) {
        free_library(&library);
        return 1;
    }
    library.role = role;
    (void)printf("Welcome, %s (%s).\n", library.username, role_name(role));

    do {
        print_menu(library.role);
        choice = read_int("Enter your choice: ", 0, 14);

        switch (choice) {
            case 1: add_book(&library); break;
            case 2: list_books(&library); break;
            case 3: search_books(&library); break;
            case 4: update_book(&library); break;
            case 5:
                if (library.role != ROLE_ADMIN) {
                    (void)puts("Access denied. Only administrators can delete books.");
                    log_event(library.username, "unauthorized-book-delete-attempt");
                } else {
                    delete_book(&library);
                }
                break;
            case 6: register_member(&library); break;
            case 7: list_members(&library); break;
            case 8: search_members(&library); break;
            case 9: update_member(&library); break;
            case 10:
                if (library.role != ROLE_ADMIN) {
                    (void)puts("Access denied. Only administrators can delete members.");
                    log_event(library.username, "unauthorized-member-delete-attempt");
                } else {
                    delete_member(&library);
                }
                break;
            case 11: issue_book(&library); break;
            case 12: return_book(&library); break;
            case 13: list_transactions(&library); break;
            case 14: show_fine_details(&library); break;
            case 0:
                (void)puts("Exiting Library Management System.");
                log_event(library.username, "logout");
                break;
            default:
                (void)puts("Invalid menu option.");
                break;
        }
    } while (choice != 0);

    free_library(&library);
    return 0;
}
