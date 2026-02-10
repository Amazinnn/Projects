#ifndef DATA_MANAGEMENT_H
#define DATA_MANAGEMENT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>

// 通用的模糊搜索函数
int search_and_select_id(MYSQL *conn, const char* table_name, const char* id_column, 
                         const char* name_column, const char* search_columns, 
                         const char* prompt, const char* display_columns) {
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
        
        // 构建模糊搜索查询
        char query[512];
        if (strcmp(table_name, "routes") == 0) {
            // 对于routes表，需要特殊处理，连接locations表显示名称
            snprintf(query, sizeof(query),
                     "SELECT r.%s, l1.loca_name as start_name, l2.loca_name as end_name, "
                     "r.route_distance, r.route_crowded_rate "
                     "FROM routes r "
                     "JOIN locations l1 ON r.start_id = l1.loca_id "
                     "JOIN locations l2 ON r.end_id = l2.loca_id "
                     "WHERE l1.loca_name LIKE '%%%s%%' OR l2.loca_name LIKE '%%%s%%' "
                     "OR r.%s = '%s' "
                     "ORDER BY r.%s LIMIT 10",
                     id_column, search_term, search_term, id_column, search_term, id_column);
        } else {
            // 对于其他表，使用通用搜索
            snprintf(query, sizeof(query),
                     "SELECT %s, %s FROM %s WHERE "
                     "(%s LIKE '%%%s%%' OR %s = '%s') "
                     "ORDER BY %s LIMIT 10",
                     id_column, display_columns, table_name, 
                     search_columns, search_term, id_column, search_term, id_column);
        }
        
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
            printf("No records found matching '%s'. Try again.\n", search_term);
            mysql_free_result(result);
            attempts++;
            continue;
        }
        
        printf("\nSearch results for '%s':\n", search_term);
        
        // 显示表头
        MYSQL_FIELD *fields = mysql_fetch_fields(result);
        int num_fields = mysql_num_fields(result);
        
        printf("ID\t");
        for (int i = 1; i < num_fields; i++) {
            printf("%s\t", fields[i].name);
        }
        printf("\n");
        printf("--------------------------------------------------\n");
        
        MYSQL_ROW row;
        int count = 0;
        int ids[10];
        char display_info[10][200]; // 存储显示信息
        
        while ((row = mysql_fetch_row(result)) && count < 10) {
            ids[count] = atoi(row[0]);
            
            // 构建显示信息
            char info[200] = "";
            for (int i = 1; i < num_fields; i++) {
                if (row[i] != NULL) {
                    strcat(info, row[i]);
                    if (i < num_fields - 1) strcat(info, " | ");
                }
            }
            strncpy(display_info[count], info, 199);
            display_info[count][199] = '\0';
            
            printf("%d. %d\t%s\n", count + 1, ids[count], display_info[count]);
            count++;
        }
        
        if (count == 1) {
            // 只有一个结果，自动选择
            int selected_id = ids[0];
            printf("Automatically selected: ID %d\n", selected_id);
            mysql_free_result(result);
            return selected_id;
        }
        
        // 多个结果，让用户选择
        printf("\nSelect a record (1-%d) or 0 to search again: ", count);
        char choice_input[10];
        safe_input(choice_input, sizeof(choice_input));
        
        int choice = atoi(choice_input);
        if (choice > 0 && choice <= count) {
            int selected_id = ids[choice - 1];
            printf("Selected: ID %d\n", selected_id);
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
    
    printf("Failed to select record after %d attempts.\n", max_attempts);
    return -1;
}

// 专门针对locations表的搜索函数
int search_and_select_location(MYSQL *conn, const char* prompt) {
    return search_and_select_id(conn, "locations", "loca_id", "loca_name", 
                               "loca_name, category", prompt, "loca_name, category, nearby_facilities");
}

// 专门针对routes表的搜索函数
int search_and_select_route(MYSQL *conn, const char* prompt) {
    return search_and_select_id(conn, "routes", "route_id", "route_id", 
                               "route_id", prompt, "start_id, end_id, route_distance, route_crowded_rate");
}

