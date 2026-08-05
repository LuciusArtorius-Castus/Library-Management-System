#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_BOOKS         1000
#define MAX_MEMBERS       1000
#define MAX_TRANSACTIONS  5000

#define BOOKS_FILE        "books.dat"
#define MEMBERS_FILE      "members.dat"
#define TRANSACTIONS_FILE "transactions.dat"

#define LENDING_DAYS      14
#define FINE_PER_DAY      2.0

#define STAFF_USERNAME    "admin"
#define STAFF_PASSWORD    "admin123"

struct Date {
    int day;
    int month;
    int year;
};

struct Book {
    int  bookID;
    char title[100];
    char author[80];
    char isbn[20];
    char publisher[80];
    char edition[30];
    char genre[40];
    char subject[40];
    char shelf[20];
    int  totalCopies;
    int  availableCopies;
};

struct Member {
    int  memberID;
    char name[80];
    char address[120];
    char contact[20];
    char email[80];
    char status[15];           
    struct Date membershipDate;
    int  maxBooks;
};

struct Transaction {
    int  transactionID;
    int  memberID;
    int  bookID;
    struct Date issueDate;
    struct Date dueDate;
    struct Date returnDate;    
    int  returned;             
    double fine;               
};

static struct Book        books[MAX_BOOKS];
static int                bookCount = 0;

static struct Member      members[MAX_MEMBERS];
static int                memberCount = 0;

static struct Transaction transactions[MAX_TRANSACTIONS];
static int                transactionCount = 0;
static int                nextTransactionID = 1;

void  pressEnterToContinue(void);
void  printSeparator(char ch, int len);
void  clearInputBuffer(void);
void  toLowerStr(const char *src, char *dst);
int   containsIgnoreCase(const char *haystack, const char *needle);

void   readLine(const char *prompt, char *buffer, int size);
int    readInt(const char *prompt);
int    readIntInRange(const char *prompt, int min, int max);
double readPositiveDouble(const char *prompt);
struct Date readDate(const char *prompt);

int    isLeapYear(int year);
int    daysInMonth(int month, int year);
int    validateDate(int d, int m, int y);
long   dateToSerial(struct Date d);
int    daysBetween(struct Date a, struct Date b);
struct Date addDays(struct Date d, int n);
struct Date calculateDueDate(struct Date issueDate);
struct Date getCurrentDate(void);
void   printDate(struct Date d);
void   dateToString(struct Date d, char *out);

void sanitizeField(char *s);
void saveBooks(void);
void loadBooks(void);
void saveMembers(void);
void loadMembers(void);
void saveTransactions(void);
void loadTransactions(void);
void saveData(void);
void loadData(void);

int findBookByID(int bookID);
int findBookByISBN(const char *isbn);
int findMemberByID(int memberID);
int findTransactionByID(int transactionID);
int findActiveTransaction(int memberID, int bookID);
int countActiveIssuesForMember(int memberID);
double getTransactionFine(int transactionIndex);

void bookManagementMenu(void);
void addBook(void);
void viewBooks(void);
void deleteBook(void);

void memberManagementMenu(void);
void addMember(void);
void updateMember(void);
void addBookToCheckout(void);
void removeBookFromCheckout(void);
void checkoutView(void);

void issuanceReturnMenu(void);
int  issueBookCore(int memberID, int bookID, int *outTransactionID, char *errMsg);
int  returnBookCore(int transactionIndex, double *outFine, char *errMsg);
void issueBook(void);
void returnBook(void);
void viewIssuanceRecords(void);
void calculateFineMenu(void);

void searchRecordsMenu(void);
void searchBooks(void);
void searchMembers(void);

void printBookRow(struct Book *b);
void printMemberIssuedBooks(int memberID);

void loginMenu(void);
void staffMenu(void);
void memberMenu(int memberID);

int main(void) {
    loadData();
    printf("==================================================\n");
    printf("      WELCOME TO THE LIBRARY MANAGEMENT SYSTEM\n");
    printf("==================================================\n");
    loginMenu();

    printf("\nSaving all data before exit...\n");
    saveData();
    printf("Goodbye!\n");
    return 0;
}

void pressEnterToContinue(void) {
    printf("\nPress ENTER to continue...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void printSeparator(char ch, int len) {
    for (int i = 0; i < len; i++) putchar(ch);
    putchar('\n');
}

void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void toLowerStr(const char *src, char *dst) {
    int i = 0;
    for (; src[i] != '\0'; i++) dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = '\0';
}

int containsIgnoreCase(const char *haystack, const char *needle) {
    if (needle[0] == '\0') return 0; 
    char h[256], n[256];
    toLowerStr(haystack, h);
    toLowerStr(needle, n);
    return strstr(h, n) != NULL;
}

void readLine(const char *prompt, char *buffer, int size) {
    printf("%s", prompt);
    if (fgets(buffer, size, stdin) == NULL) {
        printf("\nInput stream closed. Saving data and exiting...\n");
        saveData();
        exit(0);
    }
    buffer[strcspn(buffer, "\n")] = '\0';
}

int readInt(const char *prompt) {
    char buf[64];
    int value;
    while (1) {
        printf("%s", prompt);
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            printf("\nInput stream closed. Saving data and exiting...\n");
            saveData();
            exit(0);
        }
        if (sscanf(buf, "%d", &value) == 1) return value;
        printf("Invalid input. Please enter a whole number.\n");
    }
}

int readIntInRange(const char *prompt, int min, int max) {
    int value;
    while (1) {
        value = readInt(prompt);
        if (value >= min && value <= max) return value;
        printf("Please enter a value between %d and %d.\n", min, max);
    }
}

