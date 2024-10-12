#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERS 100
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50
#define USER_FILE "users.txt"

// ����� ������ ������ ����ü
typedef struct {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
} User;

User users[MAX_USERS];
int user_count = 0;
int logged_in = -1;  // ���� �α��ε� ����� (-1�̸� �α��ε��� ����)

// ����� ���Ͽ��� �����͸� �о��
void load_users() {
    FILE *file = fopen(USER_FILE, "r");
    if (file == NULL) {
        return;  // ������ ���� ���, �� ����� ��� ����
    }

    while (fscanf(file, "%s %s", users[user_count].username, users[user_count].password) != EOF) {
        user_count++;
    }
    fclose(file);
}

// ����� ���Ͽ� �����͸� ������
void save_users() {
    FILE *file = fopen(USER_FILE, "w");
    if (file == NULL) {
        printf("Error opening file!\n");
        return;
    }

    for (int i = 0; i < user_count; i++) {
        fprintf(file, "%s %s\n", users[i].username, users[i].password);
    }
    fclose(file);
}

// ȸ������ �Լ�
void register_user() {
    if (user_count >= MAX_USERS) {
        printf("Maximum number of users reached.\n");
        return;
    }

    char username[MAX_USERNAME_LEN], password[MAX_PASSWORD_LEN];

    printf("Enter username: ");
    scanf("%s", username);

    // �ߺ� ����� Ȯ��
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            printf("Username already exists.\n");
            return;
        }
    }

    printf("Enter password: ");
    scanf("%s", password);

    strcpy(users[user_count].username, username);
    strcpy(users[user_count].password, password);
    user_count++;

    save_users();
    printf("User registered successfully!\n");
}

// �α��� �Լ�
void login() {
    if (logged_in != -1) {
        printf("Already logged in as %s\n", users[logged_in].username);
        return;
    }

    char username[MAX_USERNAME_LEN], password[MAX_PASSWORD_LEN];

    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            logged_in = i;
            printf("Login successful! Logged in as %s\n", users[i].username);
            return;
        }
    }

    printf("Invalid username or password.\n");
}

// �α׾ƿ� �Լ�
void logout() {
    if (logged_in == -1) {
        printf("No user is logged in.\n");
    } else {
        printf("User %s logged out.\n", users[logged_in].username);
        logged_in = -1;
    }
}

// �޴� ���
void menu() {
    printf("1. Register\n");
    printf("2. Login\n");
    printf("3. Logout\n");
    printf("4. Exit\n");
}

int main() {
    load_users();

    int choice;

    while (1) {
        menu();
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                register_user();
                break;
            case 2:
                login();
                break;
            case 3:
                logout();
                break;
            case 4:
                printf("Exiting...\n");
                save_users();
                exit(0);
            default:
                printf("Invalid choice. Try again.\n");
        }
    }

    return 0;
}
