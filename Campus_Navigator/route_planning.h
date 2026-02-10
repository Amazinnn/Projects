#ifndef ROUTE_PLANNING_H
#define ROUTE_PLANNING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mysql.h>
#include <float.h>

// 图结构定义
#define MAX_LOCATIONS 100
#define INF 1e9

typedef struct {
    int loca_id;
    char loca_name[50];
} Location;

typedef struct {
    int location_count;
    int route_count;
    Location locations[MAX_LOCATIONS];
    double graph[MAX_LOCATIONS][MAX_LOCATIONS]; // 邻接矩阵（加权距离）
    double original_graph[MAX_LOCATIONS][MAX_LOCATIONS]; // 原始距离
    int prev[MAX_LOCATIONS]; // 前驱节点
    int location_id_to_index[10000]; // ID到索引映射（假设ID不超过10000）
    int index_to_location_id[MAX_LOCATIONS]; // 索引到ID映射
} NavigationGraph;

// 函数声明
void initialize_graph(NavigationGraph *graph);
int load_locations_from_db(MYSQL *conn, NavigationGraph *nav_graph);
int load_routes_from_db(MYSQL *conn, NavigationGraph *nav_graph);
double get_travel_speed(MYSQL *conn, const char* travel_mode);
double calculate_weighted_distance(double distance, int crowded_rate, const char* travel_mode);
int build_navigation_graph(MYSQL *conn, NavigationGraph *nav_graph, const char* travel_mode);
int find_shortest_path(NavigationGraph *nav_graph, int start_id, int end_id, 
                      double *total_distance, double *total_time, int *path, int *path_length);
void print_path_details(NavigationGraph *nav_graph, int *path, int path_length, 
                       double total_distance, double total_time, const char* travel_mode);
void handle_route_planning(MYSQL *conn);


// 图初始化
void initialize_graph(NavigationGraph *graph) {
    graph->location_count = 0;
    graph->route_count = 0;
    
    for (int i = 0; i < MAX_LOCATIONS; i++) {
        for (int j = 0; j < MAX_LOCATIONS; j++) {
            graph->graph[i][j] = INF;
            graph->original_graph[i][j] = INF;
        }
        graph->index_to_location_id[i] = -1;
    }
    
    for (int i = 0; i < 10000; i++) {
        graph->location_id_to_index[i] = -1;
    }
}

// 从数据库加载地点信息
int load_locations_from_db(MYSQL *conn, NavigationGraph *nav_graph) {
    char query[256] = "SELECT loca_id, loca_name FROM locations ORDER BY loca_id";
    
    if (mysql_query(conn, query)) {
        printf("Error loading locations: %s\n", mysql_error(conn));
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
        if (row[0] == NULL || row[1] == NULL) continue;
        
        int loca_id = atoi(row[0]);
        nav_graph->locations[count].loca_id = loca_id;
        strncpy(nav_graph->locations[count].loca_name, row[1], 49);
        nav_graph->locations[count].loca_name[49] = '\0';
        
        // 建立ID和索引的映射
        if (loca_id >= 0 && loca_id < 10000) {
            nav_graph->location_id_to_index[loca_id] = count;
        }
        nav_graph->index_to_location_id[count] = loca_id;
        
        count++;
    }
    
    nav_graph->location_count = count;
    mysql_free_result(result);
    
    printf("Loaded %d locations from database.\n", count);
    return count;
}

// 从数据库获取出行方式的速度
double get_travel_speed(MYSQL *conn, const char* travel_mode) {
    char query[256];
    snprintf(query, sizeof(query), 
             "SELECT speed_kmh FROM travel_modes WHERE model_name = '%s'", travel_mode);
    
    if (mysql_query(conn, query)) {
        printf("Error querying travel speed: %s\n", mysql_error(conn));
        return 5.0; // 默认步行速度
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL || mysql_num_rows(result) == 0) {
        printf("Travel mode '%s' not found, using default walking speed.\n", travel_mode);
        if (result) mysql_free_result(result);
        return 5.0; // 默认步行速度
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row[0] == NULL) {
        mysql_free_result(result);
        return 5.0;
    }
    
    double speed = atof(row[0]);
    mysql_free_result(result);
    
    return speed;
}