double readPositiveDouble(const char *prompt) {
    char buf[64];
    double value;
    while (1) {
        printf("%s", prompt);
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            printf("\nInput stream closed. Saving data and exiting...\n");
            saveData();
            exit(0);
        }
        if (sscanf(buf, "%lf", &value) == 1 && value >= 0) return value;
        printf("Invalid input. Please enter a non-negative number.\n");
    }
}

struct Date readDate(const char *prompt) {
    struct Date d;
    int day, month, year;
    printf("%s", prompt);
    while (1) {
        day = readIntInRange("  Day   (1-31): ", 1, 31);
        month = readIntInRange("  Month (1-12): ", 1, 12);
        year = readIntInRange("  Year  (e.g. 2026): ", 1900, 2100);
        if (validateDate(day, month, year)) {
            d.day = day; d.month = month; d.year = year;
            return d;
        }
        printf("That date does not exist. Please try again.\n");
    }
}

int isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int month, int year) {
    static const int dim[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && isLeapYear(year)) return 29;
    if (month < 1 || month > 12) return 0;
    return dim[month - 1];
}

int validateDate(int d, int m, int y) {
    if (y < 1900 || y > 2100) return 0;
    if (m < 1 || m > 12) return 0;
    if (d < 1 || d > daysInMonth(m, y)) return 0;
    return 1;
}

long dateToSerial(struct Date d) {
    long days = 0;
    for (int y = 1; y < d.year; y++) days += isLeapYear(y) ? 366 : 365;
    for (int m = 1; m < d.month; m++) days += daysInMonth(m, d.year);
    days += d.day;
    return days;
}

int daysBetween(struct Date a, struct Date b) {
    return (int)(dateToSerial(b) - dateToSerial(a));
}

struct Date addDays(struct Date d, int n) {
    for (int i = 0; i < n; i++) {
        d.day++;
        if (d.day > daysInMonth(d.month, d.year)) {
            d.day = 1;
            d.month++;
            if (d.month > 12) { d.month = 1; d.year++; }
        }
    }
    return d;
}

struct Date calculateDueDate(struct Date issueDate) {
    return addDays(issueDate, LENDING_DAYS);
}

struct Date getCurrentDate(void) {
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    struct Date d;
    d.day = lt->tm_mday;
    d.month = lt->tm_mon + 1;
    d.year = lt->tm_year + 1900;
    return d;
}

void printDate(struct Date d) {
    printf("%02d-%02d-%04d", d.day, d.month, d.year);
}

void dateToString(struct Date d, char *out) {
    sprintf(out, "%02d-%02d-%04d", d.day, d.month, d.year);
}

void sanitizeField(char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '|' || s[i] == '\n' || s[i] == '\r') s[i] = ' ';
    }
}

void saveBooks(void) {
    FILE *fp = fopen(BOOKS_FILE, "w");
    if (!fp) { printf("Error: could not save book data.\n"); return; }
    for (int i = 0; i < bookCount; i++) {
        struct Book *b = &books[i];
        fprintf(fp, "%d|%s|%s|%s|%s|%s|%s|%s|%s|%d|%d\n",
                b->bookID, b->title, b->author, b->isbn, b->publisher,
                b->edition, b->genre, b->subject, b->shelf,
                b->totalCopies, b->availableCopies);
    }
    fclose(fp);
}

void loadBooks(void) {
    bookCount = 0;
    FILE *fp = fopen(BOOKS_FILE, "r");
    if (!fp) return;

    char line[600];
    while (bookCount < MAX_BOOKS && fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        struct Book b;
        int n = sscanf(line, "%d|%99[^|]|%79[^|]|%19[^|]|%79[^|]|%29[^|]|%39[^|]|%39[^|]|%19[^|]|%d|%d",
                        &b.bookID, b.title, b.author, b.isbn, b.publisher,
                        b.edition, b.genre, b.subject, b.shelf,
                        &b.totalCopies, &b.availableCopies);
        if (n == 11) books[bookCount++] = b;
    }
    fclose(fp);
}

void saveMembers(void) {
    FILE *fp = fopen(MEMBERS_FILE, "w");
    if (!fp) { printf("Error: could not save member data.\n"); return; }
    for (int i = 0; i < memberCount; i++) {
        struct Member *m = &members[i];
        fprintf(fp, "%d|%s|%s|%s|%s|%s|%d|%d|%d|%d\n",
                m->memberID, m->name, m->address, m->contact, m->email,
                m->status, m->membershipDate.day, m->membershipDate.month,
                m->membershipDate.year, m->maxBooks);
    }
    fclose(fp);
}

void loadMembers(void) {
    memberCount = 0;
    FILE *fp = fopen(MEMBERS_FILE, "r");
    if (!fp) return;

    char line[600];
    while (memberCount < MAX_MEMBERS && fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        struct Member m;
        int n = sscanf(line, "%d|%79[^|]|%119[^|]|%19[^|]|%79[^|]|%14[^|]|%d|%d|%d|%d",
                        &m.memberID, m.name, m.address, m.contact, m.email,
                        m.status, &m.membershipDate.day, &m.membershipDate.month,
                        &m.membershipDate.year, &m.maxBooks);
        if (n == 10) members[memberCount++] = m;
    }
    fclose(fp);
}

void saveTransactions(void) {
    FILE *fp = fopen(TRANSACTIONS_FILE, "w");
    if (!fp) { printf("Error: could not save transaction data.\n"); return; }
    for (int i = 0; i < transactionCount; i++) {
        struct Transaction *t = &transactions[i];
        fprintf(fp, "%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%.2f\n",
                t->transactionID, t->memberID, t->bookID,
                t->issueDate.day, t->issueDate.month, t->issueDate.year,
                t->dueDate.day, t->dueDate.month, t->dueDate.year,
                t->returnDate.day, t->returnDate.month, t->returnDate.year,
                t->returned, t->fine);
    }
    fclose(fp);
}