// 专门针对travel_modes表的搜索函数
int search_and_select_travel_mode(MYSQL *conn, const char* prompt) {
    return search_and_select_id(conn, "travel_modes", "model_id", "model_name", 
                               "model_name", prompt, "model_name, speed_kmh");
}



// 添加出行方式
int add_travel_mode(MYSQL *conn, const char *model_name, double speed_kmh) {
    if (conn == NULL) {
        printf("Error: Database connection is NULL\n");
        return -1;
    }
    
    if (model_name == NULL || strlen(model_name) == 0) {
        printf("Error: Model name is required.\n");
        return -1;
    }
    
    if (speed_kmh <= 0) {
        printf("Error: Speed must be positive.\n");
        return -1;
    }
    
    // 检查出行方式是否已存在
    char check_query[256];
    snprintf(check_query, sizeof(check_query),
             "SELECT COUNT(*) FROM travel_modes WHERE model_name = '%s'", model_name);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking travel mode: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int mode_count = atoi(row[0]);
    mysql_free_result(result);
    
    if (mode_count > 0) {
        printf("Error: Travel mode '%s' already exists.\n", model_name);
        return -1;
    }
    
    // 构建插入SQL
    char insert_query[256];
    snprintf(insert_query, sizeof(insert_query),
             "INSERT INTO travel_modes (model_name, speed_kmh) VALUES ('%s', %.2f)",
             model_name, speed_kmh);
    
    // 执行SQL
    if (mysql_query(conn, insert_query)) {
        printf("Error adding travel mode: %s\n", mysql_error(conn));
        return -1;
    }
    
    unsigned long mode_id = mysql_insert_id(conn);
    printf("Travel mode added successfully with ID: %lu\n", mode_id);
    
    return 0;
}

