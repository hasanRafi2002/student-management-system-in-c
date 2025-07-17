#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define Student Structure
struct Student {
    char id[20];            // Changed from int to string (supports "242-35-120")
    char name[50];
    char department[50];
    int semester;
    float cgpa;
};

// Function Prototypes
void adminMenu();
void studentMenu();
void addStudent();
void viewStudent();
void updateStudent();
void deleteStudent();
void viewAllStudents();
void viewOwnDetails(char[]);

// Main Program — Login Menu
int main() {
    int choice;
    char password[20];
    char studentId[20];

    while (1) {
        printf("\n===== Student Information Management System =====\n");
        printf("1. Admin Login\n");
        printf("2. Student Login\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter Admin Password: ");
                scanf("%s", password);
                if (strcmp(password, "admin123") == 0) {
                    adminMenu();
                } else {
                    printf("❌ Incorrect Password!\n");
                }
                break;

            case 2:
                printf("Enter your Student ID: ");
                scanf(" %[^\n]", studentId);
                viewOwnDetails(studentId);
                break;

            case 3:
                printf("👋 Exiting system... Goodbye!\n");
                exit(0);

            default:
                printf("❌ Invalid choice! Try again.\n");
        }
    }

    return 0;
}

// Admin Menu Function
void adminMenu() {
    int choice;

    do {
        printf("\n===== Admin Menu =====\n");
        printf("1. Add Student\n");
        printf("2. View Student by ID\n");
        printf("3. Update Student\n");
        printf("4. Delete Student\n");
        printf("5. View All Students\n");
        printf("6. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                addStudent();
                break;
            case 2:
                viewStudent();
                break;
            case 3:
                updateStudent();
                break;
            case 4:
                deleteStudent();
                break;
            case 5:
                viewAllStudents();
                break;
            case 6:
                printf("🔒 Logged out from Admin Panel.\n");
                break;
            default:
                printf("❌ Invalid choice! Try again.\n");
        }

    } while (choice != 6);
}



// Function to Add a New Student (CSV format, ID as string)
void addStudent() {
    FILE *fp;
    struct Student s;

    fp = fopen("students.txt", "a");
    if (fp == NULL) {
        printf("❌ Error opening file!\n");
        return;
    }

    printf("\nEnter Student ID: ");
    scanf(" %[^\n]", s.id);
    printf("Enter Name: ");
    scanf(" %[^\n]", s.name);
    printf("Enter Department: ");
    scanf(" %[^\n]", s.department);
    printf("Enter Semester: ");
    scanf("%d", &s.semester);
    printf("Enter CGPA: ");
    scanf("%f", &s.cgpa);

    fprintf(fp, "%s,%s,%s,%d,%.2f\n", s.id, s.name, s.department, s.semester, s.cgpa);
    fclose(fp);

    printf("✅ Student added successfully!\n");
}


// Function to View Student by ID (CSV format, ID as string)
void viewStudent() {
    FILE *fp;
    struct Student s;
    char searchId[20];
    int found = 0;

    fp = fopen("students.txt", "r");
    if (fp == NULL) {
        printf("❌ No student records found!\n");
        return;
    }

    printf("\nEnter Student ID to view: ");
    scanf(" %[^\n]", searchId);

    while (fscanf(fp, "%19[^,],%49[^,],%49[^,],%d,%f\n", s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        if (strcmp(s.id, searchId) == 0) {
            printf("\n--- Student Details ---\n");
            printf("ID: %s\n", s.id);
            printf("Name: %s\n", s.name);
            printf("Department: %s\n", s.department);
            printf("Semester: %d\n", s.semester);
            printf("CGPA: %.2f\n", s.cgpa);
            found = 1;
            break;
        }
    }

    if (!found)
        printf("❌ Student not found!\n");

    fclose(fp);
}


// Function to Update Student by ID (CSV format, ID as string)
void updateStudent() {
    FILE *fp, *tempFp;
    struct Student s;
    char searchId[20];
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

    printf("\nEnter Student ID to update: ");
    scanf(" %[^\n]", searchId);

    while (fscanf(fp, "%19[^,],%49[^,],%49[^,],%d,%f\n", s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        if (strcmp(s.id, searchId) == 0) {
            printf("Enter new details for student ID %s:\n", searchId);
            printf("Name: ");
            scanf(" %[^\n]", s.name);
            printf("Department: ");
            scanf(" %[^\n]", s.department);
            printf("Semester: ");
            scanf("%d", &s.semester);
            printf("CGPA: ");
            scanf("%f", &s.cgpa);
            found = 1;
        }
        fprintf(tempFp, "%s,%s,%s,%d,%.2f\n", s.id, s.name, s.department, s.semester, s.cgpa);
    }

    fclose(fp);
    fclose(tempFp);

    if (found) {
        remove("students.txt");
        rename("temp.txt", "students.txt");
        printf("✅ Student ID %s updated successfully!\n", searchId);
    } else {
        remove("temp.txt");
        printf("❌ Student ID %s not found!\n", searchId);
    }
}


// Function to Delete Student by ID (CSV format, ID as string)
void deleteStudent() {
    FILE *fp, *tempFp;
    struct Student s;
    char searchId[20];
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

    printf("\nEnter Student ID to delete: ");
    scanf(" %[^\n]", searchId);

    while (fscanf(fp, "%19[^,],%49[^,],%49[^,],%d,%f\n", s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        if (strcmp(s.id, searchId) == 0) {
            found = 1;
            continue; // skip this record
        }
        fprintf(tempFp, "%s,%s,%s,%d,%.2f\n", s.id, s.name, s.department, s.semester, s.cgpa);
    }

    fclose(fp);
    fclose(tempFp);

    if (found) {
        remove("students.txt");
        rename("temp.txt", "students.txt");
        printf("✅ Student ID %s deleted successfully!\n", searchId);
    } else {
        remove("temp.txt");
        printf("❌ Student ID %s not found!\n", searchId);
    }
}





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
    // Use fixed width columns with headers
    printf("%-15s %-20s %-15s %-10s %-5s\n", "ID", "Name", "Department", "Semester", "CGPA");

    while (fscanf(fp, "%19[^,],%49[^,],%49[^,],%d,%f\n", s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        printf("%-15s %-20s %-15s %-10d %.2f\n", s.id, s.name, s.department, s.semester, s.cgpa);
        count++;
    }

    if (count == 0)
        printf("❌ No student records to display!\n");

    fclose(fp);
}



// Function for Student to View Own Details (CSV format)
void viewOwnDetails(char studentId[]) {
    FILE *fp;
    struct Student s;
    int found = 0;

    fp = fopen("students.txt", "r");
    if (fp == NULL) {
        printf("❌ No student records found!\n");
        return;
    }

    while (fscanf(fp, "%19[^,],%49[^,],%49[^,],%d,%f\n", s.id, s.name, s.department, &s.semester, &s.cgpa) != EOF) {
        if (strcmp(s.id, studentId) == 0) {
            printf("\n--- Your Student Details ---\n");
            printf("ID: %s\n", s.id);
            printf("Name: %s\n", s.name);
            printf("Department: %s\n", s.department);
            printf("Semester: %d\n", s.semester);
            printf("CGPA: %.2f\n", s.cgpa);
            found = 1;
            break;
        }
    }

    if (!found)
        printf("❌ No student found with ID %s\n", studentId);

    fclose(fp);
}