void loadTransactions(void) {
    transactionCount = 0;
    FILE *fp = fopen(TRANSACTIONS_FILE, "r");
    if (fp) {
        char line[300];
        while (transactionCount < MAX_TRANSACTIONS && fgets(line, sizeof(line), fp)) {
            if (line[0] == '\n' || line[0] == '\0') continue;
            struct Transaction t;
            int n = sscanf(line, "%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%lf",
                            &t.transactionID, &t.memberID, &t.bookID,
                            &t.issueDate.day, &t.issueDate.month, &t.issueDate.year,
                            &t.dueDate.day, &t.dueDate.month, &t.dueDate.year,
                            &t.returnDate.day, &t.returnDate.month, &t.returnDate.year,
                            &t.returned, &t.fine);
            if (n == 14) transactions[transactionCount++] = t;
        }
        fclose(fp);
    }

    nextTransactionID = 1;
    for (int i = 0; i < transactionCount; i++) {
        if (transactions[i].transactionID >= nextTransactionID)
            nextTransactionID = transactions[i].transactionID + 1;
    }
}

void saveData(void) {
    saveBooks();
    saveMembers();
    saveTransactions();
}

void loadData(void) {
    loadBooks();
    loadMembers();
    loadTransactions();
}

int findBookByID(int bookID) {
    for (int i = 0; i < bookCount; i++)
        if (books[i].bookID == bookID) return i;
    return -1;
}

int findBookByISBN(const char *isbn) {
    for (int i = 0; i < bookCount; i++)
        if (strcmp(books[i].isbn, isbn) == 0) return i;
    return -1;
}

int findMemberByID(int memberID) {
    for (int i = 0; i < memberCount; i++)
        if (members[i].memberID == memberID) return i;
    return -1;
}

int findTransactionByID(int transactionID) {
    for (int i = 0; i < transactionCount; i++)
        if (transactions[i].transactionID == transactionID) return i;
    return -1;
}

int findActiveTransaction(int memberID, int bookID) {
    for (int i = 0; i < transactionCount; i++)
        if (transactions[i].memberID == memberID &&
            transactions[i].bookID == bookID &&
            transactions[i].returned == 0) return i;
    return -1;
}

int countActiveIssuesForMember(int memberID) {
    int count = 0;
    for (int i = 0; i < transactionCount; i++)
        if (transactions[i].memberID == memberID && transactions[i].returned == 0) count++;
    return count;
}

double getTransactionFine(int transactionIndex) {
    struct Transaction *t = &transactions[transactionIndex];
    if (t->returned) return t->fine;
    struct Date today = getCurrentDate();
    int overdue = daysBetween(t->dueDate, today);
    if (overdue < 0) overdue = 0;
    return overdue * FINE_PER_DAY;
}

void bookManagementMenu(void) {
    int choice;
    do {
        printf("\n---------------- BOOK MANAGEMENT ----------------\n");
        printf("1. Add New Book\n");
        printf("2. View Books\n");
        printf("3. Delete Book\n");
        printf("4. Exit (Back to Main Menu)\n");
        printSeparator('-', 50);
        choice = readIntInRange("Enter choice: ", 1, 4);

        switch (choice) {
            case 1: addBook(); break;
            case 2: viewBooks(); pressEnterToContinue(); break;
            case 3: deleteBook(); break;
            case 4: printf("Returning to Main Menu...\n"); break;
        }
    } while (choice != 4);
}

void addBook(void) {
    struct Book b;
    char buf[100];

    printf("\n---------------- ADD NEW BOOK ----------------\n");

    if (bookCount >= MAX_BOOKS) {
        printf("Book storage is full. Cannot add more books.\n");
        return;
    }

    while (1) {
        b.bookID = readInt("Enter Book ID: ");
        if (b.bookID <= 0) { printf("Book ID must be a positive number.\n"); continue; }
        if (findBookByID(b.bookID) != -1) {
            printf("A book with this ID already exists. Please use a different ID.\n");
            continue;
        }
        break;
    }

    do { readLine("Enter Title: ", buf, sizeof(buf)); } while (strlen(buf) == 0 && printf("Title cannot be empty.\n"));
    sanitizeField(buf);
    strcpy(b.title, buf);

    do { readLine("Enter Author: ", buf, sizeof(buf)); } while (strlen(buf) == 0 && printf("Author cannot be empty.\n"));
    sanitizeField(buf);
    strcpy(b.author, buf);

    while (1) {
        readLine("Enter ISBN: ", buf, sizeof(buf));
        sanitizeField(buf);
        if (strlen(buf) == 0) { printf("ISBN cannot be empty.\n"); continue; }
        if (findBookByISBN(buf) != -1) {
            printf("A book with this ISBN already exists. Please use a different ISBN.\n");
            continue;
        }
        break;
    }
    strcpy(b.isbn, buf);

    readLine("Enter Publisher: ", buf, sizeof(buf));
    sanitizeField(buf);
    strcpy(b.publisher, buf);

    readLine("Enter Edition: ", buf, sizeof(buf));
    sanitizeField(buf);
    strcpy(b.edition, buf);

    readLine("Enter Genre: ", buf, sizeof(buf));
    sanitizeField(buf);
    strcpy(b.genre, buf);

    readLine("Enter Subject: ", buf, sizeof(buf));
    sanitizeField(buf);
    strcpy(b.subject, buf);

    readLine("Enter Shelf/Rack Number: ", buf, sizeof(buf));
    sanitizeField(buf);
    strcpy(b.shelf, buf);

    while (1) {
        b.totalCopies = readInt("Enter Total Copies: ");
        if (b.totalCopies < 0) { printf("Total copies cannot be negative.\n"); continue; }
        break;
    }

    while (1) {
        b.availableCopies = readInt("Enter Available Copies: ");
        if (b.availableCopies < 0) { printf("Available copies cannot be negative.\n"); continue; }
        if (b.availableCopies > b.totalCopies) {
            printf("Available copies cannot exceed total copies (%d).\n", b.totalCopies);
            continue;
        }
        break;
    }

    books[bookCount++] = b;
    saveBooks();

    printSeparator('=', 50);
    printf("BOOK ADDED SUCCESSFULLY\n");
    printSeparator('=', 50);
}