// 计算加权距离（考虑拥挤程度）
double calculate_weighted_distance(double distance, int crowded_rate, const char* travel_mode) {
    // 拥挤系数：1-5 对应 1.0-2.0 的权重
    double crowd_weight = 1.0 + (crowded_rate - 1) * 0.25;
    
    // 根据不同出行方式调整权重敏感性
    double sensitivity = 1.0;
    if (strcmp(travel_mode, "walking") == 0) {
        sensitivity = 1.2; // 步行对拥挤更敏感
    } else if (strcmp(travel_mode, "vehicle") == 0) {
        sensitivity = 0.8; // 车辆对拥挤相对不敏感
    }
    
    return distance * pow(crowd_weight, sensitivity);
}

// 构建导航图
int build_navigation_graph(MYSQL *conn, NavigationGraph *nav_graph, const char* travel_mode) {
    char query[512] = "SELECT r.start_id, r.end_id, r.route_distance, r.route_crowded_rate "
                      "FROM routes r ORDER BY r.route_id";
    
    if (mysql_query(conn, query)) {
        printf("Error building graph: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    int route_count = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        if (row[0] == NULL || row[1] == NULL || row[2] == NULL || row[3] == NULL) continue;
        
        int start_id = atoi(row[0]);
        int end_id = atoi(row[1]);
        double distance = atof(row[2]);
        int crowded_rate = atoi(row[3]);
        
        int start_idx = (start_id >= 0 && start_id < 10000) ? nav_graph->location_id_to_index[start_id] : -1;
        int end_idx = (end_id >= 0 && end_id < 10000) ? nav_graph->location_id_to_index[end_id] : -1;
        
        if (start_idx != -1 && end_idx != -1 && start_idx < MAX_LOCATIONS && end_idx < MAX_LOCATIONS) {
            double weighted_dist = calculate_weighted_distance(distance, crowded_rate, travel_mode);
            
            // 无向图，双向连接
            nav_graph->original_graph[start_idx][end_idx] = distance;
            nav_graph->original_graph[end_idx][start_idx] = distance;
            
            nav_graph->graph[start_idx][end_idx] = weighted_dist;
            nav_graph->graph[end_idx][start_idx] = weighted_dist;
            
            route_count++;
        }
    }
    
    nav_graph->route_count = route_count;
    mysql_free_result(result);
    printf("Built graph with %d routes for travel mode: %s\n", route_count, travel_mode);
    return route_count;
}

// Dijkstra算法寻找最短路径
int find_shortest_path(NavigationGraph *nav_graph, int start_id, int end_id, 
                      double *total_distance, double *total_time, int *path, int *path_length) {
    int start_idx = (start_id >= 0 && start_id < 10000) ? nav_graph->location_id_to_index[start_id] : -1;
    int end_idx = (end_id >= 0 && end_id < 10000) ? nav_graph->location_id_to_index[end_id] : -1;
    
    if (start_idx == -1 || end_idx == -1 || start_idx >= MAX_LOCATIONS || end_idx >= MAX_LOCATIONS) {
        printf("Error: Start or end location not found or invalid.\n");
        return -1;
    }
    
    int n = nav_graph->location_count;
    double dist[MAX_LOCATIONS];
    int visited[MAX_LOCATIONS];
    
    // 初始化
    for (int i = 0; i < n; i++) {
        dist[i] = INF;
        visited[i] = 0;
        nav_graph->prev[i] = -1;
    }
    
    dist[start_idx] = 0;
    
    for (int count = 0; count < n; count++) {
        // 找到未访问的最小距离节点
        int min_index = -1;
        double min_dist = INF;
        
        for (int v = 0; v < n; v++) {
            if (!visited[v] && dist[v] < min_dist) {
                min_dist = dist[v];
                min_index = v;
            }
        }
        
        if (min_index == -1 || min_index == end_idx) break;
        visited[min_index] = 1;
        
        // 更新相邻节点距离
        for (int v = 0; v < n; v++) {
            if (!visited[v] && nav_graph->graph[min_index][v] < INF - 1 &&
                dist[min_index] < INF - 1 &&
                dist[min_index] + nav_graph->graph[min_index][v] < dist[v]) {
                dist[v] = dist[min_index] + nav_graph->graph[min_index][v];
                nav_graph->prev[v] = min_index;
            }
        }
    }
    
    // 检查是否找到路径
    if (dist[end_idx] >= INF - 1) {
        printf("No path found between the locations.\n");
        return -1;
    }
    
    // 构建路径
    int current = end_idx;
    *path_length = 0;
    int temp_path[MAX_LOCATIONS];
    
    while (current != -1) {
        temp_path[(*path_length)++] = current;
        current = nav_graph->prev[current];
        if (*path_length >= MAX_LOCATIONS) break; // 防止无限循环
    }
    
    // 反转路径
    for (int i = 0; i < *path_length; i++) {
        path[i] = temp_path[*path_length - i - 1];
    }
    
    *total_distance = dist[end_idx];
    return 0;
}

// 打印路径详情
void print_path_details(NavigationGraph *nav_graph, int *path, int path_length, 
                       double total_distance, double total_time, const char* travel_mode) {
    printf("\n=== Route Planning Results ===\n");
    printf("Travel Mode: %s\n", travel_mode);
    printf("Total Distance: %.2f meters\n", total_distance);
    printf("Estimated Time: %.2f minutes\n", total_time);
    printf("Path Details:\n");
    printf("--------------------------------------------------\n");
    
    for (int i = 0; i < path_length; i++) {
        int loca_idx = path[i];
        if (loca_idx < 0 || loca_idx >= nav_graph->location_count) continue;
        
        int loca_id = nav_graph->index_to_location_id[loca_idx];
        printf("%d. %s (ID: %d)\n", i + 1, 
               nav_graph->locations[loca_idx].loca_name, loca_id);
        
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

int select_location_by_name(MYSQL *conn, const char* prompt) {
    char search_term[100];
    int max_attempts = 3;
    int attempts = 0;
    
    while (attempts < max_attempts) {
        printf("%s", prompt);
        safe_input(search_term, sizeof(search_term));
        
        if (strlen(search_term) == 0) {
            printf("Please enter a search term.\n");
            attempts++;
            continue;
        }
        
        // 使用LIKE进行模糊搜索（不区分大小写）
        char query[512];
        snprintf(query, sizeof(query),
                 "SELECT loca_id, loca_name, category FROM locations WHERE "
                 "LOWER(loca_name) LIKE LOWER('%%%s%%') OR "
                 "LOWER(category) LIKE LOWER('%%%s%%') "
                 "ORDER BY loca_name LIMIT 10",
                 search_term, search_term);
        
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
        char location_categories[10][20];
        
        while ((row = mysql_fetch_row(result)) && count < 10) {
            location_ids[count] = atoi(row[0]);
            strncpy(location_names[count], row[1], 49);
            location_names[count][49] = '\0';
            strncpy(location_categories[count], row[2] ? row[2] : "", 19);
            location_categories[count][19] = '\0';
            
            printf("%d. %s (%s)\n", count + 1, row[1], row[2] ? row[2] : "No category");
            count++;
        }
        
        if (count == 1) {
            // 只有一个结果，自动选择
            int selected_id = location_ids[0];
            printf("Automatically selected: %s (ID: %d)\n", location_names[0], selected_id);
            mysql_free_result(result);
            return selected_id;
        }
        
        // 多个结果，让用户选择
        printf("\nSelect a location (1-%d) or 0 to search again: ", count);
        char choice_input[10];
        safe_input(choice_input, sizeof(choice_input));
        
        int choice = atoi(choice_input);
        if (choice > 0 && choice <= count) {
            int selected_id = location_ids[choice - 1];
            printf("Selected: %s (ID: %d)\n", location_names[choice - 1], selected_id);
            mysql_free_result(result);
            return selected_id;
        } else if (choice == 0) {
            // 重新搜索
            mysql_free_result(result);
            attempts++;
            continue;
        } else {
            printf("Invalid selection.\n");
            mysql_free_result(result);
            attempts++;
        }
    }
    
    printf("Failed to select location after %d attempts.\n", max_attempts);
    return -1;
}



// 交互式选择出行方式
int select_travel_mode_interactive(MYSQL *conn, char* travel_mode, int mode_size) {
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
    
    int num_rows = mysql_num_rows(result);
    if (num_rows == 0) {
        printf("No travel modes available.\n");
        mysql_free_result(result);
        return -1;
    }
    
    printf("Available travel modes:\n");
    printf("ID\tName\t\tSpeed\n");
    printf("----------------------------\n");
    
    MYSQL_ROW row;
    int count = 0;
    int mode_ids[10];
    char mode_names[10][20];
    
    while ((row = mysql_fetch_row(result)) && count < 10) {
        mode_ids[count] = atoi(row[0]);
        strncpy(mode_names[count], row[1], 19);
        mode_names[count][19] = '\0';
        printf("%d. %s\t\t%s km/h\n", mode_ids[count], row[1], row[2]);
        count++;
    }
    
    mysql_free_result(result);
    
    int max_attempts = 3;
    int attempts = 0;
    
    while (attempts < max_attempts) {
        printf("\nSelect travel mode by ID: ");
        char input[10];
        safe_input(input, sizeof(input));
        
        int selected_id = atoi(input);
        
        // 验证选择的ID是否存在
        int valid = 0;
        for (int i = 0; i < count; i++) {
            if (mode_ids[i] == selected_id) {
                strncpy(travel_mode, mode_names[i], mode_size - 1);
                travel_mode[mode_size - 1] = '\0';
                valid = 1;
                break;
            }
        }
        
        if (valid) {
            printf("Selected travel mode: %s\n", travel_mode);
            return 0;
        } else {
            printf("Invalid travel mode ID. Please try again.\n");
            attempts++;
        }
    }
    
    printf("Failed to select travel mode after %d attempts.\n", max_attempts);
    return -1;
}



// 路径规划主处理函数
void handle_route_planning(MYSQL *conn) {
    printf("\n=== Route Planning ===\n");
    
    // 第一步：选择起点
    printf("\n--- Step 1: Select Starting Point ---\n");
    int start_id = select_location_by_name(conn, "Enter starting location name to search: ");
    if (start_id == -1) {
        printf("Failed to select starting point.\n");
        return;
    }
    
    // 第二步：选择终点
    printf("\n--- Step 2: Select Destination ---\n");
    int end_id = select_location_by_name(conn, "Enter destination location name to search: ");
    if (end_id == -1) {
        printf("Failed to select destination.\n");
        return;
    }
    
    // 第三步：选择出行方式
    printf("\n--- Step 3: Select Travel Mode ---\n");
    char travel_mode[20];
    if (select_travel_mode_interactive(conn, travel_mode, sizeof(travel_mode)) != 0) {
        printf("Failed to select travel mode.\n");
        return;
    }
    
    // 获取速度
    double speed = get_travel_speed(conn, travel_mode);
    printf("Using travel speed: %.2f km/h\n", speed);
    
    // 构建导航图
    NavigationGraph nav_graph;
    initialize_graph(&nav_graph);
    
    if (load_locations_from_db(conn, &nav_graph) <= 0) {
        printf("Failed to load locations. Please check if database has location data.\n");
        return;
    }
    
    if (build_navigation_graph(conn, &nav_graph, travel_mode) <= 0) {
        printf("Failed to build navigation graph. Please check if database has route data.\n");
        return;
    }
    
    // 寻找最短路径
    double total_distance, total_time;
    int path[MAX_LOCATIONS];
    int path_length = 0;
    
    if (find_shortest_path(&nav_graph, start_id, end_id, &total_distance, &total_time, path, &path_length) == 0) {
        // 计算时间（分钟）：距离(米) / 速度(米/分钟)
        total_time = total_distance / (speed * 1000 / 60);
        
        print_path_details(&nav_graph, path, path_length, total_distance, total_time, travel_mode);
    } else {
        printf("No path found between the selected locations.\n");
    }
}

#endif
