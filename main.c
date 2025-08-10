// =========================
// sms.c  — Part 1 of 4
// Includes, constants, structs, and utility functions
// Updated: admission entries keep status (pending/approved) so approvals don't remove them,
// and username checks are done carefully so approval can create the login.
// =========================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  #include <conio.h>
#else
  #include <unistd.h>
#endif

// -------------------------
// Configuration & constants
// -------------------------
#define ADMISSION_FILE   "admission_requests.txt" // format: tempId,name,department,semester,email,username,password,status
#define STUDENTS_FILE    "students.txt"           // format: id,name,department,semester,cgpa
#define LOGINS_FILE      "logins.txt"             // format: username,password,role,studentId
#define MARKSHEET_FILE   "marksheets.txt"
#define TEMP_FILE        "temp.txt"

#define MAX_LINE         1024
#define MAX_NAME         100
#define MAX_DEPT         60
#define MAX_USERNAME     50
#define MAX_PASS         50
#define MAX_SUBJECT      60
#define MAX_TOKEN        200
#define MAX_STATUS       16  // pending / approved

// -------------------------
// Data structures
// -------------------------
typedef struct {
    int id;                         // student ID
    char name[MAX_NAME];
    char department[MAX_DEPT];
    int semester;
    float cgpa;
} Student;

typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASS];
    char role[16]; // "admin" or "student"
    int studentId; // linked student ID (0 or -1 for admin)
} LoginEntry;

// -------------------------
// Forward declarations (other parts will implement these)
// -------------------------

/* admission + id helpers */
int nextAdmissionTempId();
int nextStudentId();

/* username checks */
int usernameExistsInLogins(const char *username);            // checks LOGINS_FILE only
int usernameExistsInAdmissionsPending(const char *username); // checks admission file only for pending entries
int usernameExists(const char *username);                    // combined check used at registration time

/* login creation + auth */
int createLogin(const char *username, const char *password, const char *role, int studentId); // append to LOGINS_FILE
int loginUser(const char *username, const char *password, char *outRole, int *outStudentId);   // authenticates using LOGINS_FILE

/* admission + approval */
int registerAdmission(); /* saves to ADMISSION_FILE with status "pending" */
int approveAdmissionById(int admissionTempId); /* admin: mark approved and create login (checks LOGINS only) */

/* student CRUD + marksheet */
int addStudentRecord(const Student *s);
int updateStudentRecord(int id, const Student *newData);
int deleteStudentRecord(int id);
int findStudentById(int id, Student *out);
void listAllStudents();

int addMarksheet(int studentId);
int viewMarksheetFor(int studentId);

/* menus */
void adminMenu();
void studentMenu(int studentId);

// -------------------------
// Utility helpers
// -------------------------

// Trim leading/trailing whitespace (in-place)
static void trim(char *s) {
    if (!s) return;
    // trim trailing
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r' || isspace((unsigned char)s[len-1]))) {
        s[--len] = '\0';
    }
    // trim leading
    size_t start = 0;
    while (s[start] && isspace((unsigned char)s[start])) start++;
    if (start > 0) memmove(s, s + start, len - start + 1);
}

// Clear screen cross-platform
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    // ANSI clear
    printf("\033[H\033[J");
    fflush(stdout);
#endif
}

// Enable ANSI colors on Windows console (call at program start)
void enableVirtualTerminal() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

// Pause until Enter pressed
void pauseAndClear() {
    printf("\nPress Enter to continue...");
    int c;
    // consume leftover input until newline
    while ((c = getchar()) != '\n' && c != EOF);
    // Now wait for Enter (user will press Enter)
    // If there's no further input to consume, the next getchar will block until Enter.
    // But to keep it simple we just proceed.
    clearScreen();
}

// Print current date/time in a friendly format
void showDateTime() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char buf[128];
    strftime(buf, sizeof(buf), "%d %b %Y  |  %H:%M:%S", &tm);
    printf("🕒 %s\n", buf);
}

// Read a line from stdin safely into buf (size includes null). Returns 1 on success.
int safeFgets(char *buf, size_t size) {
    if (!fgets(buf, (int)size, stdin)) return 0;
    trim(buf);
    return 1;
}

// Get integer with validation and custom prompt
int getIntInput(const char *prompt) {
    char line[128];
    long val;
    char *endptr;
    while (1) {
        if (prompt && *prompt) printf("%s", prompt);
        if (!safeFgets(line, sizeof(line))) {
            printf("❌ Input error. Try again.\n");
            continue;
        }
        if (line[0] == '\0') {
            printf("❌ Input cannot be empty. Try again.\n");
            continue;
        }
        val = strtol(line, &endptr, 10);
        if (*endptr != '\0') {
            printf("❌ Invalid input. Enter an integer value.\n");
            continue;
        }
        return (int)val;
    }
}

// Get float with validation and custom prompt
float getFloatInput(const char *prompt) {
    char line[128];
    float val;
    char *endptr;
    while (1) {
        if (prompt && *prompt) printf("%s", prompt);
        if (!safeFgets(line, sizeof(line))) {
            printf("❌ Input error. Try again.\n");
            continue;
        }
        if (line[0] == '\0') {
            printf("❌ Input cannot be empty. Try again.\n");
            continue;
        }
        val = strtof(line, &endptr);
        if (*endptr != '\0') {
            printf("❌ Invalid input. Enter a numeric value.\n");
            continue;
        }
        return val;
    }
}

// Get string input and ensure not empty (stores in outBuf of size)
void getStringInput(const char *prompt, char *outBuf, size_t size) {
    while (1) {
        if (prompt && *prompt) printf("%s", prompt);
        if (!safeFgets(outBuf, size)) {
            printf("❌ Input error. Try again.\n");
            continue;
        }
        if (outBuf[0] == '\0') {
            printf("❌ Input cannot be empty. Try again.\n");
            continue;
        }
        return;
    }
}

// Basic validator: checks a string has only letters, spaces, dots, or hyphen
int isValidName(const char *s) {
    if (!s) return 0;
    for (; *s; ++s) {
        if (!(isalpha((unsigned char)*s) || isspace((unsigned char)*s) || *s == '.' || *s == '-'))
            return 0;
    }
    return 1;
}