void printBookRow(struct Book *b) {
    printf("%-5d %-20.20s %-15.15s %-14.14s %-10.10s %-8.8s %6d %6d\n",
           b->bookID, b->title, b->author, b->isbn, b->genre, b->shelf,
           b->totalCopies, b->availableCopies);
}

void viewBooks(void) {
    printf("\n---------------- ALL BOOKS ----------------\n");
    if (bookCount == 0) {
        printf("No books found in the library.\n");
        return;
    }
    printf("%-5s %-20s %-15s %-14s %-10s %-8s %6s %6s\n",
           "ID", "Title", "Author", "ISBN", "Genre", "Shelf", "Total", "Avail");
    printSeparator('-', 92);
    for (int i = 0; i < bookCount; i++) {
        printBookRow(&books[i]);
    }
    printf("\nAdditional details are shown when searching for a specific book.\n");
    printf("Total distinct titles: %d\n", bookCount);
}

void deleteBook(void) {
    printf("\n---------------- DELETE BOOK ----------------\n");
    if (bookCount == 0) { printf("No books to delete.\n"); return; }

    char key[20];
    readLine("Enter Book ID or ISBN to delete: ", key, sizeof(key));

    int idx = -1;
    int idAsInt = atoi(key);
    if (idAsInt > 0) idx = findBookByID(idAsInt);
    if (idx == -1) idx = findBookByISBN(key);

    if (idx == -1) {
        printf("No book found with that ID/ISBN.\n");
        return;
    }

    for (int i = 0; i < transactionCount; i++) {
        if (transactions[i].bookID == books[idx].bookID && transactions[i].returned == 0) {
            printf("Cannot delete this book: it is currently issued to a member.\n");
            printf("Please wait until all copies are returned.\n");
            return;
        }
    }

    printf("\nYou are about to permanently delete:\n");
    printBookRow(&books[idx]);
    char confirm[10];
    readLine("Are you sure? (y/n): ", confirm, sizeof(confirm));
    if (tolower((unsigned char)confirm[0]) != 'y') {
        printf("Deletion cancelled.\n");
        return;
    }

    for (int i = idx; i < bookCount - 1; i++) books[i] = books[i + 1];
    bookCount--;
    saveBooks();

    printSeparator('=', 50);
    printf("BOOK DELETED SUCCESSFULLY\n");
    printSeparator('=', 50);
}

void memberManagementMenu(void) {
    int choice;
    do {
        printf("\n--------------- MEMBER MANAGEMENT ----------------\n");
        printf("1. New Member\n");
        printf("2. Update Member Information\n");
        printf("3. Add Book (Issue to a member)\n");
        printf("4. Remove Book (Return from a member)\n");
        printf("5. Checkout (View a member's issued books)\n");
        printf("6. Exit (Back to Main Menu)\n");
        printSeparator('-', 51);
        choice = readIntInRange("Enter choice: ", 1, 6);

        switch (choice) {
            case 1: addMember(); break;
            case 2: updateMember(); break;
            case 3: addBookToCheckout(); break;
            case 4: removeBookFromCheckout(); break;
            case 5: checkoutView(); break;
            case 6: printf("Returning to Main Menu...\n"); break;
        }
    } while (choice != 6);
}

static int isValidContact(const char *s) {
    int len = (int)strlen(s);
    if (len < 7 || len > 15) return 0;
    for (int i = 0; i < len; i++)
        if (!isdigit((unsigned char)s[i]) && s[i] != '+') return 0;
    return 1;
}

static int isValidEmail(const char *s) {
    const char *at = strchr(s, '@');
    if (!at) return 0;
    const char *dot = strchr(at, '.');
    if (!dot) return 0;
    return 1;
}

void addMember(void) {
    struct Member m;
    char buf[120];

    printf("\n---------------- NEW MEMBER ----------------\n");

    if (memberCount >= MAX_MEMBERS) {
        printf("Member storage is full. Cannot add more members.\n");
        return;
    }

    while (1) {
        m.memberID = readInt("Enter Member ID: ");
        if (m.memberID <= 0) { printf("Member ID must be a positive number.\n"); continue; }
        if (findMemberByID(m.memberID) != -1) {
            printf("A member with this ID already exists. Please use a different ID.\n");
            continue;
        }
        break;
    }

    do { readLine("Enter Full Name: ", buf, sizeof(buf)); } while (strlen(buf) == 0 && printf("Name cannot be empty.\n"));
    sanitizeField(buf);
    strcpy(m.name, buf);

    readLine("Enter Address: ", buf, sizeof(buf));
    sanitizeField(buf);
    strcpy(m.address, buf);

    while (1) {
        readLine("Enter Contact Number: ", buf, sizeof(buf));
        if (!isValidContact(buf)) { printf("Invalid contact number (7-15 digits expected).\n"); continue; }
        break;
    }
    sanitizeField(buf);
    strcpy(m.contact, buf);

    while (1) {
        readLine("Enter Email Address: ", buf, sizeof(buf));
        if (!isValidEmail(buf)) { printf("Invalid email address.\n"); continue; }
        break;
    }
    sanitizeField(buf);
    strcpy(m.email, buf);

    printf("Membership Status options: 1) Active 2) Inactive 3) Suspended\n");
    int statusChoice = readIntInRange("Choose status: ", 1, 3);
    if (statusChoice == 1) strcpy(m.status, "Active");
    else if (statusChoice == 2) strcpy(m.status, "Inactive");
    else strcpy(m.status, "Suspended");

    m.membershipDate = getCurrentDate();
    printf("Membership Date recorded as today: ");
    printDate(m.membershipDate);
    printf("\n");

    while (1) {
        m.maxBooks = readInt("Enter Maximum Books Allowed: ");
        if (m.maxBooks <= 0) { printf("Maximum books allowed must be positive.\n"); continue; }
        break;
    }

    members[memberCount++] = m;
    saveMembers();

    printSeparator('=', 50);
    printf("MEMBER REGISTERED SUCCESSFULLY\n");
    printSeparator('=', 50);
}