// 删除出行方式
int delete_travel_mode(MYSQL *conn, int mode_id) {
    if (conn == NULL) {
        printf("Error: Database connection is NULL\n");
        return -1;
    }
    
    // 检查出行方式是否存在
    char check_query[256];
    snprintf(check_query, sizeof(check_query),
             "SELECT model_name FROM travel_modes WHERE model_id = %d", mode_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking travel mode: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    if (mysql_num_rows(result) == 0) {
        printf("Error: Travel mode with ID %d does not exist.\n", mode_id);
        mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    char *model_name = row[0];
    mysql_free_result(result);
    
    // 删除出行方式
    char delete_query[256];
    snprintf(delete_query, sizeof(delete_query),
             "DELETE FROM travel_modes WHERE model_id = %d", mode_id);
    
    if (mysql_query(conn, delete_query)) {
        printf("Error deleting travel mode: %s\n", mysql_error(conn));
        return -1;
    }
    
    printf("Travel mode '%s' (ID: %d) deleted successfully.\n", model_name, mode_id);
    return 0;
}

// 更新出行方式速度
int update_travel_mode(MYSQL *conn, int mode_id, double new_speed_kmh) {
    if (conn == NULL) {
        printf("Error: Database connection is NULL\n");
        return -1;
    }
    
    if (new_speed_kmh <= 0) {
        printf("Error: Speed must be positive.\n");
        return -1;
    }
    
    // 检查出行方式是否存在
    char check_query[256];
    snprintf(check_query, sizeof(check_query),
             "SELECT model_name FROM travel_modes WHERE model_id = %d", mode_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking travel mode: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    if (mysql_num_rows(result) == 0) {
        printf("Error: Travel mode with ID %d does not exist.\n", mode_id);
        mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    char *model_name = row[0];
    mysql_free_result(result);
    
    // 更新速度
    char update_query[256];
    snprintf(update_query, sizeof(update_query),
             "UPDATE travel_modes SET speed_kmh = %.2f WHERE model_id = %d",
             new_speed_kmh, mode_id);
    
    if (mysql_query(conn, update_query)) {
        printf("Error updating travel mode: %s\n", mysql_error(conn));
        return -1;
    }
    
    printf("Travel mode '%s' (ID: %d) updated successfully. New speed: %.2f km/h\n", 
           model_name, mode_id, new_speed_kmh);
    return 0;
}

// 显示所有出行方式
void display_all_travel_modes(MYSQL *conn) {
    if (conn == NULL) return;
    
    char query[256] = "SELECT model_id, model_name, speed_kmh FROM travel_modes ORDER BY model_id";
    
    if (mysql_query(conn, query)) {
        printf("Error fetching travel modes: %s\n", mysql_error(conn));
        return;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return;
    }
    
    int num_rows = mysql_num_rows(result);
    if (num_rows == 0) {
        printf("No travel modes found in the database.\n");
        mysql_free_result(result);
        return;
    }
    
    printf("\n--- All Travel Modes (%d found) ---\n", num_rows);
    printf("ID\tName\t\t\tSpeed (km/h)\n");
    printf("--------------------------------------------\n");
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("%s\t%-20s\t%s\n", 
               row[0] ? row[0] : "NULL", 
               row[1] ? row[1] : "NULL", 
               row[2] ? row[2] : "NULL");
    }
    
    mysql_free_result(result);
}


// 添加地点
int add_location(MYSQL *conn, const char *name, const char *category, const char *facilities) {
    if (conn == NULL) {
        printf("Error: Database connection is NULL\n");
        return -1;
    }
    
    if (name == NULL || category == NULL || strlen(name) == 0 || strlen(category) == 0) {
        printf("Error: Name and category are required.\n");
        return -1;
    }
    
    // 检查地点是否已存在
    char check_query[256];
    snprintf(check_query, sizeof(check_query),
             "SELECT COUNT(*) FROM locations WHERE loca_name = '%s'", name);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking location: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int location_count = atoi(row[0]);
    mysql_free_result(result);
    
    if (location_count > 0) {
        printf("Error: Location with name '%s' already exists.\n", name);
        return -1;
    }
    
    // 构建插入SQL
    char insert_query[512];
    if (facilities != NULL && strlen(facilities) > 0) {
        snprintf(insert_query, sizeof(insert_query),
                 "INSERT INTO locations (loca_name, category, nearby_facilities) VALUES ('%s', '%s', '%s')",
                 name, category, facilities);
    } else {
        snprintf(insert_query, sizeof(insert_query),
                 "INSERT INTO locations (loca_name, category) VALUES ('%s', '%s')",
                 name, category);
    }
    
    // 执行SQL
    if (mysql_query(conn, insert_query)) {
        printf("Error adding location: %s\n", mysql_error(conn));
        return -1;
    }
    
    unsigned long location_id = mysql_insert_id(conn);
    printf("Location added successfully with ID: %lu\n", location_id);
    
    return 0;
}

// 删除地点
int delete_location(MYSQL *conn, int location_id) {
    if (conn == NULL) {
        printf("Error: Database connection is NULL\n");
        return -1;
    }
    
    // 检查地点是否存在
    char check_query[256];
    snprintf(check_query, sizeof(check_query),
             "SELECT loca_name FROM locations WHERE loca_id = %d", location_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking location: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    if (mysql_num_rows(result) == 0) {
        printf("Error: Location with ID %d does not exist.\n", location_id);
        mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    char *location_name = row[0];
    mysql_free_result(result);
    
    // 检查是否有相关路径
    snprintf(check_query, sizeof(check_query),
             "SELECT COUNT(*) FROM routes WHERE start_id = %d OR end_id = %d", 
             location_id, location_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking routes: %s\n", mysql_error(conn));
        return -1;
    }
    
    result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    row = mysql_fetch_row(result);
    int route_count = atoi(row[0]);
    mysql_free_result(result);
    
    if (route_count > 0) {
        printf("Warning: This location is connected to %d route(s).\n", route_count);
        printf("Deleting the location will also delete these routes.\n");
        printf("Do you want to continue? (y/n): ");
        
        char confirm;
        scanf("%c", &confirm);
        getchar();
        
        if (confirm != 'y' && confirm != 'Y') {
            printf("Deletion cancelled.\n");
            return -1;
        }
        
        // 删除相关路径
        char delete_routes_query[256];
        snprintf(delete_routes_query, sizeof(delete_routes_query),
                 "DELETE FROM routes WHERE start_id = %d OR end_id = %d", 
                 location_id, location_id);
        
        if (mysql_query(conn, delete_routes_query)) {
            printf("Error deleting related routes: %s\n", mysql_error(conn));
            return -1;
        }
        printf("Deleted %d related route(s).\n", route_count);
    }
    
    // 删除地点
    char delete_query[256];
    snprintf(delete_query, sizeof(delete_query),
             "DELETE FROM locations WHERE loca_id = %d", location_id);
    
    if (mysql_query(conn, delete_query)) {
        printf("Error deleting location: %s\n", mysql_error(conn));
        return -1;
    }
    
    printf("Location '%s' (ID: %d) deleted successfully.\n", location_name, location_id);
    return 0;
}


// 添加路径函数
int add_route(MYSQL *conn, int start_id, int end_id, int distance, int crowded_rate) {
    if (conn == NULL) {
        printf("Error: Database connection is NULL\n");
        return -1;
    }
    
    // 验证地点ID是否存在
    char check_query[512];
    snprintf(check_query, sizeof(check_query),
             "SELECT COUNT(*) FROM locations WHERE loca_id IN (%d, %d)", start_id, end_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking locations: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int location_count = atoi(row[0]);
    mysql_free_result(result);
    
    if (location_count != 2) {
        printf("Error: One or both location IDs do not exist.\n");
        return -1;
    }
    
    // 检查是否已存在相同路径
    snprintf(check_query, sizeof(check_query),
             "SELECT route_id FROM routes WHERE (start_id = %d AND end_id = %d) OR (start_id = %d AND end_id = %d)",
             start_id, end_id, end_id, start_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking existing routes: %s\n", mysql_error(conn));
        return -1;
    }
    
    result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    if (mysql_num_rows(result) > 0) {
        printf("Error: A route between these locations already exists.\n");
        mysql_free_result(result);
        return -1;
    }
    mysql_free_result(result);
    
    // 验证参数范围
    if (distance <= 0) {
        printf("Error: Distance must be positive.\n");
        return -1;
    }
    
    if (crowded_rate < 1 || crowded_rate > 5) {
        printf("Error: Crowded rate must be between 1 and 5.\n");
        return -1;
    }
    
    // 构建SQL插入语句
    char insert_query[512];
    snprintf(insert_query, sizeof(insert_query),
             "INSERT INTO routes (start_id, end_id, route_distance, route_crowded_rate) VALUES (%d, %d, %d, %d)",
             start_id, end_id, distance, crowded_rate);
    
    // 执行SQL语句
    if (mysql_query(conn, insert_query)) {
        printf("Error adding route: %s\n", mysql_error(conn));
        return -1;
    }
    
    // 获取插入的路径ID
    unsigned long route_id = mysql_insert_id(conn);
    printf("Route added successfully with ID: %lu\n", route_id);
    
    return 0;
}

// 删除路径函数（通过路径ID）
int delete_route_by_id(MYSQL *conn, int route_id) {
    if (conn == NULL) {
        printf("Error: Database connection is NULL\n");
        return -1;
    }
    
    // 首先检查路径是否存在
    char check_query[256];
    snprintf(check_query, sizeof(check_query),
             "SELECT r.route_id, l1.loca_name, l2.loca_name FROM routes r "
             "JOIN locations l1 ON r.start_id = l1.loca_id "
             "JOIN locations l2 ON r.end_id = l2.loca_id "
             "WHERE r.route_id = %d", route_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking route: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    if (mysql_num_rows(result) == 0) {
        printf("Error: Route with ID %d does not exist.\n", route_id);
        mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    char *start_name = row[1];
    char *end_name = row[2];
    mysql_free_result(result);
    
    // 删除路径
    char delete_query[256];
    snprintf(delete_query, sizeof(delete_query),
             "DELETE FROM routes WHERE route_id = %d", route_id);
    
    if (mysql_query(conn, delete_query)) {
        printf("Error deleting route: %s\n", mysql_error(conn));
        return -1;
    }
    
    printf("Route between '%s' and '%s' (ID: %d) deleted successfully.\n", 
           start_name, end_name, route_id);
    return 0;
}

// 删除路径函数（通过地点ID）
int delete_route_by_locations(MYSQL *conn, int start_id, int end_id) {
    if (conn == NULL) {
        printf("Error: Database connection is NULL\n");
        return -1;
    }
    
    // 首先检查路径是否存在
    char check_query[512];
    snprintf(check_query, sizeof(check_query),
             "SELECT r.route_id, l1.loca_name, l2.loca_name FROM routes r "
             "JOIN locations l1 ON r.start_id = l1.loca_id "
             "JOIN locations l2 ON r.end_id = l2.loca_id "
             "WHERE (r.start_id = %d AND r.end_id = %d) OR (r.start_id = %d AND r.end_id = %d)",
             start_id, end_id, end_id, start_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking route: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    if (mysql_num_rows(result) == 0) {
        printf("Error: No route found between these locations.\n");
        mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int route_id = atoi(row[0]);
    char *start_name = row[1];
    char *end_name = row[2];
    mysql_free_result(result);
    
    // 删除路径
    char delete_query[256];
    snprintf(delete_query, sizeof(delete_query),
             "DELETE FROM routes WHERE route_id = %d", route_id);
    
    if (mysql_query(conn, delete_query)) {
        printf("Error deleting route: %s\n", mysql_error(conn));
        return -1;
    }
    
    printf("Route between '%s' and '%s' (ID: %d) deleted successfully.\n", 
           start_name, end_name, route_id);
    return 0;
}

// 显示所有路径函数
void display_all_routes(MYSQL *conn) {
    if (conn == NULL) return;
    
    char query[512] = "SELECT r.route_id, r.start_id, l1.loca_name as start_name, "
                     "r.end_id, l2.loca_name as end_name, r.route_distance, r.route_crowded_rate "
                     "FROM routes r "
                     "JOIN locations l1 ON r.start_id = l1.loca_id "
                     "JOIN locations l2 ON r.end_id = l2.loca_id "
                     "ORDER BY r.route_id";
    
    if (mysql_query(conn, query)) {
        printf("Error fetching routes: %s\n", mysql_error(conn));
        return;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return;
    }
    
    int num_rows = mysql_num_rows(result);
    if (num_rows == 0) {
        printf("No routes found in the database.\n");
        mysql_free_result(result);
        return;
    }
    
    printf("\n--- All Routes (%d found) ---\n", num_rows);
    printf("ID\tStartID\tStartName\t\tEndID\tEndName\t\tDistance\tCrowdRate\n");
    printf("----------------------------------------------------------------------------\n");
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("%s\t%s\t%-15s\t%s\t%-15s\t%sm\t\t%s\n", 
               row[0], row[1], row[2], row[3], row[4], row[5], row[6]);
    }
    
    mysql_free_result(result);
}

// 显示所有地点函数
void display_all_locations(MYSQL *conn) {
    if (conn == NULL) return;
    
    char query[256] = "SELECT loca_id, loca_name, category, nearby_facilities FROM locations ORDER BY loca_id";
    
    if (mysql_query(conn, query)) {
        printf("Error fetching locations: %s\n", mysql_error(conn));
        return;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return;
    }
    
    int num_rows = mysql_num_rows(result);
    if (num_rows == 0) {
        printf("No locations found in the database.\n");
        mysql_free_result(result);
        return;
    }
    
    printf("\n--- All Locations (%d found) ---\n", num_rows);
    printf("ID\tName\t\t\tCategory\tFacilities\n");
    printf("------------------------------------------------------------\n");
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("%s\t%-20s\t%-10s\t%s\n", 
               row[0] ? row[0] : "NULL", 
               row[1] ? row[1] : "NULL", 
               row[2] ? row[2] : "NULL", 
               row[3] ? row[3] : "None");
    }
    
    mysql_free_result(result);
}


// 更新数据管理函数，集成路线管理功能
void handle_data_management(MYSQL *conn) {
    int choice;
    
    do {
        printf("\n--- Data Management ---\n");
        printf("1. Add Location\n");
        printf("2. Delete Location\n");
        printf("3. Add Route\n");
        printf("4. Delete Route\n");
        printf("5. View All Data\n");
        printf("6. Travel Modes Management\n");
        printf("0. Back to Main Menu\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();
        
        switch(choice) {
            case 1: {
                // 添加地点
                char name[40], category[20], facilities[200] = "";
                
                printf("Enter location name: ");
                safe_input(name, sizeof(name));
                
                printf("Enter category: ");
                safe_input(category, sizeof(category));
                
                printf("Enter nearby facilities (optional, press Enter to skip): ");
                safe_input(facilities, sizeof(facilities));
                
                if (strlen(name) == 0 || strlen(category) == 0) {
                    printf("Error: Name and category are required.\n");
                    break;
                }
                
                if (add_location(conn, name, category, 
                                strlen(facilities) > 0 ? facilities : NULL) == 0) {
                    printf("Location added successfully!\n");
                } else {
                    printf("Failed to add location.\n");
                }
                break;
            }
            case 2: {
                // 删除地点
                int loca_id;
                
                display_all_locations(conn);
                
                printf("Enter location ID to delete: ");
                if (scanf("%d", &loca_id) != 1) {
                    printf("Invalid location ID.\n");
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                
                printf("Are you sure you want to delete location ID %d? (y/n): ", loca_id);
                char confirm;
                scanf("%c", &confirm);
                getchar();
                
                if (confirm == 'y' || confirm == 'Y') {
                    if (delete_location(conn, loca_id) == 0) {
                        printf("Location deleted successfully!\n");
                    } else {
                        printf("Failed to delete location.\n");
                    }
                } else {
                    printf("Deletion cancelled.\n");
                }
                break;
            }
            case 3: {
                // 添加路径
                int start_id, end_id, distance, crowded_rate;
                
                display_all_locations(conn);
                
                printf("\n--- Add New Route ---\n");
                printf("Enter start location ID: ");
                if (scanf("%d", &start_id) != 1) {
                    printf("Invalid start ID.\n");
                    while (getchar() != '\n');
                    break;
                }
                
                printf("Enter end location ID: ");
                if (scanf("%d", &end_id) != 1) {
                    printf("Invalid end ID.\n");
                    while (getchar() != '\n');
                    break;
                }
                
                printf("Enter distance (meters): ");
                if (scanf("%d", &distance) != 1) {
                    printf("Invalid distance.\n");
                    while (getchar() != '\n');
                    break;
                }
                
                printf("Enter crowded rate (1-5, where 1 is least crowded): ");
                if (scanf("%d", &crowded_rate) != 1) {
                    printf("Invalid crowded rate.\n");
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                
                if (add_route(conn, start_id, end_id, distance, crowded_rate) == 0) {
                    printf("Route added successfully!\n");
                } else {
                    printf("Failed to add route.\n");
                }
                break;
            }
            case 4: {
                // 删除路径
                int delete_choice;
                
                printf("\n--- Delete Route ---\n");
                printf("1. Delete by Route ID\n");
                printf("2. Delete by Location IDs\n");
                printf("0. Cancel\n");
                printf("Enter your choice: ");
                
                if (scanf("%d", &delete_choice) != 1) {
                    printf("Invalid choice.\n");
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                
                if (delete_choice == 1) {
                    int route_id;
                    
                    display_all_routes(conn);
                    
                    printf("Enter route ID to delete: ");
                    if (scanf("%d", &route_id) != 1) {
                        printf("Invalid route ID.\n");
                        while (getchar() != '\n');
                        break;
                    }
                    getchar();
                    
                    printf("Are you sure you want to delete route ID %d? (y/n): ", route_id);
                    char confirm;
                    scanf("%c", &confirm);
                    getchar();
                    
                    if (confirm == 'y' || confirm == 'Y') {
                        if (delete_route_by_id(conn, route_id) == 0) {
                            printf("Route deleted successfully!\n");
                        } else {
                            printf("Failed to delete route.\n");
                        }
                    } else {
                        printf("Deletion cancelled.\n");
                    }
                } else if (delete_choice == 2) {
                    int start_id, end_id;
                    
                    display_all_locations(conn);
                    display_all_routes(conn);
                    
                    printf("Enter start location ID: ");
                    if (scanf("%d", &start_id) != 1) {
                        printf("Invalid start ID.\n");
                        while (getchar() != '\n');
                        break;
                    }
                    
                    printf("Enter end location ID: ");
                    if (scanf("%d", &end_id) != 1) {
                        printf("Invalid end ID.\n");
                        while (getchar() != '\n');
                        break;
                    }
                    getchar();
                    
                    printf("Are you sure you want to delete route between %d and %d? (y/n): ", start_id, end_id);
                    char confirm;
                    scanf("%c", &confirm);
                    getchar();
                    
                    if (confirm == 'y' || confirm == 'Y') {
                        if (delete_route_by_locations(conn, start_id, end_id) == 0) {
                            printf("Route deleted successfully!\n");
                        } else {
                            printf("Failed to delete route.\n");
                        }
                    } else {
                        printf("Deletion cancelled.\n");
                    }
                } else if (delete_choice == 0) {
                    printf("Deletion cancelled.\n");
                } else {
                    printf("Invalid choice.\n");
                }
                break;
            }
            case 5: {
                // 查看所有数据
                display_all_locations(conn);
                display_all_routes(conn);
                display_all_travel_modes(conn);
                break;
            }
            case 6: {
                // 出行方式管理子菜单
                int travel_choice;
                do {
                    printf("\n--- Travel Modes Management ---\n");
                    printf("1. Add Travel Mode\n");
                    printf("2. Delete Travel Mode\n");
                    printf("3. Update Travel Mode Speed\n");
                    printf("4. View All Travel Modes\n");
                    printf("0. Back to Data Management\n");
                    printf("Enter your choice: ");
                    
                    if (scanf("%d", &travel_choice) != 1) {
                        printf("Invalid input. Please enter a number.\n");
                        while (getchar() != '\n');
                        continue;
                    }
                    getchar();
                    
                    switch(travel_choice) {
                        case 1: {
                            // 添加出行方式
                            char model_name[20];
                            double speed;
                            
                            printf("Enter travel mode name: ");
                            safe_input(model_name, sizeof(model_name));
                            
                            printf("Enter speed (km/h): ");
                            if (scanf("%lf", &speed) != 1) {
                                printf("Invalid speed.\n");
                                while (getchar() != '\n');
                                break;
                            }
                            getchar();
                            
                            if (add_travel_mode(conn, model_name, speed) == 0) {
                                printf("Travel mode added successfully!\n");
                            } else {
                                printf("Failed to add travel mode.\n");
                            }
                            break;
                        }
                        case 2: {
                            // 删除出行方式
                            int mode_id;
                            
                            display_all_travel_modes(conn);
                            
                            printf("Enter travel mode ID to delete: ");
                            if (scanf("%d", &mode_id) != 1) {
                                printf("Invalid mode ID.\n");
                                while (getchar() != '\n');
                                break;
                            }
                            getchar();
                            
                            printf("Are you sure you want to delete travel mode ID %d? (y/n): ", mode_id);
                            char confirm;
                            scanf("%c", &confirm);
                            getchar();
                            
                            if (confirm == 'y' || confirm == 'Y') {
                                if (delete_travel_mode(conn, mode_id) == 0) {
                                    printf("Travel mode deleted successfully!\n");
                                } else {
                                    printf("Failed to delete travel mode.\n");
                                }
                            } else {
                                printf("Deletion cancelled.\n");
                            }
                            break;
                        }
                        case 3: {
                            // 更新出行方式速度
                            int mode_id;
                            double new_speed;
                            
                            display_all_travel_modes(conn);
                            
                            printf("Enter travel mode ID to update: ");
                            if (scanf("%d", &mode_id) != 1) {
                                printf("Invalid mode ID.\n");
                                while (getchar() != '\n');
                                break;
                            }
                            
                            printf("Enter new speed (km/h): ");
                            if (scanf("%lf", &new_speed) != 1) {
                                printf("Invalid speed.\n");
                                while (getchar() != '\n');
                                break;
                            }
                            getchar();
                            
                            if (update_travel_mode(conn, mode_id, new_speed) == 0) {
                                printf("Travel mode updated successfully!\n");
                            } else {
                                printf("Failed to update travel mode.\n");
                            }
                            break;
                        }
                        case 4: {
                            // 查看所有出行方式
                            display_all_travel_modes(conn);
                            break;
                        }
                        case 0:
                            printf("Returning to data management menu...\n");
                            break;
                        default:
                            printf("Invalid choice. Please try again.\n");
                    }
                } while (travel_choice != 0);
                break;
            }
            case 0:
                printf("Returning to main menu...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);
}



#endif