// Simple email check (very small): contains '@' and a dot after '@'
int isValidEmail(const char *s) {
    if (!s) return 0;
    const char *at = strchr(s, '@');
    if (!at) return 0;
    const char *dot = strchr(at, '.');
    return dot != NULL;
}

// Simple numeric string checker
int isDigitsOnly(const char *s) {
    if (!s || *s == '\0') return 0;
    for (; *s; ++s) if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

// -------------------------
// Terminal UI helpers
// -------------------------
// Print a centered, boxed title (works with typical terminal widths)
void printBoxedTitle(const char *title) {
    int width = 72; // target width for box (adjustable)
    int pad = 2;
    int tlen = (int)strlen(title);
    int inner = width - 2; // inside width
    if (tlen + pad * 2 > inner) {
        // if title too long, just print simple box
        printf("\n+-%.*s-+\n", tlen + pad*2, "----------------------------------------------------------------------");
        printf("| %s |\n", title);
        printf("+-%.*s-+\n\n", tlen + pad*2, "----------------------------------------------------------------------");
        return;
    }
    int left = (inner - tlen) / 2;
    int right = inner - tlen - left;
    // top border
    printf("\n+");
    for (int i=0;i<inner;i++) putchar('-');
    printf("+\n");
    // title line
    printf("|");
    for (int i=0;i<left;i++) putchar(' ');
    printf("%s", title);
    for (int i=0;i<right;i++) putchar(' ');
    printf("|\n");
    // bottom border
    printf("+");
    for (int i=0;i<inner;i++) putchar('-');
    printf("+\n\n");
}

// Print a centered subtitle line
void printCenteredLine(const char *line) {
    int width = 72;
    int len = (int)strlen(line);
    int left = (width - len) / 2;
    if (left < 0) left = 0;
    for (int i=0;i<left;i++) putchar(' ');
    printf("%s\n", line);
}

// Small helper to print app header (used by main)
void printAppHeader() {
    clearScreen();
    printBoxedTitle("DAFFODIL INTERNATIONAL UNIVERSITY (DIU)");
    printCenteredLine("Student Management System — Terminal Edition");
    printCenteredLine("");
    showDateTime();
    printf("\n");
}




















// =========================
// sms.c  — Part 2 of 4
// Authentication, admissions, and ID generators
// Notes:
//  - ADMISSION_FILE format:
//      tempId,name,department,semester,email,username,password,status,studentId
//    where status is "pending" or "approved" and studentId is 0 when pending.
//  - Approve operation now marks admission as approved (does NOT delete the admission file entry).
//  - Username uniqueness is enforced across both LOGINS_FILE and ADMISSION_FILE.
// =========================

/* ---------- Helper: get next IDs ---------- */

// returns next temp admission id (auto-increment), or 1001 if no file
int nextAdmissionTempId() {
    FILE *fp = fopen(ADMISSION_FILE, "r");
    if (!fp) return 1001;
    char line[MAX_LINE];
    int maxId = 1000;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        char copy[MAX_LINE];
        strcpy(copy, line);
        char *tok = strtok(copy, ",");
        if (!tok) continue;
        int id = atoi(tok);
        if (id > maxId) maxId = id;
    }
    fclose(fp);
    return maxId + 1;
}

// returns next student id based on STUDENTS_FILE, or 120 if none
int nextStudentId() {
    FILE *fp = fopen(STUDENTS_FILE, "r");
    if (!fp) return 120;
    char line[MAX_LINE];
    int maxId = 119;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        char copy[MAX_LINE];
        strcpy(copy, line);
        char *tok = strtok(copy, ",");
        if (!tok) continue;
        int id = atoi(tok);
        if (id > maxId) maxId = id;
    }
    fclose(fp);
    return maxId + 1;
}

/* ---------- Username existence checks ---------- */

