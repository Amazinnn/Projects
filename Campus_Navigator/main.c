#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>

// 安全输入实现
void safe_input(char *buffer, int max_length) {
    if (fgets(buffer, max_length, stdin) != NULL) {
        // 移除换行符
        buffer[strcspn(buffer, "\n")] = 0;
    }
}

#include "data_management_2.0.h"
#include "database.h"
#include "route_planning.h"
#include "nearest_search.h"


void display_menu();
int get_user_choice();
void handle_route_planning(MYSQL *conn);
void handle_nearest_search(MYSQL *conn);

int main() {
    printf("=== Campus Navigation System ===\n");
    
    // 连接数据库
    MYSQL *conn = connect_database();
    if (!conn) {
        printf("Failed to connect to database. Exiting...\n");
        return -1;
    }
    
    printf("Database connected successfully.\n");
    
    int choice;
    do {
        display_menu();
        choice = get_user_choice();
        
        switch(choice) {
            case 1:
                handle_route_planning(conn);
                break;
            case 2:
                handle_nearest_search(conn);
                break;
            case 3:
                // 直接调用data_management.h中的函数
                handle_data_management(conn);
                break;
            case 0:
                printf("Exiting system...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
        printf("\n");
    } while (choice != 0);
    
    // 关闭数据库连接
    disconnect_database(conn);
    printf("System shutdown complete.\n");
    
    return 0;
}

// 显示菜单
void display_menu() {
    printf("\n--- Main Menu ---\n");
    printf("1. Route Planning\n");
    printf("2. Nearest Facility Search\n");
    printf("3. Data Management\n");
    printf("0. Exit\n");
    printf("Enter your choice: ");
}

// 获取用户选择
int get_user_choice() {
    int choice;
    char input[10];
    
    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (sscanf(input, "%d", &choice) == 1) {
            return choice;
        }
    }
    return -1; // 无效输入
}
