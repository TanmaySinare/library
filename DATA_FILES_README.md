# Included library data files

These are the canonical data filenames used by `include/library.h` and `Src/storage.c`:

- `books.txt`
- `member.txt`
- `transaction.txt`
- `library.log`

Do not create parallel `members.txt` or `transactions.txt` files. The application reads and writes the singular filenames `member.txt` and `transaction.txt`.

These files contain the records available when this package was assembled. Back up your own Codespaces data before replacing any project files. Run the application from the project root so it reads and writes these files in the expected location. The application may modify them when books or members are added, updated, or deleted, and when transactions are issued or returned.