// check only LOGINS_FILE (returns 1 if exists, 0 otherwise)
int usernameExistsInLogins(const char *username) {
    if (!username || username[0] == '\0') return 0;
    FILE *fp = fopen(LOGINS_FILE, "r");
    if (!fp) return 0;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        char copy[MAX_LINE];
        strcpy(copy, line);
        char *tok = strtok(copy, ","); // username
        if (tok && strcmp(tok, username) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// check ADMISSION_FILE for any entry (pending or approved) having this username
int usernameExistsInAdmissionsPending(const char *username) {
    if (!username || username[0] == '\0') return 0;
    FILE *fp = fopen(ADMISSION_FILE, "r");
    if (!fp) return 0;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        // admission format:
        // tempId,name,department,semester,email,username,password,status,studentId
        char copy[MAX_LINE];
        strcpy(copy, line);
        char *tok = strtok(copy, ","); // tempId
        int idx = 0;
        char *foundUser = NULL;
        while (tok) {
            idx++;
            // username is the 6th field per format above
            if (idx == 6) {
                foundUser = tok;
                break;
            }
            tok = strtok(NULL, ",");
        }
        if (foundUser && strcmp(foundUser, username) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// Combined check used at registration time: ensure username not in logins and not in any admission entry
int usernameExists(const char *username) {
    if (usernameExistsInLogins(username)) return 1;
    if (usernameExistsInAdmissionsPending(username)) return 1;
    return 0;
}

/* ---------- Create login (append to LOGINS_FILE) ---------- */
// returns 1 on success, 0 if username exists, -1 on error
int createLogin(const char *username, const char *password, const char *role, int studentId) {
    if (!username || !password || !role) return -1;
    if (usernameExistsInLogins(username)) return 0; // already taken in confirmed logins
    FILE *fp = fopen(LOGINS_FILE, "a");
    if (!fp) return -1;
    // store as: username,password,role,studentId\n
    fprintf(fp, "%s,%s,%s,%d\n", username, password, role, studentId);
    fclose(fp);
    return 1;
}

/* ---------- Register admission (student -> pending) ---------- */
/* returns temp admission id (>0) on success, -1 on error */
int registerAdmission() {
    clearScreen();
    printBoxedTitle("Student Admission - Register");

    Student s;
    char email[128];
    char username[MAX_USERNAME];
    char password[MAX_PASS];

    int semester = getIntInput("Enter Semester (integer): ");
    s.semester = semester;

    getStringInput("Enter Full Name: ", s.name, sizeof(s.name));
    if (!isValidName(s.name)) {
        printf("❌ Invalid name. Use letters, spaces, . or - only.\n");
        return -1;
    }

    getStringInput("Enter Department: ", s.department, sizeof(s.department));
    getStringInput("Enter Email: ", email, sizeof(email));
    if (!isValidEmail(email)) {
        printf("❌ Invalid email format.\n");
        return -1;
    }

    // username / password for future login (will be active only after approval)
    while (1) {
        getStringInput("Choose Username: ", username, sizeof(username));
        if (usernameExists(username)) {
            printf("❌ Username already taken (either in system or a pending request). Choose another.\n");
            continue;
        }
        break;
    }
    getStringInput("Choose Password: ", password, sizeof(password));

    // determine new temp id
    int tempId = nextAdmissionTempId();

    FILE *fp = fopen(ADMISSION_FILE, "a");
    if (!fp) {
        printf("❌ Unable to open %s for writing.\n", ADMISSION_FILE);
        return -1;
    }

    // admission format:
    // tempId,name,department,semester,email,username,password,status,studentId
    // status: pending (initial). studentId: 0 (initial)
    fprintf(fp, "%d,%s,%s,%d,%s,%s,%s,pending,0\n",
            tempId, s.name, s.department, s.semester, email, username, password);
    fclose(fp);

    printf("✅ Admission request saved with temporary ID: %d\n", tempId);
    printf("ℹ️  Your request is pending. After admin approval you'll be able to login.\n");
    return tempId;
}

/* ---------- Approve admission by temp id ---------- */
/* returns 1 on success, 0 if not found, -1 on error */
int approveAdmissionById(int admissionTempId) {
    FILE *fp = fopen(ADMISSION_FILE, "r");
    if (!fp) {
        printf("❌ %s not found.\n", ADMISSION_FILE);
        return -1;
    }

    FILE *tmp = fopen(TEMP_FILE, "w");
    if (!tmp) {
        fclose(fp);
        printf("❌ Error creating temp file.\n");
        return -1;
    }

    char line[MAX_LINE];
    int found = 0;
    int approvedCount = 0;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') {
            fprintf(tmp, "\n");
            continue;
        }

        char copy[MAX_LINE];
        strcpy(copy, line);

        // parse tokens:
        // tempId,name,department,semester,email,username,password,status,studentId
        char *tok = strtok(copy, ",");
        if (!tok) {
            fprintf(tmp, "%s\n", line); // preserve malformed line
            continue;
        }
        int tid = atoi(tok);

        if (tid == admissionTempId) {
            // we found the admission entry to approve
            found = 1;

            // re-parse original line tokens to preserve commas in later fields (we assume fields have no commas)
            char fields[MAX_LINE];
            strcpy(fields, line);
            char *parts[10];
            int p = 0;
            char *ptr = strtok(fields, ",");
            while (ptr && p < 10) {
                parts[p++] = ptr;
                ptr = strtok(NULL, ",");
            }
            // ensure we have at least 7 tokens (up to password). If fewer, write back unchanged.
            if (p < 7) {
                printf("❌ Malformed admission entry for tempId %d. Skipping.\n", tid);
                fprintf(tmp, "%s\n", line);
                continue;
            }
            char *name = parts[1];
            char *department = parts[2];
            char *semStr = parts[3];
            char *email = parts[4];
            char *username = parts[5];
            char *password = parts[6];
            char *status = (p >= 8) ? parts[7] : "pending";
            // studentId if present
            int linkedId = 0;
            if (p >= 9) linkedId = atoi(parts[8]);

            // if already approved, leave as-is
            if (strcmp(status, "approved") == 0 && linkedId > 0) {
                printf("ℹ️ Admission %d already approved (Student ID %d).\n", tid, linkedId);
                fprintf(tmp, "%s\n", line);
                continue;
            }

            // Before creating student, ensure username is still free in LOGINS_FILE
            if (usernameExistsInLogins(username)) {
                printf("❌ Cannot approve admission %d — username '%s' already exists in system. Ask applicant to choose a different username.\n", tid, username);
                // keep admission pending and write original line unchanged
                fprintf(tmp, "%s\n", line);
                continue;
            }

            // Create student record
            Student s;
            s.id = nextStudentId();
            strncpy(s.name, name, sizeof(s.name)-1); s.name[sizeof(s.name)-1]=0;
            strncpy(s.department, department, sizeof(s.department)-1); s.department[sizeof(s.department)-1]=0;
            s.semester = atoi(semStr);
            s.cgpa = 0.00f;

            if (addStudentRecord(&s) != 1) {
                printf("❌ Failed to create student record for admission %d. Keeping request pending.\n", tid);
                fprintf(tmp, "%s\n", line);
                continue;
            }

            // Create login linked to this student id
            int cr = createLogin(username, password, "student", s.id);
            if (cr == 1) {
                // mark admission as approved and set studentId in the admission record
                // rebuild the line with status 'approved' and studentId
                // Preserve original formatting of name/department etc.
                fprintf(tmp, "%d,%s,%s,%d,%s,%s,%s,approved,%d\n",
                        tid, s.name, s.department, s.semester, email, username, password, s.id);
                printf("✅ Admission %d approved. Assigned Student ID: %d, username: %s\n", tid, s.id, username);
                approvedCount++;
            } else if (cr == 0) {
                // username already exists in logins — rollback created student to avoid orphan
                deleteStudentRecord(s.id);
                printf("❌ Username '%s' already exists (race condition). Approval failed. Admission remains pending.\n", username);
                // write back original pending entry
                fprintf(tmp, "%s\n", line);
            } else {
                // file error — rollback student
                deleteStudentRecord(s.id);
                printf("❌ Error creating login file. Approval aborted for %d.\n", tid);
                fprintf(tmp, "%s\n", line);
            }
        } else {
            // write back other lines unchanged
            fprintf(tmp, "%s\n", line);
        }
    }

    fclose(fp);
    fclose(tmp);

    // replace admission file with updated temp
    if (remove(ADMISSION_FILE) != 0) {
        // if remove fails, still try rename (may overwrite)
    }
    if (rename(TEMP_FILE, ADMISSION_FILE) != 0) {
        printf("❌ Error updating admission file.\n");
        return -1;
    }

    if (!found) return 0;
    return (approvedCount > 0) ? 1 : 0; // 1 if at least one approval succeeded, 0 if found but not approved due to username conflict
}

/* ---------- User login (checks LOGINS_FILE) ---------- */
/* outRole must be large enough (>=16). outStudentId pointer is required.
   Returns 1 on success, 0 on failure */
int loginUser(const char *username, const char *password, char *outRole, int *outStudentId) {
    if (!username || !password || !outRole || !outStudentId) return 0;
    FILE *fp = fopen(LOGINS_FILE, "r");
    if (!fp) return 0;
    char line[MAX_LINE];
    int success = 0;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        // format: username,password,role,studentId
        char tmp[MAX_LINE];
        strcpy(tmp, line);
        char *u = strtok(tmp, ",");
        char *p = strtok(NULL, ",");
        char *r = strtok(NULL, ",");
        char *sidStr = strtok(NULL, ",");
        if (u && p && r && sidStr) {
            if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
                strncpy(outRole, r, 15);
                outRole[15] = '\0';
                *outStudentId = atoi(sidStr);
                success = 1;
                break;
            }
        }
    }
    fclose(fp);
    return success;
}