void updateMember(void) {
    printf("\n---------------- UPDATE MEMBER INFORMATION ----------------\n");
    int id = readInt("Enter Member ID to update: ");
    int idx = findMemberByID(id);
    if (idx == -1) { printf("No member found with ID %d.\n", id); return; }

    struct Member *m = &members[idx];
    char buf[120];

    printf("Leave a field blank to keep its current value.\n\n");

    printf("Current Name: %s\n", m->name);
    readLine("New Name: ", buf, sizeof(buf));
    if (strlen(buf) > 0) { sanitizeField(buf); strcpy(m->name, buf); }

    printf("Current Address: %s\n", m->address);
    readLine("New Address: ", buf, sizeof(buf));
    if (strlen(buf) > 0) { sanitizeField(buf); strcpy(m->address, buf); }

    printf("Current Contact: %s\n", m->contact);
    readLine("New Contact: ", buf, sizeof(buf));
    if (strlen(buf) > 0) {
        if (isValidContact(buf)) { sanitizeField(buf); strcpy(m->contact, buf); }
        else printf("Invalid contact number - keeping the old value.\n");
    }

    printf("Current Email: %s\n", m->email);
    readLine("New Email: ", buf, sizeof(buf));
    if (strlen(buf) > 0) {
        if (isValidEmail(buf)) { sanitizeField(buf); strcpy(m->email, buf); }
        else printf("Invalid email - keeping the old value.\n");
    }

    printf("Current Status: %s\n", m->status);
    printf("Status options: 1) Active 2) Inactive 3) Suspended 4) Keep current\n");
    int statusChoice = readIntInRange("Choose status: ", 1, 4);
    if (statusChoice == 1) strcpy(m->status, "Active");
    else if (statusChoice == 2) strcpy(m->status, "Inactive");
    else if (statusChoice == 3) strcpy(m->status, "Suspended");

    saveMembers();

    printSeparator('=', 50);
    printf("MEMBER INFORMATION UPDATED SUCCESSFULLY\n");
    printSeparator('=', 50);
}

void addBookToCheckout(void) {
    printf("\n---------------- ADD BOOK TO MEMBER ----------------\n");
    int memberID = readInt("Enter Member ID: ");
    int bookID = readInt("Enter Book ID: ");

    int transID;
    char err[150];
    int status = issueBookCore(memberID, bookID, &transID, err);
    if (status != 0) {
        printf("Could not issue book: %s\n", err);
        return;
    }

    printSeparator('=', 50);
    printf("BOOK ISSUED SUCCESSFULLY (Transaction ID: %d)\n", transID);
    printSeparator('=', 50);
}

void removeBookFromCheckout(void) {
    printf("\n---------------- REMOVE BOOK FROM MEMBER ----------------\n");
    int memberID = readInt("Enter Member ID: ");
    int bookID = readInt("Enter Book ID: ");

    int tIdx = findActiveTransaction(memberID, bookID);
    if (tIdx == -1) {
        printf("No active issuance found for this member/book combination.\n");
        return;
    }

    double fine;
    char err[150];
    int status = returnBookCore(tIdx, &fine, err);
    if (status != 0) {
        printf("Could not return book: %s\n", err);
        return;
    }

    printSeparator('=', 50);
    printf("BOOK RETURNED SUCCESSFULLY\n");
    if (fine > 0) printf("Fine Due: Rs. %.2f\n", fine);
    else printf("No fine due.\n");
    printSeparator('=', 50);
}

void printMemberIssuedBooks(int memberID) {
    printf("%-4s %-20s %-12s %-12s %6s %-10s %8s\n",
           "TID", "Title", "Issue Date", "Due Date", "Days", "Status", "Fine");
    printSeparator('-', 80);

    int found = 0;
    struct Date today = getCurrentDate();

    for (int i = 0; i < transactionCount; i++) {
        if (transactions[i].memberID != memberID) continue;
        found = 1;

        struct Transaction *t = &transactions[i];
        int bIdx = findBookByID(t->bookID);
        const char *title = (bIdx != -1) ? books[bIdx].title : "(deleted book)";

        int daysBorrowed;
        const char *status;
        if (t->returned) {
            daysBorrowed = daysBetween(t->issueDate, t->returnDate);
            status = "Returned";
        } else {
            daysBorrowed = daysBetween(t->issueDate, today);
            status = (daysBetween(t->dueDate, today) > 0) ? "Overdue" : "Issued";
        }

        char issueStr[12], dueStr[12];
        dateToString(t->issueDate, issueStr);
        dateToString(t->dueDate, dueStr);

        printf("%-4d %-20.20s %-12s %-12s %6d %-10s %8.2f\n",
               t->transactionID, title, issueStr, dueStr, daysBorrowed,
               status, getTransactionFine(i));
    }

    if (!found) printf("No issuance records found for this member.\n");
}

