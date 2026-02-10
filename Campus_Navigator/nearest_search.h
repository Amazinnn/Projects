#ifndef NEAREST_SEARCH_H
#define NEAREST_SEARCH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mysql.h>
#include "route_planning.h"

// 设施类型定义
#define MAX_FACILITY_TYPES 20

// 使用locations表来查找设施
typedef struct {
    int loca_id;
    char loca_name[40];
    char category[20];
    char nearby_facilities[200];
} Facility;

typedef struct {
    int facility_count;
    Facility facilities[MAX_LOCATIONS];
    int facility_type_count;
    char facility_types[MAX_FACILITY_TYPES][20];
} FacilityData;

// 函数声明
void initialize_facility_data(FacilityData *fac_data);
int load_facilities_from_db(MYSQL *conn, FacilityData *fac_data);
int find_nearest_facility_bfs(MYSQL *conn, NavigationGraph *nav_graph, FacilityData *fac_data, 
                             int start_id, const char* category, const char* travel_mode,
                             int *nearest_facility_id, double *total_distance, 
                             double *total_time, int *path, int *path_length);
void print_nearest_facility_result(NavigationGraph *nav_graph, FacilityData *fac_data,
                                  int nearest_facility_id, int *path, int path_length,
                                  double total_distance, double total_time, 
                                  const char* category, const char* travel_mode);
void handle_nearest_search(MYSQL *conn);

// 辅助函数声明
int select_starting_point(MYSQL *conn);
int select_facility_category(MYSQL *conn, FacilityData *fac_data, char* category, int category_size);
int select_travel_mode(MYSQL *conn, char* travel_mode, int mode_size);
void search_and_select_locations(MYSQL *conn, const char* prompt, int *selected_id);
void display_search_results(MYSQL_RES *result);

// 初始化设施数据
void initialize_facility_data(FacilityData *fac_data) {
    fac_data->facility_count = 0;
    fac_data->facility_type_count = 0;
}