// =========================
// sms.c  — Part 3 of 4
// Student CRUD and Marksheet functions
// =========================

/* ---------- Add student record (students.txt) ---------- */
/* Returns 1 on success, 0 if duplicate id, -1 on error */
int addStudentRecord(const Student *s) {
    if (!s) return -1;

    // Check duplicate id first
    Student tmp;
    int f = findStudentById(s->id, &tmp);
    if (f == 1) return 0; // duplicate
    if (f == -1) {
        // file read error, but proceed to try append
    }

    FILE *fp = fopen(STUDENTS_FILE, "a");
    if (!fp) return -1;

    // Format: id,name,department,semester,cgpa
    fprintf(fp, "%d,%s,%s,%d,%.2f\n", s->id, s->name, s->department, s->semester, s->cgpa);
    fclose(fp);
    return 1;
}

/* ---------- Find student by id ---------- */
/* Returns 1 if found and fills out, 0 if not found, -1 on file error */
int findStudentById(int id, Student *out) {
    FILE *fp = fopen(STUDENTS_FILE, "r");
    if (!fp) return -1;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        Student s;
        // parse carefully
        char copy[MAX_LINE];
        strcpy(copy, line);
        char *tok = strtok(copy, ",");
        if (!tok) continue;
        s.id = atoi(tok);
        char *name = strtok(NULL, ",");
        char *dept = strtok(NULL, ",");
        char *semStr = strtok(NULL, ",");
        char *cgpaStr = strtok(NULL, ",");
        if (!name || !dept || !semStr || !cgpaStr) continue;
        strncpy(s.name, name, sizeof(s.name)-1); s.name[sizeof(s.name)-1]=0;
        strncpy(s.department, dept, sizeof(s.department)-1); s.department[sizeof(s.department)-1]=0;
        s.semester = atoi(semStr);
        s.cgpa = (float)atof(cgpaStr);

        if (s.id == id) {
            if (out) *out = s;
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

/* ---------- Update student record ---------- */
/* Returns 1 on success, 0 if not found, -1 on error */
int updateStudentRecord(int id, const Student *newData) {
    FILE *fp = fopen(STUDENTS_FILE, "r");
    if (!fp) return -1;
    FILE *tmp = fopen(TEMP_FILE, "w");
    if (!tmp) { fclose(fp); return -1; }

    char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        // parse id
        char copy[MAX_LINE];
        strcpy(copy, line);
        char *tok = strtok(copy, ",");
        if (!tok) continue;
        int curId = atoi(tok);
        if (curId == id) {
            // write updated data
            fprintf(tmp, "%d,%s,%s,%d,%.2f\n",
                    id,
                    newData->name,
                    newData->department,
                    newData->semester,
                    newData->cgpa);
            found = 1;
        } else {
            // write original line back
            fprintf(tmp, "%s\n", line);
        }
    }

    fclose(fp);
    fclose(tmp);

    if (!found) {
        remove(TEMP_FILE);
        return 0;
    }
    remove(STUDENTS_FILE);
    rename(TEMP_FILE, STUDENTS_FILE);
    return 1;
}

/* ---------- Delete student record ---------- */
/* Returns 1 on success, 0 if not found, -1 on error */
int deleteStudentRecord(int id) {
    FILE *fp = fopen(STUDENTS_FILE, "r");
    if (!fp) return -1;
    FILE *tmp = fopen(TEMP_FILE, "w");
    if (!tmp) { fclose(fp); return -1; }

    char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        // parse id
        char copy[MAX_LINE];
        strcpy(copy, line);
        char *tok = strtok(copy, ",");
        if (!tok) continue;
        int curId = atoi(tok);
        if (curId == id) {
            found = 1;
            // skip writing this line (delete)
        } else {
            fprintf(tmp, "%s\n", line);
        }
    }

    fclose(fp);
    fclose(tmp);

    if (!found) {
        remove(TEMP_FILE);
        return 0;
    }
    remove(STUDENTS_FILE);
    rename(TEMP_FILE, STUDENTS_FILE);

    // Also remove marksheets for this student (optional cleanup)
    FILE *mfp = fopen(MARKSHEET_FILE, "r");
    if (mfp) {
        FILE *mtemp = fopen(TEMP_FILE, "w");
        if (mtemp) {
            while (fgets(line, sizeof(line), mfp)) {
                trim(line);
                if (line[0] == '\0') continue;
                char copy2[MAX_LINE];
                strcpy(copy2, line);
                char *tok2 = strtok(copy2, ",");
                if (!tok2) continue;
                int mid = atoi(tok2);
                if (mid == id) {
                    // skip marksheet for deleted student
                    continue;
                } else {
                    fprintf(mtemp, "%s\n", line);
                }
            }
            fclose(mtemp);
            remove(MARKSHEET_FILE);
            rename(TEMP_FILE, MARKSHEET_FILE);
        }
        fclose(mfp);
    }

    // Also remove login entries linked to this studentId (LOGINS_FILE format: username,password,role,studentId)
    FILE *lfp = fopen(LOGINS_FILE, "r");
    if (lfp) {
        FILE *ltmp = fopen(TEMP_FILE, "w");
        if (ltmp) {
            while (fgets(line, sizeof(line), lfp)) {
                trim(line);
                if (line[0] == '\0') continue;
                char copy3[MAX_LINE];
                strcpy(copy3, line);
                char *toku = strtok(copy3, ",");
                char *tokp = strtok(NULL, ",");
                char *tokr = strtok(NULL, ",");
                char *toksid = strtok(NULL, ",");
                if (!toksid) {
                    // malformed - keep it to avoid accidental deletion
                    fprintf(ltmp, "%s\n", line);
                    continue;
                }
                int lid = atoi(toksid);
                if (lid == id) {
                    // skip this login (delete)
                    continue;
                } else {
                    fprintf(ltmp, "%s\n", line);
                }
            }
            fclose(ltmp);
            remove(LOGINS_FILE);
            rename(TEMP_FILE, LOGINS_FILE);
        }
        fclose(lfp);
    }

    return 1;
}