void checkoutView(void) {
    printf("\n---------------- MEMBER CHECKOUT ----------------\n");
    int memberID = readInt("Enter Member ID: ");
    int idx = findMemberByID(memberID);
    if (idx == -1) { printf("No member found with ID %d.\n", memberID); return; }

    printf("\nMember: %s (ID: %d)\n", members[idx].name, members[idx].memberID);
    printMemberIssuedBooks(memberID);
    pressEnterToContinue();
}

void issuanceReturnMenu(void) {
    int choice;
    do {
        printf("\n------------- ISSUANCE AND RETURN ----------------\n");
        printf("1. Issue Book\n");
        printf("2. Return Book\n");
        printf("3. View Issuance Records\n");
        printf("4. Calculate Fine\n");
        printf("5. Exit (Back to Main Menu)\n");
        printSeparator('-', 51);
        choice = readIntInRange("Enter choice: ", 1, 5);

        switch (choice) {
            case 1: issueBook(); break;
            case 2: returnBook(); break;
            case 3: viewIssuanceRecords(); pressEnterToContinue(); break;
            case 4: calculateFineMenu(); break;
            case 5: printf("Returning to Main Menu...\n"); break;
        }
    } while (choice != 5);
}

int issueBookCore(int memberID, int bookID, int *outTransactionID, char *err) {
    int mIdx = findMemberByID(memberID);
    if (mIdx == -1) { strcpy(err, "Member not found."); return -1; }

    if (strcmp(members[mIdx].status, "Active") != 0) {
        strcpy(err, "Member's membership is not Active.");
        return -2;
    }

    int bIdx = findBookByID(bookID);
    if (bIdx == -1) { strcpy(err, "Book not found."); return -3; }

    if (books[bIdx].availableCopies <= 0) {
        strcpy(err, "No available copies of this book.");
        return -4;
    }

    if (countActiveIssuesForMember(memberID) >= members[mIdx].maxBooks) {
        strcpy(err, "Member has reached their maximum borrowing limit.");
        return -5;
    }

    if (findActiveTransaction(memberID, bookID) != -1) {
        strcpy(err, "Member already has this book checked out.");
        return -6;
    }

    if (transactionCount >= MAX_TRANSACTIONS) {
        strcpy(err, "Transaction storage is full.");
        return -7;
    }

    struct Transaction t;
    t.transactionID = nextTransactionID++;
    t.memberID = memberID;
    t.bookID = bookID;
    t.issueDate = getCurrentDate();
    t.dueDate = calculateDueDate(t.issueDate);
    t.returnDate.day = 0; t.returnDate.month = 0; t.returnDate.year = 0;
    t.returned = 0;
    t.fine = 0.0;

    transactions[transactionCount++] = t;
    books[bIdx].availableCopies--;

    saveTransactions();
    saveBooks();

    if (outTransactionID) *outTransactionID = t.transactionID;
    return 0;
}

int returnBookCore(int transactionIndex, double *outFine, char *err) {
    struct Transaction *t = &transactions[transactionIndex];
    if (t->returned) { strcpy(err, "This book has already been returned."); return -1; }

    t->returnDate = getCurrentDate();
    int overdue = daysBetween(t->dueDate, t->returnDate);
    if (overdue < 0) overdue = 0;
    t->fine = overdue * FINE_PER_DAY;
    t->returned = 1;

    int bIdx = findBookByID(t->bookID);
    if (bIdx != -1) {
        if (books[bIdx].availableCopies < books[bIdx].totalCopies)
            books[bIdx].availableCopies++;
    }

    saveTransactions();
    saveBooks();

    if (outFine) *outFine = t->fine;
    return 0;
}

void issueBook(void) {
    printf("\n---------------- ISSUE BOOK ----------------\n");
    int memberID = readInt("Enter Member ID: ");
    int bookID = readInt("Enter Book ID: ");

    int transID;
    char err[150];
    int status = issueBookCore(memberID, bookID, &transID, err);
    if (status != 0) {
        printf("Issuance failed: %s\n", err);
        return;
    }

    int tIdx = findTransactionByID(transID);
    printSeparator('=', 50);
    printf("BOOK ISSUED SUCCESSFULLY\n");
    printf("Transaction ID : %d\n", transID);
    printf("Member         : %s (ID %d)\n", members[findMemberByID(memberID)].name, memberID);
    printf("Book           : %s (ID %d)\n", books[findBookByID(bookID)].title, bookID);
    printf("Issue Date     : "); printDate(transactions[tIdx].issueDate); printf("\n");
    printf("Due Date       : "); printDate(transactions[tIdx].dueDate); printf("\n");
    printSeparator('=', 50);
}

void returnBook(void) {
    printf("\n---------------- RETURN BOOK ----------------\n");
    printf("1. Return using Transaction ID\n");
    printf("2. Return using Member ID + Book ID\n");
    int mode = readIntInRange("Choose an option: ", 1, 2);

    int tIdx = -1;
    if (mode == 1) {
        int tid = readInt("Enter Transaction ID: ");
        tIdx = findTransactionByID(tid);
        if (tIdx == -1) { printf("No transaction found with that ID.\n"); return; }
        if (transactions[tIdx].returned) { printf("This book was already returned.\n"); return; }
    } else {
        int memberID = readInt("Enter Member ID: ");
        int bookID = readInt("Enter Book ID: ");
        tIdx = findActiveTransaction(memberID, bookID);
        if (tIdx == -1) {
            printf("No active (unreturned) issuance found for that member/book.\n");
            return;
        }
    }

    double fine;
    char err[150];
    int status = returnBookCore(tIdx, &fine, err);
    if (status != 0) {
        printf("Return failed: %s\n", err);
        return;
    }

    struct Transaction *t = &transactions[tIdx];
    int daysBorrowed = daysBetween(t->issueDate, t->returnDate);
    int overdueDays = daysBetween(t->dueDate, t->returnDate);
    if (overdueDays < 0) overdueDays = 0;

    printSeparator('=', 50);
    printf("BOOK RETURNED SUCCESSFULLY\n");
    printf("Transaction ID : %d\n", t->transactionID);
    printf("Return Date    : "); printDate(t->returnDate); printf("\n");
    printf("Days Borrowed  : %d\n", daysBorrowed);
    printf("Overdue Days   : %d\n", overdueDays);
    printf("Fine Amount    : Rs. %.2f\n", fine);
    printSeparator('=', 50);
}