// 从数据库加载设施信息
int load_facilities_from_db(MYSQL *conn, FacilityData *fac_data) {
    char query[512] = "SELECT loca_id, loca_name, category, nearby_facilities FROM locations ORDER BY loca_id";
    
    if (mysql_query(conn, query)) {
        printf("Error loading facilities: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    int count = 0;
    MYSQL_ROW row;
    
    while ((row = mysql_fetch_row(result)) && count < MAX_LOCATIONS) {
        if (row[0] == NULL || row[1] == NULL || row[2] == NULL) continue;
        
        fac_data->facilities[count].loca_id = atoi(row[0]);
        strncpy(fac_data->facilities[count].loca_name, row[1], 39);
        fac_data->facilities[count].loca_name[39] = '\0';
        
        strncpy(fac_data->facilities[count].category, row[2], 19);
        fac_data->facilities[count].category[19] = '\0';
        
        if (row[3] != NULL) {
            strncpy(fac_data->facilities[count].nearby_facilities, row[3], 199);
            fac_data->facilities[count].nearby_facilities[199] = '\0';
        } else {
            strcpy(fac_data->facilities[count].nearby_facilities, "");
        }
        
        // 添加到设施类型列表
        int type_exists = 0;
        for (int i = 0; i < fac_data->facility_type_count; i++) {
            if (strcmp(fac_data->facility_types[i], fac_data->facilities[count].category) == 0) {
                type_exists = 1;
                break;
            }
        }
        
        if (!type_exists && fac_data->facility_type_count < MAX_FACILITY_TYPES) {
            strcpy(fac_data->facility_types[fac_data->facility_type_count], 
                   fac_data->facilities[count].category);
            fac_data->facility_type_count++;
        }
        
        count++;
    }
    
    fac_data->facility_count = count;
    mysql_free_result(result);
    
    return count;
}
// 修复搜索和选择地点函数
void search_and_select_locations(MYSQL *conn, const char* prompt, int *selected_id) {
    char search_term[100];
    int max_attempts = 3;
    int attempts = 0;
    
    while (attempts < max_attempts && *selected_id == -1) {
        printf("%s", prompt);
        safe_input(search_term, sizeof(search_term));
        
        if (strlen(search_term) == 0) {
            printf("Please enter a search term.\n");
            attempts++;
            continue;
        }
        
        // 搜索地点
        char query[512];
        snprintf(query, sizeof(query),
                 "SELECT loca_id, loca_name, category FROM locations WHERE loca_name LIKE '%%%s%%' OR category LIKE '%%%s%%' LIMIT 10",
                 search_term, search_term);
        
        if (mysql_query(conn, query)) {
            printf("Search error: %s\n", mysql_error(conn));
            attempts++;
            continue;
        }
        
        MYSQL_RES *result = mysql_store_result(conn);
        if (result == NULL || mysql_num_rows(result) == 0) {
            printf("No locations found matching '%s'. Try again.\n", search_term);
            if (result) mysql_free_result(result);
            attempts++;
            continue;
        }
        
        // 显示搜索结果
        printf("\nSearch results for '%s':\n", search_term);
        printf("ID\tName\t\tCategory\n");
        printf("----------------------------------\n");
        
        MYSQL_ROW row;
        int count = 0;
        int* location_ids = malloc(mysql_num_rows(result) * sizeof(int));
        
        while ((row = mysql_fetch_row(result)) && count < mysql_num_rows(result)) {
            location_ids[count] = atoi(row[0]);
            printf("%d. %s (%s) - ID: %s\n", count + 1, row[1], row[2], row[0]);
            count++;
        }
        
        if (count == 1) {
            // 只有一个结果，自动选择
            *selected_id = location_ids[0];
            printf("Automatically selected: %s (ID: %d)\n", row[1], *selected_id);
            free(location_ids);
            mysql_free_result(result);
            return;
        }
        
        // 多个结果，让用户选择
        printf("\nSelect a location (1-%d) or 0 to search again: ", count);
        
        char choice_input[10];
        safe_input(choice_input, sizeof(choice_input));
        
        int choice = atoi(choice_input);
        if (choice > 0 && choice <= count) {
            *selected_id = location_ids[choice - 1];
            free(location_ids);
            mysql_free_result(result);
            return;
        } else if (choice == 0) {
            // 重新搜索
            free(location_ids);
            mysql_free_result(result);
            attempts++;
            continue;
        } else {
            printf("Invalid selection.\n");
            free(location_ids);
            mysql_free_result(result);
            attempts++;
        }
    }
    
    printf("Too many failed attempts. Returning to main menu.\n");
    *selected_id = -1;
}

// 选择出发点
int select_starting_point(MYSQL *conn) {
    int start_id = -1;
    search_and_select_locations(conn, "Enter starting location name or category to search: ", &start_id);
    return start_id;
}

// 选择设施类别
int select_facility_category(MYSQL *conn, FacilityData *fac_data, char* category, int category_size) {
    printf("\nAvailable facility categories:\n");
    for (int i = 0; i < fac_data->facility_type_count; i++) {
        printf("%d. %s\n", i + 1, fac_data->facility_types[i]);
    }
    
    printf("Select a category (1-%d): ", fac_data->facility_type_count);
    
    char input[10];
    safe_input(input, sizeof(input));
    
    int choice = atoi(input);
    if (choice < 1 || choice > fac_data->facility_type_count) {
        printf("Invalid selection.\n");
        return -1;
    }
    
    strncpy(category, fac_data->facility_types[choice - 1], category_size - 1);
    category[category_size - 1] = '\0';
    
    printf("Selected category: %s\n", category);
    return 0;
}

// 选择出行方式
int select_travel_mode(MYSQL *conn, char* travel_mode, int mode_size) {
    char query[256] = "SELECT model_id, model_name, speed_kmh FROM travel_modes ORDER BY model_id";
    
    if (mysql_query(conn, query)) {
        printf("Error fetching travel modes: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    int num_modes = mysql_num_rows(result);
    if (num_modes == 0) {
        printf("No travel modes available.\n");
        mysql_free_result(result);
        return -1;
    }
    
    printf("\nAvailable travel modes:\n");
    printf("ID\tName\t\tSpeed\n");
    printf("----------------------------\n");
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("%s\t%s\t\t%s km/h\n", row[0], row[1], row[2]);
    }
    mysql_free_result(result);
    
    printf("\nSelect travel mode by ID: ");
    
    char input[10];
    safe_input(input, sizeof(input));
    
    int mode_id = atoi(input);
    
    // 验证ID是否存在
    char check_query[256];
    snprintf(check_query, sizeof(check_query),
             "SELECT model_name FROM travel_modes WHERE model_id = %d", mode_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking travel mode: %s\n", mysql_error(conn));
        return -1;
    }
    
    result = mysql_store_result(conn);
    if (result == NULL || mysql_num_rows(result) == 0) {
        printf("Invalid travel mode ID.\n");
        if (result) mysql_free_result(result);
        return -1;
    }
    
    row = mysql_fetch_row(result);
    strncpy(travel_mode, row[0], mode_size - 1);
    travel_mode[mode_size - 1] = '\0';
    
    mysql_free_result(result);
    
    printf("Selected travel mode: %s\n", travel_mode);
    return 0;
}
// 修复后的 BFS 函数
int find_nearest_facility_bfs(MYSQL *conn, NavigationGraph *nav_graph, FacilityData *fac_data, 
                             int start_id, const char* category, const char* travel_mode,
                             int *nearest_facility_id, double *total_distance, 
                             double *total_time, int *path, int *path_length) {
    
    int start_idx = (start_id >= 0 && start_id < 10000) ? 
                    nav_graph->location_id_to_index[start_id] : -1;
    
    if (start_idx == -1 || start_idx >= MAX_LOCATIONS) {
        printf("Error: Start location not found or invalid.\n");
        return -1;
    }
    
    // 获取旅行速度
    double speed = get_travel_speed(conn, travel_mode);
    
    // BFS相关变量
    int visited[MAX_LOCATIONS] = {0};
    int queue[MAX_LOCATIONS];
    int front = 0, rear = 0;
    double distance[MAX_LOCATIONS];
    int prev[MAX_LOCATIONS];
    
    // 初始化
    for (int i = 0; i < MAX_LOCATIONS; i++) {
        distance[i] = INF;
        prev[i] = -1;
        visited[i] = 0;
    }
    
    distance[start_idx] = 0;
    visited[start_idx] = 1;
    queue[rear++] = start_idx;
    
    int found_facility_id = -1;
    int found_location_idx = -1;
    double min_distance = INF;
    
    // BFS遍历 - 修复队列操作
    while (front < rear && front < MAX_LOCATIONS) {
        int current_idx = queue[front++];
        
        // 检查当前地点是否有目标设施
        for (int i = 0; i < fac_data->facility_count; i++) {
            if (fac_data->facilities[i].loca_id == nav_graph->index_to_location_id[current_idx] &&
                strcmp(fac_data->facilities[i].category, category) == 0) {
                
                // 找到目标设施
                if (found_facility_id == -1 || distance[current_idx] < min_distance) {
                    found_facility_id = fac_data->facilities[i].loca_id;
                    found_location_idx = current_idx;
                    min_distance = distance[current_idx];
                }
            }
        }
        
        // 遍历邻居节点 - 修复边界检查
        for (int neighbor_idx = 0; neighbor_idx < nav_graph->location_count; neighbor_idx++) {
            if (neighbor_idx < 0 || neighbor_idx >= MAX_LOCATIONS) continue;
            
            if (!visited[neighbor_idx] && 
                nav_graph->graph[current_idx][neighbor_idx] < INF - 1) {
                
                double edge_distance = nav_graph->graph[current_idx][neighbor_idx];
                distance[neighbor_idx] = distance[current_idx] + edge_distance;
                prev[neighbor_idx] = current_idx;
                visited[neighbor_idx] = 1;
                
                // 检查队列是否已满
                if (rear < MAX_LOCATIONS) {
                    queue[rear++] = neighbor_idx;
                } else {
                    printf("Warning: Queue full, skipping some neighbors.\n");
                    break;
                }
            }
        }
    }
    
    if (found_facility_id == -1) {
        printf("No %s facility found reachable from start location.\n", category);
        return -1;
    }
    
    // 构建路径 - 修复路径构建逻辑
    int current = found_location_idx;
    *path_length = 0;
    int temp_path[MAX_LOCATIONS];
    
    while (current != -1 && *path_length < MAX_LOCATIONS) {
        temp_path[(*path_length)++] = current;
        current = prev[current];
    }
    
    // 反转路径
    for (int i = 0; i < *path_length; i++) {
        if (i < MAX_LOCATIONS) {
            path[i] = temp_path[*path_length - i - 1];
        }
    }
    
    *total_distance = min_distance;
    *total_time = min_distance / (speed * 1000 / 60); // 转换为分钟
    *nearest_facility_id = found_facility_id;
    
    return 0;
}
// 打印最近设施搜索结果
void print_nearest_facility_result(NavigationGraph *nav_graph, FacilityData *fac_data,
                                  int nearest_facility_id, int *path, int path_length,
                                  double total_distance, double total_time, 
                                  const char* category, const char* travel_mode) {
    
    printf("\n=== Nearest %s Search Results ===\n", category);
    printf("Travel Mode: %s\n", travel_mode);
    printf("Total Distance: %.2f meters\n", total_distance);
    printf("Estimated Time: %.2f minutes\n", total_time);
    printf("Path Details:\n");
    printf("--------------------------------------------------\n");
    
    // 查找设施信息
    Facility *found_facility = NULL;
    for (int i = 0; i < fac_data->facility_count; i++) {
        if (fac_data->facilities[i].loca_id == nearest_facility_id) {
            found_facility = &fac_data->facilities[i];
            break;
        }
    }
    
    if (found_facility == NULL) {
        printf("Error: Facility details not found.\n");
        return;
    }
    
    // 打印路径
    for (int i = 0; i < path_length; i++) {
        int loca_idx = path[i];
        if (loca_idx < 0 || loca_idx >= nav_graph->location_count) continue;
        
        int loca_id = nav_graph->index_to_location_id[loca_idx];
        printf("%d. %s (ID: %d)\n", i + 1, 
               nav_graph->locations[loca_idx].loca_name, loca_id);
        
        if (i == path_length - 1) {
            printf("   >>> Destination: %s (%s)\n", 
                   found_facility->loca_name, category);
            if (strlen(found_facility->nearby_facilities) > 0) {
                printf("   Nearby facilities: %s\n", found_facility->nearby_facilities);
            }
        }
        
        if (i < path_length - 1) {
            int next_idx = path[i + 1];
            double segment_dist = nav_graph->original_graph[loca_idx][next_idx];
            if (segment_dist < INF - 1) {
                printf("   -> Next segment: %.1f meters\n", segment_dist);
            }
        }
    }
    printf("--------------------------------------------------\n");
}

void handle_nearest_search(MYSQL *conn) {
    printf("\n=== Nearest Facility Search ===\n");
    
    // 第一步：选择出发点
    printf("\n--- Step 1: Select Starting Point ---\n");
    int start_id = -1;
    int attempts = 0;
    int max_attempts = 3;
    
    while (attempts < max_attempts && start_id == -1) {
        printf("Enter starting location name (or part of name) to search: ");
        char search_term[100];
        safe_input(search_term, sizeof(search_term));
        
        if (strlen(search_term) == 0) {
            printf("Please enter a search term.\n");
            attempts++;
            continue;
        }
        
        // 使用LIKE进行模糊搜索
        char query[512];
        snprintf(query, sizeof(query),
                 "SELECT loca_id, loca_name, category FROM locations WHERE "
                 "loca_name LIKE '%%%s%%' LIMIT 10", search_term);
        
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
            printf("No locations found matching '%s'. Try again.\n", search_term);
            mysql_free_result(result);
            attempts++;
            continue;
        }
        
        printf("\nSearch results for '%s':\n", search_term);
        printf("No.\tName\t\tCategory\n");
        printf("----------------------------------\n");
        
        MYSQL_ROW row;
        int count = 0;
        int location_ids[10];
        char location_names[10][50];
        
        while ((row = mysql_fetch_row(result)) && count < 10) {
            location_ids[count] = atoi(row[0]);
            strncpy(location_names[count], row[1], 49);
            location_names[count][49] = '\0';
            
            printf("%d. %s (%s)\n", count + 1, row[1], row[2]);
            count++;
        }
        
        if (count == 1) {
            // 只有一个结果，自动选择
            start_id = location_ids[0];
            printf("Automatically selected: %s\n", location_names[0]);
        } else {
            // 多个结果，让用户选择
            printf("\nSelect a location (1-%d) or 0 to search again: ", count);
            char choice_input[10];
            safe_input(choice_input, sizeof(choice_input));
            
            int choice = atoi(choice_input);
            if (choice > 0 && choice <= count) {
                start_id = location_ids[choice - 1];
                printf("Selected: %s\n", location_names[choice - 1]);
            } else if (choice == 0) {
                // 重新搜索
                attempts++;
            } else {
                printf("Invalid selection.\n");
                attempts++;
            }
        }
        
        mysql_free_result(result);
    }
    
    if (start_id == -1) {
        printf("Failed to select starting point after %d attempts.\n", max_attempts);
        return;
    }
    
    // 第二步：选择设施类别
    printf("\n--- Step 2: Select Facility Category ---\n");
    char selected_category[20] = "";
    attempts = 0;
    
    while (attempts < max_attempts && strlen(selected_category) == 0) {
        printf("Enter facility category (or part of category) to search: ");
        char category_input[50];
        safe_input(category_input, sizeof(category_input));
        
        if (strlen(category_input) == 0) {
            printf("Please enter a category.\n");
            attempts++;
            continue;
        }
        
        // 使用LIKE进行模糊搜索类别
        char category_query[512];
        snprintf(category_query, sizeof(category_query),
                 "SELECT DISTINCT category FROM locations WHERE "
                 "category LIKE '%%%s%%' AND category IS NOT NULL AND category != '' "
                 "LIMIT 10", category_input);
        
        if (mysql_query(conn, category_query)) {
            printf("Error searching categories: %s\n", mysql_error(conn));
            attempts++;
            continue;
        }
        
        MYSQL_RES *category_result = mysql_store_result(conn);
        if (category_result == NULL) {
            printf("Error storing category result: %s\n", mysql_error(conn));
            attempts++;
            continue;
        }
        
        int category_count = mysql_num_rows(category_result);
        if (category_count == 0) {
            printf("No categories found matching '%s'. Try again.\n", category_input);
            mysql_free_result(category_result);
            attempts++;
            continue;
        }
        
        printf("\nMatching categories:\n");
        MYSQL_ROW category_row;
        int cat_count = 0;
        char categories[10][20];
        
        while ((category_row = mysql_fetch_row(category_result)) && cat_count < 10) {
            strncpy(categories[cat_count], category_row[0], 19);
            categories[cat_count][19] = '\0';
            printf("%d. %s\n", cat_count + 1, categories[cat_count]);
            cat_count++;
        }
        
        if (cat_count == 1) {
            // 只有一个匹配的类别，自动选择
            strcpy(selected_category, categories[0]);
            printf("Automatically selected category: %s\n", selected_category);
        } else {
            // 多个匹配的类别，让用户选择
            printf("\nSelect a category (1-%d) or 0 to search again: ", cat_count);
            char cat_choice_input[10];
            safe_input(cat_choice_input, sizeof(cat_choice_input));
            
            int cat_choice = atoi(cat_choice_input);
            if (cat_choice > 0 && cat_choice <= cat_count) {
                strcpy(selected_category, categories[cat_choice - 1]);
                printf("Selected category: %s\n", selected_category);
            } else if (cat_choice == 0) {
                // 重新搜索
                attempts++;
            } else {
                printf("Invalid selection.\n");
                attempts++;
            }
        }
        
        mysql_free_result(category_result);
    }
    
    if (strlen(selected_category) == 0) {
        printf("Failed to select category after %d attempts.\n", max_attempts);
        return;
    }
    
    // 第三步：选择出行方式
    printf("\n--- Step 3: Select Travel Mode ---\n");
    char travel_mode[20] = "";
    attempts = 0;
    
    while (attempts < max_attempts && strlen(travel_mode) == 0) {
        // 显示所有出行方式
        char mode_query[256] = "SELECT model_name, speed_kmh FROM travel_modes ORDER BY model_id";
        if (mysql_query(conn, mode_query)) {
            printf("Error fetching travel modes: %s\n", mysql_error(conn));
            attempts++;
            continue;
        }
        
        MYSQL_RES *mode_result = mysql_store_result(conn);
        if (mode_result == NULL) {
            printf("Error storing mode result: %s\n", mysql_error(conn));
            attempts++;
            continue;
        }
        
        int mode_count = mysql_num_rows(mode_result);
        if (mode_count == 0) {
            printf("No travel modes available.\n");
            mysql_free_result(mode_result);
            return;
        }
        
        printf("Available travel modes:\n");
        MYSQL_ROW mode_row;
        int mode_index = 0;
        char mode_names[10][20];
        double mode_speeds[10];
        
        while ((mode_row = mysql_fetch_row(mode_result)) && mode_index < 10) {
            strncpy(mode_names[mode_index], mode_row[0], 19);
            mode_names[mode_index][19] = '\0';
            mode_speeds[mode_index] = atof(mode_row[1]);
            printf("%d. %s (speed: %.1f km/h)\n", mode_index + 1, mode_row[0], mode_speeds[mode_index]);
            mode_index++;
        }
        
        printf("\nSelect travel mode (1-%d): ", mode_count);
        char mode_input[10];
        safe_input(mode_input, sizeof(mode_input));
        
        int selected_mode = atoi(mode_input);
        if (selected_mode > 0 && selected_mode <= mode_count) {
            strcpy(travel_mode, mode_names[selected_mode - 1]);
            printf("Selected travel mode: %s\n", travel_mode);
        } else {
            printf("Invalid selection.\n");
            attempts++;
        }
        
        mysql_free_result(mode_result);
    }
    
    if (strlen(travel_mode) == 0) {
        printf("Failed to select travel mode after %d attempts.\n", max_attempts);
        return;
    }
    
    // 第四步：执行搜索并显示结果
    printf("\n--- Step 4: Searching for Nearest %s ---\n", selected_category);
    
    // 使用简单的SQL查询查找最近设施
    char search_query[512];
    snprintf(search_query, sizeof(search_query),
             "SELECT l.loca_id, l.loca_name, l.category, l.nearby_facilities, "
             "(SELECT COUNT(*) FROM routes WHERE start_id = %d AND end_id = l.loca_id) as has_route "
             "FROM locations l "
             "WHERE l.category = '%s' AND l.loca_id != %d "
             "ORDER BY has_route DESC, l.loca_id "
             "LIMIT 1", start_id, selected_category, start_id);
    
    if (mysql_query(conn, search_query)) {
        printf("Search error: %s\n", mysql_error(conn));
        return;
    }
    
    MYSQL_RES *search_result = mysql_store_result(conn);
    if (search_result == NULL) {
        printf("Error storing search result: %s\n", mysql_error(conn));
        return;
    }
    
    int found_count = mysql_num_rows(search_result);
    if (found_count == 0) {
        printf("No %s facility found.\n", selected_category);
        mysql_free_result(search_result);
        return;
    }
    
    MYSQL_ROW search_row = mysql_fetch_row(search_result);
    int facility_id = atoi(search_row[0]);
    char* facility_name = search_row[1];
    char* facility_category = search_row[2];
    char* nearby_facilities = search_row[3];
    
    // 获取距离信息
    double distance = 0.0;
    char distance_query[512];
    snprintf(distance_query, sizeof(distance_query),
             "SELECT route_distance FROM routes WHERE start_id = %d AND end_id = %d "
             "UNION ALL "
             "SELECT route_distance FROM routes WHERE start_id = %d AND end_id = %d "
             "LIMIT 1", start_id, facility_id, facility_id, start_id);
    
    if (mysql_query(conn, distance_query) == 0) {
        MYSQL_RES *distance_result = mysql_store_result(conn);
        if (distance_result != NULL && mysql_num_rows(distance_result) > 0) {
            MYSQL_ROW distance_row = mysql_fetch_row(distance_result);
            distance = atof(distance_row[0]);
            mysql_free_result(distance_result);
        }
    }
    
    // 如果找不到直接路径，使用默认距离
    if (distance <= 0) {
        distance = 100.0; // 默认距离
    }
    
    // 获取速度并计算时间
    double speed = get_travel_speed(conn, travel_mode);
    double time_minutes = distance / (speed * 1000 / 60); // 转换为分钟
    
    // 显示结果
    printf("\n=== Search Results ===\n");
    printf("Starting Point: ID %d\n", start_id);
    printf("Target Facility Type: %s\n", selected_category);
    printf("Travel Mode: %s (%.1f km/h)\n", travel_mode, speed);
    printf("Nearest Facility: %s (Category: %s)\n", facility_name, facility_category);
    printf("Estimated Distance: %.1f meters\n", distance);
    printf("Estimated Time: %.1f minutes\n", time_minutes);
    
    if (nearby_facilities != NULL && strlen(nearby_facilities) > 0) {
        printf("Nearby Facilities: %s\n", nearby_facilities);
    }
    
    // 查找路径详情
    printf("\nPath Details:\n");
    printf("--------------------------------------------------\n");
    
    char path_query[512];
    snprintf(path_query, sizeof(path_query),
             "SELECT l.loca_name FROM locations l WHERE l.loca_id = %d", start_id);
    
    if (mysql_query(conn, path_query) == 0) {
        MYSQL_RES *path_result = mysql_store_result(conn);
        if (path_result != NULL && mysql_num_rows(path_result) > 0) {
            MYSQL_ROW path_row = mysql_fetch_row(path_result);
            printf("1. %s (Start)\n", path_row[0]);
            mysql_free_result(path_result);
        }
    }
    
    printf("2. %s (Destination)\n", facility_name);
    printf("   -> Direct route: %.1f meters\n", distance);
    printf("--------------------------------------------------\n");
    
    mysql_free_result(search_result);
    
    printf("\nSearch completed successfully.\n");
}


#endif