/* ---------- List all students ---------- */
void listAllStudents() {
    FILE *fp = fopen(STUDENTS_FILE, "r");
    if (!fp) {
        printf("❌ No student records found!\n");
        return;
    }
    printf("\n===== All Student Records =====\n");
    printf("%-6s  %-25s  %-15s  %-8s  %-6s\n", "ID", "Name", "Department", "Semester", "CGPA");
    printf("----------------------------------------------------------------------\n");
    char line[MAX_LINE];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        Student s;
        char copy[MAX_LINE];
        strcpy(copy, line);
        char *tok = strtok(copy, ",");
        if (!tok) continue;
        s.id = atoi(tok);
        char *name = strtok(NULL, ",");
        char *dept = strtok(NULL, ",");
        char *semStr = strtok(NULL, ",");
        char *cgpaStr = strtok(NULL, ",");
        if (!name || !dept || !semStr || !cgpaStr) continue;
        strncpy(s.name, name, sizeof(s.name)-1); s.name[sizeof(s.name)-1]=0;
        strncpy(s.department, dept, sizeof(s.department)-1); s.department[sizeof(s.department)-1]=0;
        s.semester = atoi(semStr);
        s.cgpa = (float)atof(cgpaStr);

        printf("%-6d  %-25s  %-15s  %-8d  %-6.2f\n", s.id, s.name, s.department, s.semester, s.cgpa);
        count++;
    }
    if (count == 0) printf("❌ No student records to display!\n");
    fclose(fp);
}

/* ---------- Add marksheet for a student ---------- */
/* Format per line: id,semesterLabel,subject,score,grade,subject,score,grade,... */
/* Returns 1 on success, 0 if student not found, -1 on file error */
int addMarksheet(int studentId) {
    // verify student exists
    Student s;
    int found = findStudentById(studentId, &s);
    if (found != 1) return 0;

    FILE *fp = fopen(MARKSHEET_FILE, "a");
    if (!fp) return -1;

    char semesterLabel[80];
    getStringInput("Enter Semester label (e.g. Spring2025): ", semesterLabel, sizeof(semesterLabel));

    // start line with id and semester
    fprintf(fp, "%d,%s", studentId, semesterLabel);

    while (1) {
        printf("\n1. Add Subject\n2. Done\n");
        int ch = getIntInput("Enter choice: ");
        if (ch == 2) break;
        if (ch != 1) {
            printf("❌ Invalid choice. Try again.\n");
            continue;
        }
        char subject[MAX_SUBJECT];
        char grade[8];
        float score;
        getStringInput("Enter Subject Name: ", subject, sizeof(subject));
        score = getFloatInput("Enter Subject CGPA (numeric): ");
        getStringInput("Enter Grade (A/B/C/D/F): ", grade, sizeof(grade));

        // append triplet
        fprintf(fp, ",%s,%.2f,%s", subject, score, grade);
    }

    fprintf(fp, "\n");
    fclose(fp);
    return 1;
}

/* ---------- View marksheet(s) for a student ---------- */
/* If studentId <=0, interactive prompt is used; otherwise prints all marksheets for that id */
/* Returns number of marksheets found (>0) or 0 if none, -1 on file error */
int viewMarksheetFor(int studentId) {
    if (studentId <= 0) {
        studentId = getIntInput("Enter Student ID to view marksheet: ");
    }

    FILE *fp = fopen(MARKSHEET_FILE, "r");
    if (!fp) return -1;

    char line[MAX_LINE];
    int foundAny = 0;

    // To display student basic info:
    Student s;
    int stFound = findStudentById(studentId, &s);

    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        // make a copy for strtok
        char copy[MAX_LINE];
        strcpy(copy, line);
        char *tok = strtok(copy, ",");
        if (!tok) continue;
        int id = atoi(tok);
        if (id != studentId) continue;

        foundAny = 1;
        // next token is semester label
        char *semester = strtok(NULL, ",");
        if (!semester) semester = "Unknown";

        // print nice header (A4-like)
        printf("\n****************************************************\n");
        printf("            OFFICIAL MARKSHEET REPORT\n");
        printf("      DAFFODIL INTERNATIONAL UNIVERSITY\n");
        printf("****************************************************\n");
        if (stFound == 1) {
            printf("Student ID   : %d\n", s.id);
            printf("Student Name : %s\n", s.name);
            printf("Department   : %s\n", s.department);
        } else {
            printf("Student ID   : %d\n", id);
            printf("Student Name : (Not found in students.txt)\n");
        }
        printf("Semester     : %s\n", semester);
        printf("----------------------------------------------------\n");
        printf("%-4s  %-30s  %-6s  %-6s\n", "No.", "Subject", "CGPA", "Grade");
        printf("----------------------------------------------------\n");

        int count = 0;
        float total = 0.0f;
        while (1) {
            char *sub = strtok(NULL, ",");
            if (!sub) break;
            char *scoreStr = strtok(NULL, ",");
            char *grade = strtok(NULL, ",");
            if (!scoreStr || !grade) break;
            float score = (float)atof(scoreStr);
            printf("%-4d  %-30s  %-6.2f  %-6s\n", ++count, sub, score, grade);
            total += score;
        }
        float avg = (count > 0) ? (total / (float)count) : 0.0f;
        printf("----------------------------------------------------\n");
        printf(" Semester Average CGPA: %.2f\n", avg);
        printf("****************************************************\n");
        // continue to display other marksheets (if multiple)
    }

    fclose(fp);
    if (!foundAny) {
        printf("❌ No marksheet found for Student ID %d.\n", studentId);
        return 0;
    }
    return 1;
}