void viewIssuanceRecords(void) {
    printf("\n---------------- ISSUANCE RECORDS ----------------\n");
    if (transactionCount == 0) { printf("No issuance records found.\n"); return; }

    printf("%-4s %-6s %-6s %-12s %-12s %-10s %8s\n",
           "TID", "MemID", "BookID", "Issue Date", "Due Date", "Status", "Fine");
    printSeparator('-', 70);

    for (int i = 0; i < transactionCount; i++) {
        struct Transaction *t = &transactions[i];
        char issueStr[12], dueStr[12];
        dateToString(t->issueDate, issueStr);
        dateToString(t->dueDate, dueStr);
        const char *status = t->returned ? "Returned" : "Issued";
        printf("%-4d %-6d %-6d %-12s %-12s %-10s %8.2f\n",
               t->transactionID, t->memberID, t->bookID, issueStr, dueStr,
               status, getTransactionFine(i));
    }
}

void calculateFineMenu(void) {
    printf("\n---------------- CALCULATE FINE ----------------\n");
    int tid = readInt("Enter Transaction ID: ");
    int idx = findTransactionByID(tid);
    if (idx == -1) { printf("No transaction found with that ID.\n"); return; }

    struct Transaction *t = &transactions[idx];
    double fine = getTransactionFine(idx);

    printf("\nTransaction ID : %d\n", t->transactionID);
    printf("Member ID      : %d\n", t->memberID);
    printf("Book ID        : %d\n", t->bookID);
    printf("Issue Date     : "); printDate(t->issueDate); printf("\n");
    printf("Due Date       : "); printDate(t->dueDate); printf("\n");
    if (t->returned) {
        printf("Return Date    : "); printDate(t->returnDate); printf("\n");
        printf("Status         : Returned\n");
    } else {
        struct Date today = getCurrentDate();
        printf("Status         : Still Issued (fine calculated as of today, ");
        printDate(today);
        printf(")\n");
    }
    printf("Fine Amount    : Rs. %.2f\n", fine);
    pressEnterToContinue();
}

void searchRecordsMenu(void) {
    int choice;
    do {
        printf("\n---------------- SEARCH RECORDS ------------------\n");
        printf("1. Search Books\n");
        printf("2. Search Members\n");
        printf("3. Exit (Back to Main Menu)\n");
        printSeparator('-', 51);
        choice = readIntInRange("Enter choice: ", 1, 3);

        switch (choice) {
            case 1: searchBooks(); break;
            case 2: searchMembers(); break;
            case 3: printf("Returning to Main Menu...\n"); break;
        }
    } while (choice != 3);
}

void searchBooks(void) {
    printf("\n---------------- SEARCH BOOKS ----------------\n");
    printf("Search by:\n");
    printf("1. Title\n2. Author\n3. ISBN\n4. Publisher\n5. Edition\n6. Genre\n7. Subject\n8. Shelf/Rack\n");
    int field = readIntInRange("Choose field: ", 1, 8);

    char term[100];
    readLine("Enter search term: ", term, sizeof(term));

    int found = 0;
    printf("\n%-5s %-20s %-15s %-14s %-10s %-8s %6s %6s\n",
           "ID", "Title", "Author", "ISBN", "Genre", "Shelf", "Total", "Avail");
    printSeparator('-', 92);

    for (int i = 0; i < bookCount; i++) {
        struct Book *b = &books[i];
        const char *value = NULL;
        switch (field) {
            case 1: value = b->title; break;
            case 2: value = b->author; break;
            case 3: value = b->isbn; break;
            case 4: value = b->publisher; break;
            case 5: value = b->edition; break;
            case 6: value = b->genre; break;
            case 7: value = b->subject; break;
            case 8: value = b->shelf; break;
        }
        if (value && containsIgnoreCase(value, term)) {
            printBookRow(b);
            found = 1;
        }
    }

    if (!found) printf("No matching books found.\n");
    pressEnterToContinue();
}

void searchMembers(void) {
    printf("\n---------------- SEARCH MEMBERS ----------------\n");
    printf("Search by:\n");
    printf("1. Member Name\n2. Member ID\n3. Membership Status\n4. Contact Information\n");
    int field = readIntInRange("Choose field: ", 1, 4);

    char term[100];
    int searchID = 0;
    if (field == 2) searchID = readInt("Enter Member ID: ");
    else readLine("Enter search term: ", term, sizeof(term));

    int found = 0;
    printf("\n%-6s %-20s %-14s %-24s %-10s %6s\n",
           "ID", "Name", "Contact", "Email", "Status", "Books");
    printSeparator('-', 84);

    for (int i = 0; i < memberCount; i++) {
        struct Member *m = &members[i];
        int matches = 0;
        if (field == 1) matches = containsIgnoreCase(m->name, term);
        else if (field == 2) matches = (m->memberID == searchID);
        else if (field == 3) matches = containsIgnoreCase(m->status, term);
        else if (field == 4) matches = containsIgnoreCase(m->contact, term) || containsIgnoreCase(m->email, term);

        if (matches) {
            printf("%-6d %-20.20s %-14s %-24.24s %-10s %6d\n",
                   m->memberID, m->name, m->contact, m->email, m->status,
                   countActiveIssuesForMember(m->memberID));
            found = 1;
        }
    }

    if (!found) printf("No matching members found.\n");
    pressEnterToContinue();
}

