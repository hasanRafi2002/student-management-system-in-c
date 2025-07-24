#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ==== Constants ====
#define SUBJECT_COUNT 5

// ==== Student Structure ====
struct Student {
    int id;
    char name[50];
    char department[50];
    int semester;
    float cgpa;
};

// ==== Function Prototypes ====

void clearScreen();
void pauseAndClear();
void showDateTime();
int getIntInput(char[]);
float getFloatInput(char[]);

void adminMenu();
void studentMenu();

void addStudent();
void viewStudent();
void updateStudent();
void deleteStudent();
void viewAllStudents();
void viewOwnDetails(int);

void addMarksheet();
void viewMarksheet();

// ==== Utility Functions ====

void clearScreen() {
    printf("\033[H\033[J");  // ANSI clear screen
}

void pauseAndClear() {
    printf("\nPress Enter to continue...");
    getchar(); getchar();   // Wait for user input
    clearScreen();          // Then clear
}

void showDateTime() {
    time_t t;
    time(&t);
    printf("🕒 %s\n", ctime(&t));
}

int getIntInput(char prompt[]) {
    int value;
    char term;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d%c", &value, &term) != 2 || term != '\n') {
            printf("❌ Invalid input. Enter a number only.\n");
            while (getchar() != '\n');
        } else {
            return value;
        }
    }
}

float getFloatInput(char prompt[]) {
    float value;
    char term;
    while (1) {
        printf("%s", prompt);
        if (scanf("%f%c", &value, &term) != 2 || term != '\n') {
            printf("❌ Invalid input. Enter a numeric value.\n");
            while (getchar() != '\n');
        } else {
            return value;
        }
    }
}

// ==== Main Function ====

int main() {




    int choice;
    char password[20];
    int studentId;

    while (1) {

        clearScreen();
                    printf("\t\t\t -------------------------------------------------------------------------\n");
    printf("\t\t\t|                                                                         |\n");
    printf("\t\t\t|                                                                         |\n");
    printf("\t\t\t|       W      W  WW W W  W       WW W W  WW W WW  WW    WW  WW W W       |\n");
    printf("\t\t\t|       W      W  W       W       W       W     W  W W  W W  W            |\n");
    printf("\t\t\t|       W  WW  W  WW W W  W       W       W     W  W  WW  W  WW W W       |\n");
    printf("\t\t\t|       W W  W W  W       W       W       W     W  W      W  W            |\n");
    printf("\t\t\t|       WW    WW  WW W W  WW W W  WW W W  WW W WW  W      W  WW W W       |\n");
    printf("\t\t\t|                                                                         |\n");
    printf("\t\t\t|                                                                         |\n");
    printf("\t\t\t -------------------------------------------------------------------------\n");
        printf("===== Student Information Management System =====\n");
        showDateTime();

        printf("\n1. Admin Login\n");
        printf("2. Student Login\n");
        printf("3. Exit\n");

        choice = getIntInput("Enter your choice: ");

        switch (choice) {
            case 1:
                printf("Enter Admin Password: ");
                scanf(" %s", password);
                if (strcmp(password, "admin123") == 0) {
                    adminMenu();
                } else {
                    printf("❌ Incorrect Password!\n");
                    pauseAndClear();
                }
                break;

            case 2:
                studentId = getIntInput("Enter your Student ID: ");
                viewOwnDetails(studentId);
                pauseAndClear();
                break;

            case 3:
                clearScreen();
                printf("👋 Exiting system... Goodbye!\n");
                exit(0);

            default:
                printf("❌ Invalid choice! Try again.\n");
                pauseAndClear();
        }
    }

    return 0;
}


// ==== Admin Menu ====
void adminMenu() {
    int choice;

    do {
        clearScreen();
        printf("===== Admin Menu =====\n");
        showDateTime();

        printf("\n1. Add Student\n");
        printf("2. View Student by ID\n");
        printf("3. Update Student\n");
        printf("4. Delete Student\n");
        printf("5. View All Students\n");
        printf("6. Add Marksheet\n");
        printf("7. View Marksheet\n");
        printf("8. Logout\n");

        choice = getIntInput("Enter your choice: ");

        switch (choice) {
            case 1: addStudent(); break;
            case 2: viewStudent(); break;
            case 3: updateStudent(); break;
            case 4: deleteStudent(); break;
            case 5: viewAllStudents(); break;
            case 6: addMarksheet(); break;
            case 7: viewMarksheet(); break;
            case 8: printf("🔒 Logged out from Admin Panel.\n"); break;
            default: printf("❌ Invalid choice! Try again.\n");
        }

        if (choice != 8) pauseAndClear();

    } while (choice != 8);
}