// =========================
// sms.c  — Part 4 of 4
// UI, menus, main(), and interactive admin helpers
// Improvements:
//  - Admission entries keep status (pending/approved) — not deleted.
//  - Approve flow gives clearer messages (not found / already approved / username conflict).
//  - Username uniqueness enforced when user registers and when admin creates a login.
// =========================

/* ---------- List pending (and approved) admission requests ---------- */
void listPendingAdmissions() {
    FILE *fp = fopen(ADMISSION_FILE, "r");
    if (!fp) {
        printf("ℹ️  No admission requests found.\n");
        return;
    }

    printf("\n===== Admission Requests =====\n");
    printf("%-8s  %-30s  %-15s  %-6s  %-25s  %-15s  %-8s  %-8s\n",
           "TempID", "Name", "Department", "Sem", "Email", "Username", "Status", "StudID");
    printf("---------------------------------------------------------------------------------------------------------------\n");

    char line[MAX_LINE];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        // tempId,name,department,semester,email,username,password,status,studentId
        char copy[MAX_LINE];
        strcpy(copy, line);

        // Safe tokenization (fields are assumed not to contain commas)
        char *tok = strtok(copy, ",");
        if (!tok) continue;
        char *tempId = tok;
        char *name = strtok(NULL, ",");
        char *dept = strtok(NULL, ",");
        char *sem = strtok(NULL, ",");
        char *email = strtok(NULL, ",");
        char *username = strtok(NULL, ",");
        char *password = strtok(NULL, ","); // not printed
        char *status = strtok(NULL, ",");
        char *studId = strtok(NULL, ",");

        if (!name || !dept || !sem || !email || !username) continue;
        if (!status) status = "pending";
        if (!studId) studId = "0";

        printf("%-8s  %-30s  %-15s  %-6s  %-25s  %-15s  %-8s  %-8s\n",
               tempId, name, dept, sem, email, username, status, studId);
        count++;
    }

    if (count == 0) printf("ℹ️  No admission requests to display.\n");
    fclose(fp);
}

/* ---------- Admin interactive helpers ---------- */
void adminAddStudentInteractive() {
    clearScreen();
    printBoxedTitle("Admin: Add Student Manually");

    Student s;
    s.id = nextStudentId();
    getStringInput("Enter Full Name: ", s.name, sizeof(s.name));
    getStringInput("Enter Department: ", s.department, sizeof(s.department));
    s.semester = getIntInput("Enter Semester (int): ");
    s.cgpa = getFloatInput("Enter CGPA (numeric): ");
    int r = addStudentRecord(&s);
    if (r == 1) {
        printf("✅ Student added with ID %d\n", s.id);

        // prompt to create login now
        char choice[8];
        printf("Create login for this student now? (y/n): ");
        safeFgets(choice, sizeof(choice));
        if (choice[0] == 'y' || choice[0] == 'Y') {
            char username[MAX_USERNAME];
            char password[MAX_PASS];
            while (1) {
                getStringInput("Choose Username: ", username, sizeof(username));
                // use global check so admin cannot pick username already used by a pending request
                if (usernameExists(username)) {
                    printf("❌ Username already taken (existing login or pending request). Choose another.\n");
                    continue;
                }
                break;
            }
            getStringInput("Choose Password: ", password, sizeof(password));
            int cr = createLogin(username, password, "student", s.id);
            if (cr == 1) {
                printf("✅ Login created for student (username: %s).\n", username);
            } else if (cr == 0) {
                // this should not happen because we checked usernameExists, but handle gracefully
                printf("❌ Username already exists. Login not created.\n");
            } else {
                // file error: rollback student creation to avoid orphan record
                deleteStudentRecord(s.id);
                printf("❌ Error creating login. Student record rolled back.\n");
            }
        } else {
            printf("ℹ️  You can create login later or approve an existing pending admission.\n");
        }
    } else if (r == 0) {
        printf("❌ Duplicate ID. Student not added.\n");
    } else {
        printf("❌ Error adding student.\n");
    }
}

void adminViewStudentInteractive() {
    clearScreen();
    printBoxedTitle("Admin: View Student By ID");
    int id = getIntInput("Enter Student ID: ");
    Student s;
    int r = findStudentById(id, &s);
    if (r == 1) {
        printf("\n--- Student Details ---\n");
        printf("ID        : %d\n", s.id);
        printf("Name      : %s\n", s.name);
        printf("Department: %s\n", s.department);
        printf("Semester  : %d\n", s.semester);
        printf("CGPA      : %.2f\n", s.cgpa);
    } else if (r == 0) {
        printf("❌ Student ID %d not found.\n", id);
    } else {
        printf("❌ Error reading students file.\n");
    }
}

void adminUpdateStudentInteractive() {
    clearScreen();
    printBoxedTitle("Admin: Update Student By ID");
    int id = getIntInput("Enter Student ID to update: ");
    Student s;
    int r = findStudentById(id, &s);
    if (r != 1) {
        printf("❌ Student ID %d not found.\n", id);
        return;
    }
    printf("Enter new details (existing values shown in brackets).\n");

    char buf[MAX_NAME];
    printf("Name [%s]: ", s.name);
    if (safeFgets(buf, sizeof(buf)) && buf[0] != '\0') strncpy(s.name, buf, sizeof(s.name)-1);

    printf("Department [%s]: ", s.department);
    if (safeFgets(buf, sizeof(buf)) && buf[0] != '\0') strncpy(s.department, buf, sizeof(s.department)-1);

    char tmp[64];
    printf("Semester [%d]: ", s.semester);
    if (safeFgets(tmp, sizeof(tmp)) && tmp[0] != '\0') s.semester = atoi(tmp);

    printf("CGPA [%.2f]: ", s.cgpa);
    if (safeFgets(tmp, sizeof(tmp)) && tmp[0] != '\0') s.cgpa = (float)atof(tmp);

    int upr = updateStudentRecord(id, &s);
    if (upr == 1) printf("✅ Student ID %d updated.\n", id);
    else if (upr == 0) printf("❌ Student ID %d not found.\n");
    else printf("❌ Error updating record.\n");
}