void loginMenu(void) {
    int choice;
    do {
        printf("\n==================================================\n");
        printf("               LOGIN / ROLE SELECTION\n");
        printf("==================================================\n");
        printf("1. Login as Library Staff\n");
        printf("2. Login as Library Member\n");
        printf("3. Exit\n");
        printSeparator('=', 50);
        choice = readIntInRange("Enter choice: ", 1, 3);

        if (choice == 1) {
            char user[30], pass[30];
            readLine("Username: ", user, sizeof(user));
            readLine("Password: ", pass, sizeof(pass));
            if (strcmp(user, STAFF_USERNAME) == 0 && strcmp(pass, STAFF_PASSWORD) == 0) {
                printf("\nLogin successful. Welcome, Library Staff!\n");
                staffMenu();
            } else {
                printf("\nInvalid username or password.\n");
            }
        } else if (choice == 2) {
            int id = readInt("Enter your Member ID: ");
            int idx = findMemberByID(id);
            if (idx == -1) {
                printf("\nNo member found with ID %d. Please contact library staff.\n", id);
            } else {
                printf("\nLogin successful. Welcome, %s!\n", members[idx].name);
                memberMenu(id);
            }
        } else {
            printf("\nExiting Login Menu...\n");
        }
    } while (choice != 3);
}

void staffMenu(void) {
    int choice;
    do {
        printf("\n==================================================\n");
        printf("               LIBRARY MANAGEMENT SYSTEM\n");
        printf("==================================================\n");
        printf("1. Book Management\n");
        printf("2. Member Management\n");
        printf("3. Issuance and Return\n");
        printf("4. Search Records\n");
        printf("5. View Fine Information\n");
        printf("6. Save Data and Exit\n");
        printSeparator('=', 50);
        choice = readIntInRange("Enter choice: ", 1, 6);

        switch (choice) {
            case 1: bookManagementMenu(); break;
            case 2: memberManagementMenu(); break;
            case 3: issuanceReturnMenu(); break;
            case 4: searchRecordsMenu(); break;
            case 5: {
                printf("\n---------------- FINE INFORMATION (ALL MEMBERS) ----------------\n");
                double grandTotal = 0;
                int any = 0;
                for (int i = 0; i < transactionCount; i++) {
                    double f = getTransactionFine(i);
                    if (f > 0) {
                        any = 1;
                        int mIdx = findMemberByID(transactions[i].memberID);
                        const char *name = (mIdx != -1) ? members[mIdx].name : "(unknown)";
                        printf("Transaction %-4d | Member %-4d (%s) | Fine: Rs. %.2f\n",
                               transactions[i].transactionID, transactions[i].memberID, name, f);
                        grandTotal += f;
                    }
                }
                if (!any) printf("No outstanding fines at this time.\n");
                else printf("\nTotal Outstanding Fines (Library-wide): Rs. %.2f\n", grandTotal);
                pressEnterToContinue();
                break;
            }
            case 6:
                printf("\nSaving data...\n");
                saveData();
                printf("Data saved. Logging out of Staff account.\n");
                break;
        }
    } while (choice != 6);
}

void memberMenu(int memberID) {
    int choice;
    int idx = findMemberByID(memberID);
    if (idx == -1) return;

    do {
        printf("\n==================================================\n");
        printf("               LIBRARY MEMBER PORTAL\n");
        printf("==================================================\n");
        printf("1. View My Member Details\n");
        printf("2. View My Issued Books\n");
        printf("3. View My Fine Information\n");
        printf("4. View Books Available In Library\n");
        printf("5. Search Books\n");
        printf("6. Exit (Logout)\n");
        printSeparator('=', 50);
        choice = readIntInRange("Enter choice: ", 1, 6);

        struct Member *m = &members[idx];

        switch (choice) {
            case 1:
                printf("\n---------------- MY MEMBER DETAILS ----------------\n");
                printf("Member ID   : %d\n", m->memberID);
                printf("Name        : %s\n", m->name);
                printf("Address     : %s\n", m->address);
                printf("Contact     : %s\n", m->contact);
                printf("Email       : %s\n", m->email);
                printf("Status      : %s\n", m->status);
                printf("Member Since: "); printDate(m->membershipDate); printf("\n");
                printf("Max Books   : %d\n", m->maxBooks);
                pressEnterToContinue();
                break;

            case 2:
                printf("\n---------------- MY ISSUED BOOKS ----------------\n");
                printMemberIssuedBooks(memberID);
                pressEnterToContinue();
                break;

            case 3: {
                printf("\n---------------- MY FINE INFORMATION ----------------\n");
                double total = 0;
                int any = 0;
                for (int i = 0; i < transactionCount; i++) {
                    if (transactions[i].memberID != memberID) continue;
                    double f = getTransactionFine(i);
                    if (f > 0) {
                        any = 1;
                        printf("Transaction %-4d | Fine: Rs. %.2f\n", transactions[i].transactionID, f);
                        total += f;
                    }
                }
                if (!any) printf("You have no outstanding fines. \n");
                else printf("\nTotal Fine Due: Rs. %.2f\n", total);
                pressEnterToContinue();
                break;
            }

            case 4:
                viewBooks();
                pressEnterToContinue();
                break;

            case 5:
                searchBooks();
                break;

            case 6:
                printf("\nLogging out...\n");
                break;
        }
    } while (choice != 6);
}