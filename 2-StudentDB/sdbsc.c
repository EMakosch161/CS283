#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // For file system calls
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

// database include files
#include "db.h"
#include "sdbsc.h"

/*
 *  open_db
 *      dbFile:         name of the database file
 *      should_truncate: if true, empties the file when opening
 *
 *  returns: File descriptor on success, or ERR_DB_FILE on error
 *
 *  console: No output on success; prints M_ERR_DB_OPEN on error.
 */
int open_db(char *dbFile, bool should_truncate)
{
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
    int flags = O_RDWR | O_CREAT;
    if (should_truncate)
        flags |= O_TRUNC;

    int fd = open(dbFile, flags, mode);
    if (fd == -1)
    {
        printf(M_ERR_DB_OPEN);
        return ERR_DB_FILE;
    }
    return fd;
}

/*
 *  get_student
 *      fd: file descriptor
 *      id: student id to look for
 *      *s: pointer to where the student data should be copied
 *
 *  Returns NO_ERROR if found, ERR_DB_FILE on I/O error, or SRCH_NOT_FOUND if not found.
 *
 *  console: No output.
 */
int get_student(int fd, int id, student_t *s)
{
    if (id < MIN_STD_ID || id > MAX_STD_ID)
        return SRCH_NOT_FOUND;

    off_t offSet = id * STUDENT_RECORD_SIZE;
    if (lseek(fd, offSet, SEEK_SET) == -1)
        return ERR_DB_FILE;

    ssize_t bytesRead = read(fd, s, STUDENT_RECORD_SIZE);
    if (bytesRead == -1)
        return ERR_DB_FILE;
    else if (bytesRead < STUDENT_RECORD_SIZE || s->id == DELETED_STUDENT_ID)
        return SRCH_NOT_FOUND;

    return NO_ERROR;
}

/*
 *  add_student
 *      fd:    file descriptor
 *      id:    student id
 *      fname: student first name
 *      lname: student last name
 *      gpa:   GPA as an integer (e.g., 341 represents 3.41)
 *
 *  Adds a student if the record is empty. Prints errors if the record exists or if I/O fails.
 *
 *  Returns NO_ERROR on success, ERR_DB_FILE on I/O error, or ERR_DB_OP if duplicate.
 */
int add_student(int fd, int id, char *fname, char *lname, int gpa)
{
    off_t offSet = id * STUDENT_RECORD_SIZE;
    student_t student;

    if (lseek(fd, offSet, SEEK_SET) < 0)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }

    ssize_t bytesRead = read(fd, &student, STUDENT_RECORD_SIZE);
    if (bytesRead == -1)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    else if (bytesRead < STUDENT_RECORD_SIZE)
    {
        memset(&student, 0, STUDENT_RECORD_SIZE);
    }

    if (memcmp(&student, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != 0)
    {
        printf(M_ERR_DB_ADD_DUP, id);
        return ERR_DB_OP;
    }

    student.id = id;
    strncpy(student.fname, fname, sizeof(student.fname) - 1);
    student.fname[sizeof(student.fname) - 1] = '\0';
    strncpy(student.lname, lname, sizeof(student.lname) - 1);
    student.lname[sizeof(student.lname) - 1] = '\0';
    student.gpa = gpa;

    if (lseek(fd, offSet, SEEK_SET) < 0)
    {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }

    if (write(fd, &student, STUDENT_RECORD_SIZE) != STUDENT_RECORD_SIZE)
    {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }

    printf(M_STD_ADDED, id);
    return NO_ERROR;
}

/*
 *  del_student
 *      fd: file descriptor
 *      id: student id to delete
 *
 *  Deletes a student by writing an empty record if the student exists.
 *
 *  Returns NO_ERROR on success, ERR_DB_FILE on I/O error, or ERR_DB_OP if student not found.
 */