// ==== Add Student ====
void addStudent() {
    FILE *fp;
    struct Student s;

    fp = fopen("students.txt", "a");
    if (fp == NULL) {
        printf("❌ Error opening file!\n");
        return;
    }

    s.id = getIntInput("Enter Student ID: ");
    printf("Enter Name: ");
    scanf(" %[^\n]", s.name);
    printf("Enter Department: ");
    scanf(" %[^\n]", s.department);
    s.semester = getIntInput("Enter Semester: ");
    s.cgpa = getFloatInput("Enter CGPA: ");

    fprintf(fp, "%d,%s,%s,%d,%.2f\n", s.id, s.name, s.department, s.semester, s.cgpa);
    fclose(fp);

    printf("✅ Student added successfully!\n");
}

// ==== View Student ====
void viewStudent() {
    FILE *fp;
    struct Student s;
    int searchId;
    int found = 0;

    fp = fopen("students.txt", "r");
    if (fp == NULL) {
        printf("❌ No student records found!\n");
        return;
    }

    searchId = getIntInput("Enter Student ID to view: ");

    while (fscanf(fp, "%d,%[^,],%[^,],%d,%f\n", &s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        if (s.id == searchId) {
            printf("\n--- Student Details ---\n");
            printf("ID        : %d\n", s.id);
            printf("Name      : %s\n", s.name);
            printf("Department: %s\n", s.department);
            printf("Semester  : %d\n", s.semester);
            printf("CGPA      : %.2f\n", s.cgpa);
            found = 1;
            break;
        }
    }

    if (!found)
        printf("❌ Student not found!\n");

    fclose(fp);
}

// ==== Update Student ====
void updateStudent() {
    FILE *fp, *tempFp;
    struct Student s;
    int searchId;
    int found = 0;

    fp = fopen("students.txt", "r");
    if (fp == NULL) {
        printf("❌ No student records found!\n");
        return;
    }

    tempFp = fopen("temp.txt", "w");
    if (tempFp == NULL) {
        printf("❌ Error creating temporary file!\n");
        fclose(fp);
        return;
    }

    searchId = getIntInput("Enter Student ID to update: ");

    while (fscanf(fp, "%d,%[^,],%[^,],%d,%f\n", &s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        if (s.id == searchId) {
            printf("Enter new details for student ID %d:\n", searchId);
            printf("Name: ");
            scanf(" %[^\n]", s.name);
            printf("Department: ");
            scanf(" %[^\n]", s.department);
            s.semester = getIntInput("Enter Semester: ");
            s.cgpa = getFloatInput("Enter CGPA: ");
            found = 1;
        }
        fprintf(tempFp, "%d,%s,%s,%d,%.2f\n", s.id, s.name, s.department, s.semester, s.cgpa);
    }

    fclose(fp);
    fclose(tempFp);

    if (found) {
        remove("students.txt");
        rename("temp.txt", "students.txt");
        printf("✅ Student ID %d updated successfully!\n", searchId);
    } else {
        remove("temp.txt");
        printf("❌ Student ID %d not found!\n", searchId);
    }
}

// ==== Delete Student ====
void deleteStudent() {
    FILE *fp, *tempFp;
    struct Student s;
    int searchId;
    int found = 0;

    fp = fopen("students.txt", "r");
    if (fp == NULL) {
        printf("❌ No student records found!\n");
        return;
    }

    tempFp = fopen("temp.txt", "w");
    if (tempFp == NULL) {
        printf("❌ Error creating temporary file!\n");
        fclose(fp);
        return;
    }

    searchId = getIntInput("Enter Student ID to delete: ");

    while (fscanf(fp, "%d,%[^,],%[^,],%d,%f\n", &s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        if (s.id == searchId) {
            found = 1;
            continue;
        }
        fprintf(tempFp, "%d,%s,%s,%d,%.2f\n", s.id, s.name, s.department, s.semester, s.cgpa);
    }

    fclose(fp);
    fclose(tempFp);

    if (found) {
        remove("students.txt");
        rename("temp.txt", "students.txt");
        printf("✅ Student ID %d deleted successfully!\n", searchId);
    } else {
        remove("temp.txt");
        printf("❌ Student ID %d not found!\n", searchId);
    }
}

// ==== View All Students ====
void viewAllStudents() {
    FILE *fp;
    struct Student s;
    int count = 0;

    fp = fopen("students.txt", "r");
    if (fp == NULL) {
        printf("❌ No student records found!\n");
        return;
    }

    printf("\n===== All Student Records =====\n");
    printf("%-10s %-20s %-15s %-10s %-5s\n", "ID", "Name", "Department", "Semester", "CGPA");

    while (fscanf(fp, "%d,%[^,],%[^,],%d,%f\n", &s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        printf("%-10d %-20s %-15s %-10d %.2f\n", s.id, s.name, s.department, s.semester, s.cgpa);
        count++;
    }

    if (count == 0)
        printf("❌ No student records to display!\n");

    fclose(fp);
}


// ==== Add Marksheet ====
void addMarksheet() {
    int id, found = 0;
    struct Student s;
    FILE *fp = fopen("students.txt", "r");

    if (!fp) {
        printf("❌ students.txt not found!\n");
        return;
    }

    id = getIntInput("Enter Student ID to add marksheet: ");

    // Validate ID exists
    while (fscanf(fp, "%d,%[^,],%[^,],%d,%f\n",
                  &s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        if (s.id == id) {
            found = 1;
            break;
        }
    }
    fclose(fp);

    if (!found) {
        printf("❌ Student ID %d not found. Cannot add marksheet.\n", id);
        return;
    }

    FILE *mfp = fopen("marksheets.txt", "a");
    if (!mfp) {
        printf("❌ Error opening marksheets.txt\n");
        return;
    }

    fprintf(mfp, "%d", id);  // Write student ID first

    char subject[50], grade[5];
    float subjectCGPA;
    while (1) {
        int choice = getIntInput("\n1. Add Subject\n2. Done\nEnter choice: ");
        if (choice == 2) break;

        printf("Enter Subject Name: ");
        scanf(" %[^\n]", subject);
        subjectCGPA = getFloatInput("Enter Subject CGPA: ");
        printf("Enter Grade (A/B/C/D/F): ");
        scanf(" %[^\n]", grade);

        fprintf(mfp, ",%s,%.2f,%s", subject, subjectCGPA, grade);
    }

    fprintf(mfp, "\n");
    fclose(mfp);
    printf("✅ Marksheet saved successfully for ID %d!\n", id);
}

// ==== View Marksheet ====
void viewMarksheet() {
    int id;
    FILE *fp = fopen("marksheets.txt", "r");

    if (!fp) {
        printf("❌ marksheets.txt not found!\n");
        return;
    }

    id = getIntInput("Enter Student ID to view marksheet: ");

    char line[1000];
    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *token = strtok(line, ",");
        if (token && atoi(token) == id) {
            printf("\n📄 Marksheet for Student ID: %d\n", id);
            int count = 1;
            while ((token = strtok(NULL, ",")) != NULL) {
                char *subject = token;
                char *cgpaStr = strtok(NULL, ",");
                char *grade = strtok(NULL, ",");

                if (subject && cgpaStr && grade)
                    printf("%d. %s - CGPA: %s - Grade: %s\n", count++, subject, cgpaStr, grade);
            }
            found = 1;
            break;
        }
    }

    if (!found)
        printf("❌ No marksheet found for Student ID %d.\n", id);

    fclose(fp);
}


// ==== View Own Student Details ====
void viewOwnDetails(int studentId) {
    FILE *fp;
    struct Student s;
    int found = 0;

    fp = fopen("students.txt", "r");
    if (fp == NULL) {
        printf("❌ No student records found!\n");
        return;
    }

    while (fscanf(fp, "%d,%[^,],%[^,],%d,%f\n", 
                  &s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        if (s.id == studentId) {
            printf("\n--- Your Student Details ---\n");
            printf("ID        : %d\n", s.id);
            printf("Name      : %s\n", s.name);
            printf("Department: %s\n", s.department);
            printf("Semester  : %d\n", s.semester);
            printf("CGPA      : %.2f\n", s.cgpa);
            found = 1;
            break;
        }
    }

    fclose(fp);

    if (!found) {
        printf("❌ No student found with ID %d\n", studentId);
    } else {
        // Optional: show marksheet too
        char choice;
        printf("\nDo you want to view your marksheet as well? (y/n): ");
        scanf(" %c", &choice);
        if (choice == 'y' || choice == 'Y') {
            viewMarksheet(); // Reuse existing function
        }
    }
}