void adminDeleteStudentInteractive() {
    clearScreen();
    printBoxedTitle("Admin: Delete Student");
    int id = getIntInput("Enter Student ID to delete: ");
    int r = deleteStudentRecord(id);
    if (r == 1) printf("✅ Student ID %d deleted.\n", id);
    else if (r == 0) printf("❌ Student ID %d not found.\n", id);
    else printf("❌ Error deleting student.\n");
}

void adminApproveAdmissionInteractive() {
    clearScreen();
    printBoxedTitle("Admin: Approve Admission");
    listPendingAdmissions();
    int tempId = getIntInput("\nEnter Temp Admission ID to approve (or 0 to cancel): ");
    if (tempId == 0) {
        printf("Cancelled.\n");
        return;
    }

    // First, check if this tempId exists and its current status
    FILE *fp = fopen(ADMISSION_FILE, "r");
    if (!fp) {
        printf("❌ Unable to read admissions file.\n");
        return;
    }
    char line[MAX_LINE];
    int found = 0;
    char curStatus[MAX_STATUS] = {0};
    char pendingUsername[MAX_USERNAME] = {0};
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0') continue;
        char copy[MAX_LINE];
        strcpy(copy, line);
        char *tok = strtok(copy, ",");
        if (!tok) continue;
        int tid = atoi(tok);
        if (tid == tempId) {
            found = 1;
            // parse to find status and username
            // tempId,name,department,semester,email,username,password,status,studentId
            char *parts[10];
            int p = 0;
            char *ptr = strtok(line, ",");
            while (ptr && p < 10) {
                parts[p++] = ptr;
                ptr = strtok(NULL, ",");
            }
            if (p >= 7) {
                strncpy(pendingUsername, parts[5], sizeof(pendingUsername)-1);
            }
            if (p >= 8) {
                strncpy(curStatus, parts[7], sizeof(curStatus)-1);
            } else {
                strcpy(curStatus, "pending");
            }
            break;
        }
    }
    fclose(fp);

    if (!found) {
        printf("❌ Admission ID %d not found.\n", tempId);
        return;
    }
    if (strcmp(curStatus, "approved") == 0) {
        printf("ℹ️ Admission ID %d is already approved.\n", tempId);
        return;
    }

    // Try approve
    int r = approveAdmissionById(tempId);
    if (r == 1) {
        printf("✅ Approval completed.\n");
    } else if (r == 0) {
        // Could be username conflict (createLogin failed) or not approved
        // Check if username is already in logins to give clear message
        if (usernameExistsInLogins(pendingUsername)) {
            printf("❌ Cannot approve: username '%s' already exists in system. Ask applicant to choose a different username.\n", pendingUsername);
        } else {
            printf("❌ Approval failed or nothing changed. Check the admission entry.\n");
        }
    } else {
        printf("❌ Error processing approval.\n");
    }
}

/* ---------- Admin Menu ---------- */
void adminMenu() {
    while (1) {
        clearScreen();
        printBoxedTitle("ADMIN PANEL");
        showDateTime();
        printf("\n1. View All Admissions\n");
        printf("2. Approve Admission\n");
        printf("3. Add Student (manual)\n");
        printf("4. View Student by ID\n");
        printf("5. Update Student\n");
        printf("6. Delete Student\n");
        printf("7. View All Students\n");
        printf("8. Add Marksheet\n");
        printf("9. View Marksheet\n");
        printf("10. Logout\n");

        int ch = getIntInput("Enter choice: ");
        switch (ch) {
            case 1: listPendingAdmissions(); break;
            case 2: adminApproveAdmissionInteractive(); break;
            case 3: adminAddStudentInteractive(); break;
            case 4: adminViewStudentInteractive(); break;
            case 5: adminUpdateStudentInteractive(); break;
            case 6: adminDeleteStudentInteractive(); break;
            case 7: listAllStudents(); break;
            case 8: {
                int id = getIntInput("Enter Student ID to add marksheet: ");
                int r = addMarksheet(id);
                if (r == 1) printf("✅ Marksheet added.\n");
                else if (r == 0) printf("❌ Student not found.\n");
                else printf("❌ Error saving marksheet.\n");
                break;
            }
            case 9:
                viewMarksheetFor(0); // interactive prompt inside
                break;
            case 10:
                printf("🔒 Logging out of admin panel.\n");
                pauseAndClear();
                return;
            default:
                printf("❌ Invalid choice. Try again.\n");
        }
        pauseAndClear();
    }
}