int del_student(int fd, int id)
{
    student_t student;
    int result = get_student(fd, id, &student);
    if (result == SRCH_NOT_FOUND)
    {
        printf(M_STD_NOT_FND_MSG, id);
        return ERR_DB_OP;
    }
    else if (result == ERR_DB_FILE)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }

    off_t offSet = id * STUDENT_RECORD_SIZE;
    if (lseek(fd, offSet, SEEK_SET) < 0)
    {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }
    if (write(fd, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != STUDENT_RECORD_SIZE)
    {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }
    printf(M_STD_DEL_MSG, id);
    return NO_ERROR;
}

/*
 *  count_db_records
 *      fd: file descriptor
 *
 *  Reads through the database and counts non-empty student records.
 *
 *  Returns the count on success, or ERR_DB_FILE on I/O error.
 *
 *  Console: Prints M_DB_RECORD_CNT with the count or M_DB_EMPTY if none found.
 */
int count_db_records(int fd)
{
    student_t student;
    int count = 0;
    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    while (1)
    {
        ssize_t bytesRead = read(fd, &student, STUDENT_RECORD_SIZE);
        if (bytesRead < 0)
        {
            printf(M_ERR_DB_READ);
            return ERR_DB_FILE;
        }
        if (bytesRead == 0)
            break;
        if (bytesRead < STUDENT_RECORD_SIZE)
        {
            printf(M_ERR_DB_READ);
            return ERR_DB_FILE;
        }
        if (memcmp(&student, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != 0 &&
            student.id != DELETED_STUDENT_ID)
        {
            count++;
        }
    }
    if (count == 0)
        printf(M_DB_EMPTY);
    else
        printf(M_DB_RECORD_CNT, count);
    return count;
}

/*
 *  print_db
 *      fd: file descriptor
 *
 *  Reads and prints all valid student records. Prints a header once before any records.
 *
 *  Returns NO_ERROR on success, or ERR_DB_FILE on I/O error.
 *
 *  Console: Displays a table of student records or an empty message.
 */
int print_db(int fd)
{
    student_t student;
    bool headerShown = false;
    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }
    while (read(fd, &student, STUDENT_RECORD_SIZE) > 0)
    {
        if (memcmp(&student, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != 0 &&
            student.id != DELETED_STUDENT_ID)
        {
            if (!headerShown)
            {
                printf(STUDENT_PRINT_HDR_STRING, "ID", "FIRST_NAME", "LAST_NAME", "GPA");
                headerShown = true;
            }
            float realGpa = student.gpa / 100.0;
            printf(STUDENT_PRINT_FMT_STRING, student.id, student.fname, student.lname, realGpa);
        }
    }
    if (!headerShown)
        printf(M_DB_EMPTY);
    return NO_ERROR;
}

/*
 *  print_student
 *      *s: pointer to a student_t structure to print
 *
 *  Validates the student pointer and prints a header then the student record.
 *
 *  Returns nothing.
 *
 *  Console: Displays the student record or prints M_ERR_STD_PRINT if invalid.
 */
void print_student(student_t *s)
{
    if (s == NULL || s->id == DELETED_STUDENT_ID)
    {
        printf(M_ERR_STD_PRINT);
    }
    else
    {
        float realGpa = s->gpa / 100.0;
        printf(STUDENT_PRINT_HDR_STRING, "ID", "FIRST_NAME", "LAST_NAME", "GPA");
        printf(STUDENT_PRINT_FMT_STRING, s->id, s->fname, s->lname, realGpa);
    }
}

/*
 *  compress_db
 *      fd: file descriptor of the current database file
 *
 *  [Extra Credit]
 *  This function is not implemented.
 *
 *  Returns the original fd.
 *
 *  Console: Prints M_NOT_IMPL.
 */
int compress_db(int fd)
{
    printf(M_NOT_IMPL);
    return fd;
}

/*
 *  validate_range
 *      id: proposed student id
 *      gpa: proposed GPA (as an integer)
 *
 *  Checks if id and gpa are within the allowed ranges.
 *
 *  Returns NO_ERROR if valid, or EXIT_FAIL_ARGS if not.
 *
 *  Console: No output.
 */
int validate_range(int id, int gpa)
{
    if (id < MIN_STD_ID || id > MAX_STD_ID)
        return EXIT_FAIL_ARGS;
    if (gpa < MIN_STD_GPA || gpa > MAX_STD_GPA)
        return EXIT_FAIL_ARGS;
    return NO_ERROR;
}

/*
 *  usage
 *      exename: executable name (argv[0])
 *
 *  Prints the program usage information.
 *
 *  Returns nothing.
 *
 *  Console: Displays usage information.
 */
void usage(char *exename)
{
    printf("usage: %s -[h|a|c|d|f|p|z] options.  Where:\n", exename);
    printf("\t-h:  prints help\n");
    printf("\t-a id first_name last_name gpa(as 3 digit int):  adds a student\n");
    printf("\t-c:  counts the records in the database\n");
    printf("\t-d id:  deletes a student\n");
    printf("\t-f id:  finds and prints a student in the database\n");
    printf("\t-p:  prints all records in the student database\n");
    printf("\t-x:  compress the database file [EXTRA CREDIT]\n");
    printf("\t-z:  zero db file (remove all records)\n");
}

/* Welcome to main() */
int main(int argc, char *argv[])
{
    char opt;      // user selected option
    int fd;        // file descriptor for database files
    int rc;        // return code from operations
    int exit_code; // exit code to shell
    int id;        // student id from argv[2]
    int gpa;       // gpa from argv[5]
    student_t student = {0};

    if ((argc < 2) || (*argv[1] != '-'))
    {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1);
    if (opt == 'h')
    {
        usage(argv[0]);
        exit(EXIT_OK);
    }

    fd = open_db(DB_FILE, false);
    if (fd < 0)
        exit(EXIT_FAIL_DB);

    exit_code = EXIT_OK;
    switch (opt)
    {
        case 'a':
            if (argc != 6)
            {
                usage(argv[0]);
                exit_code = EXIT_FAIL_ARGS;
                break;
            }
            id = atoi(argv[2]);
            gpa = atoi(argv[5]);
            exit_code = validate_range(id, gpa);
            if (exit_code == EXIT_FAIL_ARGS)
            {
                printf(M_ERR_STD_RNG);
                break;
            }
            rc = add_student(fd, id, argv[3], argv[4], gpa);
            if (rc < 0)
                exit_code = EXIT_FAIL_DB;
            break;

        case 'c':
            rc = count_db_records(fd);
            if (rc < 0)
                exit_code = EXIT_FAIL_DB;
            break;

        case 'd':
            if (argc != 3)
            {
                usage(argv[0]);
                exit_code = EXIT_FAIL_ARGS;
                break;
            }
            id = atoi(argv[2]);
            rc = del_student(fd, id);
            if (rc < 0)
                exit_code = EXIT_FAIL_DB;
            break;

        case 'f':
            if (argc != 3)
            {
                usage(argv[0]);
                exit_code = EXIT_FAIL_ARGS;
                break;
            }
            id = atoi(argv[2]);
            rc = get_student(fd, id, &student);
            switch (rc)
            {
                case NO_ERROR:
                    print_student(&student);
                    break;
                case SRCH_NOT_FOUND:
                    printf(M_STD_NOT_FND_MSG, id);
                    exit_code = EXIT_FAIL_DB;
                    break;
                default:
                    printf(M_ERR_DB_READ);
                    exit_code = EXIT_FAIL_DB;
                    break;
            }
            break;

        case 'p':
            rc = print_db(fd);
            if (rc < 0)
                exit_code = EXIT_FAIL_DB;
            break;

        case 'x':
            fd = compress_db(fd);
            if (fd < 0)
                exit_code = EXIT_FAIL_DB;
            break;

        case 'z':
            close(fd);
            fd = open_db(DB_FILE, true);
            if (fd < 0)
            {
                exit_code = EXIT_FAIL_DB;
                break;
            }
            printf(M_DB_ZERO_OK);
            exit_code = EXIT_OK;
            break;

        default:
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
    }

    close(fd);
    exit(exit_code);
}