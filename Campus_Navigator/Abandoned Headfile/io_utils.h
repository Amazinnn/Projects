// io_utils.h
#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>

#define MAX_INPUT_LENGTH 100
#define MAX_RESULTS 20

// 安全输入函数
void safe_input(char *buffer, int max_length);

// 模糊搜索结构体
typedef struct {
    int id;
    char display_text[100];
} SearchResult;

// 模糊搜索函数
int fuzzy_search(MYSQL *conn, const char *table_name, const char *id_column, 
                 const char *search_columns[], int num_search_columns, 
                 const char *display_columns[], int num_display_columns,
                 const char *prompt, int *selected_id, char *selected_display);

// 从列表中选择项
int select_from_list(const char *title, const char *items[], int num_items, 
                     const char *prompt);

// 显示搜索结果
void display_search_results(MYSQL_RES *result, const char *display_columns[], 
                           int num_display_columns);

// 安全输入实现
void safe_input(char *buffer, int max_length) {
    if (fgets(buffer, max_length, stdin) != NULL) {
        // 移除换行符
        buffer[strcspn(buffer, "\n")] = 0;
    }
}

// 模糊搜索实现
int fuzzy_search(MYSQL *conn, const char *table_name, const char *id_column, 
                 const char *search_columns[], int num_search_columns, 
                 const char *display_columns[], int num_display_columns,
                 const char *prompt, int *selected_id, char *selected_display) {
    
    int attempts = 0;
    int max_attempts = 3;
    
    while (attempts < max_attempts) {
        printf("%s", prompt);
        
        char search_term[MAX_INPUT_LENGTH];
        safe_input(search_term, sizeof(search_term));
        
        if (strlen(search_term) == 0) {
            printf("Please enter a search term.\n");
            attempts++;
            continue;
        }
        
        // 构建搜索条件（对大小写不敏感）
        char where_conditions[512] = "";
        for (int i = 0; i < num_search_columns; i++) {
            if (i > 0) {
                strcat(where_conditions, " OR ");
            }
            char condition[100];
            // 使用LOWER函数确保大小写不敏感
            snprintf(condition, sizeof(condition), 
                     "LOWER(%s) LIKE LOWER('%%%s%%')", search_columns[i], search_term);
            strcat(where_conditions, condition);
        }
        
        // 构建显示列
        char display_columns_str[200] = "";
        strcat(display_columns_str, id_column);
        for (int i = 0; i < num_display_columns; i++) {
            char temp[50];
            snprintf(temp, sizeof(temp), ", %s", display_columns[i]);
            strcat(display_columns_str, temp);
        }
        
        // 构建查询
        char query[1024];
        snprintf(query, sizeof(query),
                 "SELECT %s FROM %s WHERE %s ORDER BY %s LIMIT %d",
                 display_columns_str, table_name, where_conditions, 
                 display_columns[0], MAX_RESULTS);
        
        if (mysql_query(conn, query)) {
            printf("Search error: %s\n", mysql_error(conn));
            attempts++;
            continue;
        }
        
        MYSQL_RES *result = mysql_store_result(conn);
        if (result == NULL) {
            printf("Error storing result: %s\n", mysql_error(conn));
            attempts++;
            continue;
        }
        
        int num_rows = mysql_num_rows(result);
        if (num_rows == 0) {
            printf("No results found matching '%s'. Try again.\n", search_term);
            mysql_free_result(result);
            attempts++;
            continue;
        }
        
        printf("\nSearch results for '%s':\n", search_term);
        printf("No.\t");
        
        // 显示列标题
        for (int i = 0; i < num_display_columns; i++) {
            printf("%s\t", display_columns[i]);
        }
        printf("\n");
        
        printf("--------------------------------------------------\n");
        
        MYSQL_ROW row;
        int count = 0;
        int result_ids[MAX_RESULTS];
        char result_displays[MAX_RESULTS][100];
        
        while ((row = mysql_fetch_row(result)) && count < MAX_RESULTS) {
            result_ids[count] = atoi(row[0]);
            
            // 构建显示文本
            char display[100] = "";
            for (int i = 0; i < num_display_columns; i++) {
                if (i > 0) {
                    strcat(display, " | ");
                }
                strncat(display, row[i + 1], 20); // +1 因为第一个是ID
            }
            strncpy(result_displays[count], display, 99);
            result_displays[count][99] = '\0';
            
            printf("%d.\t%s\n", count + 1, result_displays[count]);
            count++;
        }
        
        if (count == 1) {
            // 只有一个结果，自动选择
            *selected_id = result_ids[0];
            strcpy(selected_display, result_displays[0]);
            printf("Automatically selected: %s\n", selected_display);
            mysql_free_result(result);
            return 0;
        }
        
        // 多个结果，让用户选择
        printf("\nSelect an item (1-%d) or 0 to search again: ", count);
        char choice_input[10];
        safe_input(choice_input, sizeof(choice_input));
        
        int choice = atoi(choice_input);
        if (choice > 0 && choice <= count) {
            *selected_id = result_ids[choice - 1];
            strcpy(selected_display, result_displays[choice - 1]);
            printf("Selected: %s\n", selected_display);
            mysql_free_result(result);
            return 0;
        } else if (choice == 0) {
            // 重新搜索
            mysql_free_result(result);
            attempts++;
        } else {
            printf("Invalid selection.\n");
            mysql_free_result(result);
            attempts++;
        }
    }
    
    printf("Search failed after %d attempts.\n", max_attempts);
    return -1;
}

// 从列表中选择项
int select_from_list(const char *title, const char *items[], int num_items, 
                     const char *prompt) {
    
    if (num_items == 0) {
        printf("No items available.\n");
        return -1;
    }
    
    printf("\n%s:\n", title);
    for (int i = 0; i < num_items; i++) {
        printf("%d. %s\n", i + 1, items[i]);
    }
    
    int attempts = 0;
    int max_attempts = 3;
    
    while (attempts < max_attempts) {
        printf("%s", prompt);
        
        char input[10];
        safe_input(input, sizeof(input));
        
        int choice = atoi(input);
        if (choice > 0 && choice <= num_items) {
            return choice - 1; // 返回索引
        } else {
            printf("Invalid selection. Please enter a number between 1 and %d.\n", num_items);
            attempts++;
        }
    }
    
    printf("Failed to make a selection after %d attempts.\n", max_attempts);
    return -1;
}

// 显示搜索结果（通用函数）
void display_search_results(MYSQL_RES *result, const char *display_columns[], 
                           int num_display_columns) {
    
    int num_rows = mysql_num_rows(result);
    if (num_rows == 0) {
        printf("No results found.\n");
        return;
    }
    
    // 显示列标题
    printf("No.\t");
    for (int i = 0; i < num_display_columns; i++) {
        printf("%s\t", display_columns[i]);
    }
    printf("\n");
    
    printf("--------------------------------------------------\n");
    
    // 显示数据
    MYSQL_ROW row;
    int count = 1;
    while ((row = mysql_fetch_row(result))) {
        printf("%d.\t", count++);
        for (int i = 0; i < num_display_columns; i++) {
            printf("%s\t", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }
}

#endif