/* ---------- Student Menu ---------- */
void studentMenu(int studentId) {
    while (1) {
        clearScreen();
        printBoxedTitle("STUDENT PORTAL");
        showDateTime();

        printf("\n1. View My Details\n");
        printf("2. Update My Details\n");
        printf("3. View My Marksheet(s)\n");
        printf("4. Logout\n");

        int ch = getIntInput("Enter choice: ");
        if (ch == 1) {
            Student s;
            int r = findStudentById(studentId, &s);
            if (r == 1) {
                printf("\n--- Your Details ---\n");
                printf("ID        : %d\n", s.id);
                printf("Name      : %s\n", s.name);
                printf("Department: %s\n", s.department);
                printf("Semester  : %d\n", s.semester);
                printf("CGPA      : %.2f\n", s.cgpa);
            } else {
                printf("❌ Your student record not found.\n");
            }
        } else if (ch == 2) {
            Student s;
            int r = findStudentById(studentId, &s);
            if (r != 1) {
                printf("❌ Your student record not found.\n");
            } else {
                printf("Enter new details (leave blank to keep current):\n");
                char buf[MAX_NAME];
                printf("Name [%s]: ", s.name);
                if (safeFgets(buf, sizeof(buf)) && buf[0] != '\0') strncpy(s.name, buf, sizeof(s.name)-1);

                printf("Department [%s]: ", s.department);
                if (safeFgets(buf, sizeof(buf)) && buf[0] != '\0') strncpy(s.department, buf, sizeof(s.department)-1);

                char tmp[64];
                printf("Semester [%d]: ", s.semester);
                if (safeFgets(tmp, sizeof(tmp)) && tmp[0] != '\0') s.semester = atoi(tmp);

                // printf("CGPA [%.2f]: ", s.cgpa);
                if (safeFgets(tmp, sizeof(tmp)) && tmp[0] != '\0') s.cgpa = (float)atof(tmp);

                int upr = updateStudentRecord(studentId, &s);
                if (upr == 1) printf("✅ Your details updated.\n");
                else printf("❌ Failed to update your details.\n");
            }
        }
        
        
        // else if (ch == 3) {
        //     viewMarksheetFor(studentId);
        // } 

        else if (ch == 3) {
    int vm = viewMarksheetFor(studentId);
    if (vm == 1) {
        // marksheet(s) shown by the function
    } else if (vm == 0) {
        // no marksheet found
        printf("\nℹ️  Marksheet not available for you yet. Please contact admin if you expect a marksheet.\n");
    } else {
        // file error
        printf("\n❌ Error reading marksheet data. Please try again later or contact admin.\n");
    }
}

        
        
        
        
        
        
        else if (ch == 4) {
            printf("🔒 Logging out.\n");
            pauseAndClear();
            return;
        } else {
            printf("❌ Invalid choice. Try again.\n");
        }
        pauseAndClear();
    }
}

/* ---------- Main program flow ---------- */
int main() {
    enableVirtualTerminal(); // enable colors on Windows if possible
    printAppHeader();

    while (1) {
        printf("\nMain Menu:\n");
        printf("1. Register (Admission Request)\n");
        printf("2. Student Login\n");
        printf("3. Admin Login\n");
        printf("4. Exit\n");

        int choice = getIntInput("Enter choice: ");
        if (choice == 1) {
            registerAdmission();
            pauseAndClear();
            printAppHeader();
        } else if (choice == 2) {
            clearScreen();
            printBoxedTitle("Student Login");
            char username[MAX_USERNAME], password[MAX_PASS];
            getStringInput("Username: ", username, sizeof(username));
            getStringInput("Password: ", password, sizeof(password));
            char role[16];
            int sid = 0;
            if (loginUser(username, password, role, &sid)) {
                if (strcmp(role, "student") == 0) {
                    if (sid <= 0) {
                        printf("❌ Your account is not linked to a student record. Contact admin.\n");
                        pauseAndClear();
                        printAppHeader();
                        continue;
                    }
                    Student s;
                    int r = findStudentById(sid, &s);
                    if (r == 1) {
                        printf("✅ Login successful. Welcome, %s!\n", s.name);
                        pauseAndClear();
                        studentMenu(sid);
                    } else {
                        printf("❌ Student record linked to this login not found. Contact admin.\n");
                        pauseAndClear();
                    }
                } else {
                    printf("❌ You are not a student. Use admin login if you manage the system.\n");
                    pauseAndClear();
                }
            } else {
                // Provide helpful hint: if username exists as pending admission, tell user pending approval
                // We'll check ADMISSION_FILE for a matching username with status 'pending'
                int pendingFound = 0;
                FILE *afp = fopen(ADMISSION_FILE, "r");
                if (afp) {
                    char aline[MAX_LINE];
                    while (fgets(aline, sizeof(aline), afp)) {
                        trim(aline);
                        if (aline[0] == '\0') continue;
                        char copy[MAX_LINE];
                        strcpy(copy, aline);
                        char *tok = strtok(copy, ",");
                        if (!tok) continue;
                        // parse fields to get username and status
                        char *parts[10];
                        int p = 0;
                        char *ptr = strtok(aline, ",");
                        while (ptr && p < 10) {
                            parts[p++] = ptr;
                            ptr = strtok(NULL, ",");
                        }
                        if (p >= 6) {
                            char *adUser = parts[5];
                            char *adStatus = (p >= 8) ? parts[7] : "pending";
                            if (strcmp(adUser, username) == 0) {
                                if (strcmp(adStatus, "pending") == 0) pendingFound = 1;
                                break;
                            }
                        }
                    }
                    fclose(afp);
                }
                if (pendingFound) {
                    printf("❌ Login failed: your registration is still pending admin approval.\n");
                } else {
                    printf("❌ Login failed. Check your credentials or account approval status.\n");
                }
                pauseAndClear();
            }
            printAppHeader();
        } else if (choice == 3) {
            clearScreen();
            printBoxedTitle("Admin Login");
            // Allow built-in admin password or login via logins.txt with role "admin"
            char pwd[80];
            getStringInput("Enter Admin Master Password (or press Enter to use username login): ", pwd, sizeof(pwd));
            if (strcmp(pwd, "admin123") == 0) {
                printf("🔐 Admin access granted (master password).\n");
                pauseAndClear();
                adminMenu();
                printAppHeader();
            } else {
                char username[MAX_USERNAME], password[MAX_PASS];
                char role[16];
                int sid = 0;
                getStringInput("Admin Username: ", username, sizeof(username));
                getStringInput("Password: ", password, sizeof(password));
                if (loginUser(username, password, role, &sid) && strcmp(role, "admin") == 0) {
                    printf("🔐 Admin login successful.\n");
                    pauseAndClear();
                    adminMenu();
                    printAppHeader();
                } else {
                    printf("❌ Admin login failed.\n");
                    pauseAndClear();
                    printAppHeader();
                }
            }
        } else if (choice == 4) {
            printf("👋 Exiting... Goodbye!\n");
            break;
        } else {
            printf("❌ Invalid choice. Try again.\n");
            pauseAndClear();
            printAppHeader();
        }
    }

    return 0;
}
