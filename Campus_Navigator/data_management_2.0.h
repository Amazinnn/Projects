#ifndef DATA_MANAGEMENT_H
#define DATA_MANAGEMENT_H

#include <stdio.h>
#include <string.h>
#include <mysql.h>
#include <stdlib.h>


// 根据路径ID删除路径
int delete_route_by_id(MYSQL *conn, int route_id) {
    if (conn == NULL) {
        printf("Error: Database connection is NULL\n");
        return -1;
    }
    
    // 首先检查路径是否存在
    char check_query[512];
    snprintf(check_query, sizeof(check_query),
             "SELECT r.route_id, l1.loca_name, l2.loca_name, r.route_distance, r.route_crowded_rate "
             "FROM routes r "
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
    
    // 获取路径信息用于日志记录
    MYSQL_ROW row = mysql_fetch_row(result);
    char *start_name = row[1];
    char *end_name = row[2];
    char *distance = row[3];
    char *crowded_rate = row[4];
    mysql_free_result(result);
    
    // 执行删除操作
    char delete_query[256];
    snprintf(delete_query, sizeof(delete_query),
             "DELETE FROM routes WHERE route_id = %d", route_id);
    
    if (mysql_query(conn, delete_query)) {
        printf("Error deleting route: %s\n", mysql_error(conn));
        return -1;
    }
    
    // 检查是否成功删除
    int affected_rows = mysql_affected_rows(conn);
    if (affected_rows > 0) {
        printf("Route between '%s' and '%s' (ID: %d) deleted successfully.\n", 
               start_name, end_name, route_id);
        return 0;
    } else {
        printf("No route was deleted. Route ID %d may not exist.\n", route_id);
        return -1;
    }
}

// 专门针对locations表的搜索函数
int search_and_select_location(MYSQL *conn, const char* prompt) {
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
        
        // 构建模糊搜索查询（不区分大小写）
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
        printf("ID\tName\t\tCategory\n");
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



int add_location(MYSQL *conn) {
    printf("\n=== Add New Location ===\n");
    
    char name[40], category[20], facilities[200] = "";
    
    printf("Enter location name: ");
    safe_input(name, sizeof(name));
    
    printf("Enter category: ");
    safe_input(category, sizeof(category));
    
    printf("Enter nearby facilities (optional, press Enter to skip): ");
    safe_input(facilities, sizeof(facilities));
    
    if (strlen(name) == 0 || strlen(category) == 0) {
        printf("Error: Name and category are required.\n");
        return -1;
    }
    
    // 检查地点是否已存在
    char check_query[256];
    snprintf(check_query, sizeof(check_query),
             "SELECT COUNT(*) FROM locations WHERE LOWER(loca_name) = LOWER('%s')", name);
    
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
    if (strlen(facilities) > 0) {
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


// 删除地点 - 使用search_and_select_location函数
int delete_location(MYSQL *conn) {
    printf("\n=== Delete Location ===\n");
    
    // 使用搜索函数让用户选择要删除的地点
    int loca_id = search_and_select_location(conn, "Enter location name to search for deletion: ");
    if (loca_id == -1) {
        printf("Failed to select location for deletion.\n");
        return -1;
    }
    
    // 获取地点信息用于确认
    char check_query[256];
    snprintf(check_query, sizeof(check_query),
             "SELECT loca_name, category FROM locations WHERE loca_id = %d", loca_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error getting location info: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL || mysql_num_rows(result) == 0) {
        printf("Error: Location not found.\n");
        if (result) mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    char *location_name = row[0];
    char *category = row[1];
    mysql_free_result(result);
    
    // 确认删除
    printf("Are you sure you want to delete location '%s' (Category: %s, ID: %d)? (y/n): ", 
           location_name, category, loca_id);
    
    char confirm;
    scanf("%c", &confirm);
    getchar();
    
    if (confirm != 'y' && confirm != 'Y') {
        printf("Deletion cancelled.\n");
        return -1;
    }
    
    // 检查是否有相关路径
    snprintf(check_query, sizeof(check_query),
             "SELECT route_id FROM routes WHERE start_id = %d OR end_id = %d", 
             loca_id, loca_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking routes: %s\n", mysql_error(conn));
        return -1;
    }
    
    result = mysql_store_result(conn);
    if (result == NULL) {
        printf("Error storing result: %s\n", mysql_error(conn));
        return -1;
    }
    
    int route_count = mysql_num_rows(result);
    if (route_count > 0) {
        printf("Warning: This location is connected to %d route(s).\n", route_count);
        printf("Deleting the location will also delete these routes.\n");
        printf("Do you want to continue? (y/n): ");
        
        char route_confirm;
        scanf("%c", &route_confirm);
        getchar();
        
        if (route_confirm != 'y' && route_confirm != 'Y') {
            printf("Deletion cancelled.\n");
            mysql_free_result(result);
            return -1;
        }
        
        // 删除相关路径
        MYSQL_ROW route_row;
        int deleted_routes = 0;
        while ((route_row = mysql_fetch_row(result))) {
            int route_id = atoi(route_row[0]);
            if (delete_route_by_id(conn, route_id) == 0) {
                deleted_routes++;
            }
        }
        printf("Deleted %d related route(s).\n", deleted_routes);
    }
    mysql_free_result(result);
    
    // 删除地点
    char delete_query[256];
    snprintf(delete_query, sizeof(delete_query),
             "DELETE FROM locations WHERE loca_id = %d", loca_id);
    
    if (mysql_query(conn, delete_query)) {
        printf("Error deleting location: %s\n", mysql_error(conn));
        return -1;
    }
    
    printf("Location '%s' (ID: %d) deleted successfully.\n", location_name, loca_id);
    return 0;
}

// 搜索地点 - 支持输入 "all" 显示所有地点
int search_location(MYSQL *conn) {
    printf("\n=== Search Location ===\n");
    
    char search_term[100];
    printf("Enter location name to search (or 'all' to list all locations): ");
    safe_input(search_term, sizeof(search_term));
    
    if (strlen(search_term) == 0) {
        printf("Please enter a search term.\n");
        return -1;
    }
    
    // 处理 "all" 特殊情况
    if (strcasecmp(search_term, "all") == 0) {
        // 显示所有地点
        char query[512];
        snprintf(query, sizeof(query),
                 "SELECT loca_id, loca_name, category, nearby_facilities FROM locations ORDER BY loca_id");
        
        if (mysql_query(conn, query)) {
            printf("Error fetching all locations: %s\n", mysql_error(conn));
            return -1;
        }
        
        MYSQL_RES *result = mysql_store_result(conn);
        if (result == NULL) {
            printf("Error storing result: %s\n", mysql_error(conn));
            return -1;
        }
        
        int num_rows = mysql_num_rows(result);
        if (num_rows == 0) {
            printf("No locations found in the database.\n");
            mysql_free_result(result);
            return -1;
        }
        
        printf("\n=== All Locations (%d found) ===\n", num_rows);
        printf("ID\tName\t\t\tCategory\tNearby Facilities\n");
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
        return 0;
    } else {
        // 使用搜索函数进行交互式搜索
        int loca_id = search_and_select_location(conn, "Enter location name to search: ");
        if (loca_id == -1) {
            printf("Failed to select location for viewing.\n");
            return -1;
        }
        
        // 获取地点的详细信息
        char query[512];
        snprintf(query, sizeof(query),
                 "SELECT loca_id, loca_name, category, nearby_facilities FROM locations WHERE loca_id = %d", 
                 loca_id);
        
        if (mysql_query(conn, query)) {
            printf("Error getting location details: %s\n", mysql_error(conn));
            return -1;
        }
        
        MYSQL_RES *result = mysql_store_result(conn);
        if (result == NULL || mysql_num_rows(result) == 0) {
            printf("Error: Location not found.\n");
            if (result) mysql_free_result(result);
            return -1;
        }
        
        MYSQL_ROW row = mysql_fetch_row(result);
        printf("\n=== Location Details ===\n");
        printf("ID: %s\n", row[0] ? row[0] : "NULL");
        printf("Name: %s\n", row[1] ? row[1] : "NULL");
        printf("Category: %s\n", row[2] ? row[2] : "NULL");
        printf("Nearby Facilities: %s\n", row[3] ? row[3] : "None");
        
        mysql_free_result(result);
        
        // 显示相关路径信息
        printf("\n=== Routes Connected to This Location ===\n");
        
        char route_query[512];
        snprintf(route_query, sizeof(route_query),
                 "SELECT r.route_id, l1.loca_name, l2.loca_name, r.route_distance, r.route_crowded_rate "
                 "FROM routes r "
                 "JOIN locations l1 ON r.start_id = l1.loca_id "
                 "JOIN locations l2 ON r.end_id = l2.loca_id "
                 "WHERE r.start_id = %d OR r.end_id = %d", loca_id, loca_id);
        
        if (mysql_query(conn, route_query) == 0) {
            MYSQL_RES *route_result = mysql_store_result(conn);
            if (route_result != NULL) {
                int route_count = mysql_num_rows(route_result);
                if (route_count > 0) {
                    printf("RouteID\tStart\t\tEnd\t\tDistance\tCrowdRate\n");
                    printf("------------------------------------------------------------\n");
                    
                    MYSQL_ROW route_row;
                    while ((route_row = mysql_fetch_row(route_result))) {
                        printf("%s\t%s\t%s\t%sm\t\t%s\n", 
                               route_row[0], route_row[1], route_row[2], 
                               route_row[3], route_row[4]);
                    }
                } else {
                    printf("No routes connected to this location.\n");
                }
                mysql_free_result(route_result);
            }
        } else {
            printf("Error fetching route information: %s\n", mysql_error(conn));
        }
        
        return 0;
    }
}

// 更新地点 - 使用search_and_select_location函数
int update_location(MYSQL *conn) {
    printf("\n=== Update Location ===\n");
    
    // 使用搜索函数让用户选择要更新的地点
    int loca_id = search_and_select_location(conn, "Enter location name to search for update: ");
    if (loca_id == -1) {
        printf("Failed to select location for update.\n");
        return -1;
    }
    
    // 获取当前地点信息
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT loca_name, category, nearby_facilities FROM locations WHERE loca_id = %d", loca_id);
    
    if (mysql_query(conn, query)) {
        printf("Error getting location info: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL || mysql_num_rows(result) == 0) {
        printf("Error: Location not found.\n");
        if (result) mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    char current_name[40], current_category[20], current_facilities[200];
    strncpy(current_name, row[0] ? row[0] : "", 39);
    current_name[39] = '\0';
    strncpy(current_category, row[1] ? row[1] : "", 19);
    current_category[19] = '\0';
    strncpy(current_facilities, row[2] ? row[2] : "", 199);
    current_facilities[199] = '\0';
    
    mysql_free_result(result);
    
    printf("\nCurrent location information:\n");
    printf("Name: %s\n", current_name);
    printf("Category: %s\n", current_category);
    printf("Nearby Facilities: %s\n", strlen(current_facilities) > 0 ? current_facilities : "None");
    
    // 获取更新信息
    char new_name[40], new_category[20], new_facilities[200];
    
    printf("\nEnter new location name (press Enter to keep current '%s'): ", current_name);
    safe_input(new_name, sizeof(new_name));
    if (strlen(new_name) == 0) {
        strcpy(new_name, current_name);
    }
    
    printf("Enter new category (press Enter to keep current '%s'): ", current_category);
    safe_input(new_category, sizeof(new_category));
    if (strlen(new_category) == 0) {
        strcpy(new_category, current_category);
    }
    
    printf("Enter new nearby facilities (press Enter to keep current): ");
    safe_input(new_facilities, sizeof(new_facilities));
    if (strlen(new_facilities) == 0) {
        strcpy(new_facilities, current_facilities);
    }
    
    // 检查名称是否与其他地点冲突（排除自身）
    if (strcasecmp(new_name, current_name) != 0) {
        char check_query[256];
        snprintf(check_query, sizeof(check_query),
                 "SELECT COUNT(*) FROM locations WHERE LOWER(loca_name) = LOWER('%s') AND loca_id != %d", 
                 new_name, loca_id);
        
        if (mysql_query(conn, check_query)) {
            printf("Error checking location name: %s\n", mysql_error(conn));
            return -1;
        }
        
        result = mysql_store_result(conn);
        if (result == NULL) {
            printf("Error storing result: %s\n", mysql_error(conn));
            return -1;
        }
        
        row = mysql_fetch_row(result);
        int name_count = atoi(row[0]);
        mysql_free_result(result);
        
        if (name_count > 0) {
            printf("Error: Location with name '%s' already exists.\n", new_name);
            return -1;
        }
    }
    
    // 构建更新SQL
    char update_query[512];
    snprintf(update_query, sizeof(update_query),
             "UPDATE locations SET loca_name = '%s', category = '%s', nearby_facilities = '%s' WHERE loca_id = %d",
             new_name, new_category, new_facilities, loca_id);
    
    // 执行更新
    if (mysql_query(conn, update_query)) {
        printf("Error updating location: %s\n", mysql_error(conn));
        return -1;
    }
    
    printf("Location updated successfully!\n");
    printf("New information:\n");
    printf("Name: %s\n", new_name);
    printf("Category: %s\n", new_category);
    printf("Nearby Facilities: %s\n", strlen(new_facilities) > 0 ? new_facilities : "None");
    
    return 0;
}



// 专门针对routes表的搜索函数
int search_and_select_route(MYSQL *conn, const char* prompt) {
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
        snprintf(query, sizeof(query),
                 "SELECT r.route_id, l1.loca_name, l2.loca_name, r.route_distance, r.route_crowded_rate "
                 "FROM routes r "
                 "JOIN locations l1 ON r.start_id = l1.loca_id "
                 "JOIN locations l2 ON r.end_id = l2.loca_id "
                 "WHERE LOWER(l1.loca_name) LIKE LOWER('%%%s%%') OR "
                 "LOWER(l2.loca_name) LIKE LOWER('%%%s%%') "
                 "ORDER BY r.route_id LIMIT 10",
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
            printf("No routes found matching '%s'. Try again.\n", search_term);
            mysql_free_result(result);
            attempts++;
            continue;
        }
        
        printf("\nSearch results for '%s':\n", search_term);
        printf("RouteID\tStart\t\tEnd\t\tDistance\tCrowdRate\n");
        printf("------------------------------------------------------------\n");
        
        MYSQL_ROW row;
        int count = 0;
        int route_ids[10];
        char start_names[10][50];
        char end_names[10][50];
        int distances[10];
        int crowded_rates[10];
        
        while ((row = mysql_fetch_row(result)) && count < 10) {
            route_ids[count] = atoi(row[0]);
            strncpy(start_names[count], row[1], 49);
            start_names[count][49] = '\0';
            strncpy(end_names[count], row[2], 49);
            end_names[count][49] = '\0';
            distances[count] = atoi(row[3]);
            crowded_rates[count] = atoi(row[4]);
            
            printf("%d. %s -> %s (%dm, crowded: %d)\n", 
                   count + 1, row[1], row[2], distances[count], crowded_rates[count]);
            count++;
        }
        
        if (count == 1) {
            // 只有一个结果，自动选择
            int selected_id = route_ids[0];
            printf("Automatically selected: Route %d\n", selected_id);
            mysql_free_result(result);
            return selected_id;
        }
        
        // 多个结果，让用户选择
        printf("\nSelect a route (1-%d) or 0 to search again: ", count);
        char choice_input[10];
        safe_input(choice_input, sizeof(choice_input));
        
        int choice = atoi(choice_input);
        if (choice > 0 && choice <= count) {
            int selected_id = route_ids[choice - 1];
            printf("Selected: Route %d (%s -> %s)\n", 
                   selected_id, start_names[choice - 1], end_names[choice - 1]);
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
    
    printf("Failed to select route after %d attempts.\n", max_attempts);
    return -1;
}


// 添加路径
int add_route(MYSQL *conn) {
    printf("\n=== Add New Route ===\n");
    
    // 选择起点
    printf("Select starting location:\n");
    int start_id = search_and_select_location(conn, "Enter start location name to search: ");
    if (start_id == -1) {
        printf("Failed to select start location.\n");
        return -1;
    }
    
    // 选择终点
    printf("Select destination location:\n");
    int end_id = search_and_select_location(conn, "Enter destination location name to search: ");
    if (end_id == -1) {
        printf("Failed to select destination location.\n");
        return -1;
    }
    
    // 检查起点和终点是否相同
    if (start_id == end_id) {
        printf("Error: Start and end locations cannot be the same.\n");
        return -1;
    }
    
    // 检查路径是否已存在
    char check_query[512];
    snprintf(check_query, sizeof(check_query),
             "SELECT route_id FROM routes WHERE "
             "(start_id = %d AND end_id = %d) OR (start_id = %d AND end_id = %d)",
             start_id, end_id, end_id, start_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error checking existing routes: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
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
    
    // 获取路径信息
    int distance, crowded_rate;
    printf("Enter distance (meters): ");
    if (scanf("%d", &distance) != 1 || distance <= 0) {
        printf("Invalid distance. Must be a positive number.\n");
        while (getchar() != '\n');
        return -1;
    }
    
    printf("Enter crowded rate (1-5, where 1 is least crowded): ");
    if (scanf("%d", &crowded_rate) != 1 || crowded_rate < 1 || crowded_rate > 5) {
        printf("Invalid crowded rate. Must be between 1 and 5.\n");
        while (getchar() != '\n');
        return -1;
    }
    getchar(); // 清除换行符
    
    // 获取地点名称用于显示
    char start_name[50], end_name[50];
    snprintf(check_query, sizeof(check_query),
             "SELECT l1.loca_name, l2.loca_name FROM locations l1, locations l2 "
             "WHERE l1.loca_id = %d AND l2.loca_id = %d", start_id, end_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error getting location names: %s\n", mysql_error(conn));
        return -1;
    }
    
    result = mysql_store_result(conn);
    if (result == NULL || mysql_num_rows(result) == 0) {
        printf("Error: Location not found.\n");
        if (result) mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    strncpy(start_name, row[0], 49);
    start_name[49] = '\0';
    strncpy(end_name, row[1], 49);
    end_name[49] = '\0';
    mysql_free_result(result);
    
    // 确认添加
    printf("\nRoute to be added:\n");
    printf("From: %s (ID: %d)\n", start_name, start_id);
    printf("To: %s (ID: %d)\n", end_name, end_id);
    printf("Distance: %d meters\n", distance);
    printf("Crowded Rate: %d\n", crowded_rate);
    printf("Add this route? (y/n): ");
    
    char confirm;
    scanf("%c", &confirm);
    getchar();
    
    if (confirm != 'y' && confirm != 'Y') {
        printf("Route addition cancelled.\n");
        return -1;
    }
    
    // 插入新路径
    char insert_query[512];
    snprintf(insert_query, sizeof(insert_query),
             "INSERT INTO routes (start_id, end_id, route_distance, route_crowded_rate) "
             "VALUES (%d, %d, %d, %d)", start_id, end_id, distance, crowded_rate);
    
    if (mysql_query(conn, insert_query)) {
        printf("Error adding route: %s\n", mysql_error(conn));
        return -1;
    }
    
    unsigned long route_id = mysql_insert_id(conn);
    printf("Route added successfully with ID: %lu\n", route_id);
    
    return 0;
}

// 删除路径函数 - 包含 delete_route_by_id 调用
int delete_route(MYSQL *conn) {
    printf("\n=== Delete Route ===\n");
    
    // 使用搜索函数让用户选择要删除的路径
    int route_id = search_and_select_route(conn, "Enter route information to search for deletion: ");
    if (route_id == -1) {
        printf("Failed to select route for deletion.\n");
        return -1;
    }
    
    // 获取路径详情用于确认
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT r.route_id, l1.loca_name, l2.loca_name, r.route_distance, r.route_crowded_rate "
             "FROM routes r "
             "JOIN locations l1 ON r.start_id = l1.loca_id "
             "JOIN locations l2 ON r.end_id = l2.loca_id "
             "WHERE r.route_id = %d", route_id);
    
    if (mysql_query(conn, query)) {
        printf("Error getting route info: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL || mysql_num_rows(result) == 0) {
        printf("Error: Route not found.\n");
        if (result) mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    char *start_name = row[1];
    char *end_name = row[2];
    char *distance = row[3];
    char *crowded_rate = row[4];
    mysql_free_result(result);
    
    // 确认删除
    printf("Are you sure you want to delete this route?\n");
    printf("From: %s to %s\n", start_name, end_name);
    printf("Distance: %s meters, Crowded Rate: %s\n", distance, crowded_rate);
    printf("Confirm deletion? (y/n): ");
    
    char confirm;
    scanf("%c", &confirm);
    getchar();
    
    if (confirm != 'y' && confirm != 'Y') {
        printf("Deletion cancelled.\n");
        return -1;
    }
    
    // 调用 delete_route_by_id 函数执行删除
    if (delete_route_by_id(conn, route_id) == 0) {
        printf("Route deleted successfully!\n");
        return 0;
    } else {
        printf("Failed to delete route.\n");
        return -1;
    }
}


// 搜索路径 - 支持输入 "all" 显示所有路径
int search_route(MYSQL *conn) {
    printf("\n=== Search Route ===\n");
    
    char search_term[100];
    printf("Enter location name to search routes (or 'all' to list all routes): ");
    safe_input(search_term, sizeof(search_term));
    
    if (strlen(search_term) == 0) {
        printf("Please enter a search term.\n");
        return -1;
    }
    
    // 处理 "all" 特殊情况
    if (strcasecmp(search_term, "all") == 0) {
        // 显示所有路径
        char query[1024];
        snprintf(query, sizeof(query),
                 "SELECT r.route_id, l1.loca_id, l1.loca_name, l2.loca_id, l2.loca_name, "
                 "r.route_distance, r.route_crowded_rate "
                 "FROM routes r "
                 "JOIN locations l1 ON r.start_id = l1.loca_id "
                 "JOIN locations l2 ON r.end_id = l2.loca_id "
                 "ORDER BY r.route_id");
        
        if (mysql_query(conn, query)) {
            printf("Search error: %s\n", mysql_error(conn));
            return -1;
        }
        
        MYSQL_RES *result = mysql_store_result(conn);
        if (result == NULL) {
            printf("Error storing result: %s\n", mysql_error(conn));
            return -1;
        }
        
        int num_rows = mysql_num_rows(result);
        if (num_rows == 0) {
            printf("No routes found in the database.\n");
            mysql_free_result(result);
            return -1;
        }
        
        printf("\n=== All Routes (%d found) ===\n", num_rows);
        printf("RouteID\tStartID\tStartName\t\tEndID\tEndName\t\tDistance\tCrowdRate\n");
        printf("----------------------------------------------------------------------------\n");
        
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            printf("%s\t%s\t%-15s\t%s\t%-15s\t%sm\t\t%s\n", 
                   row[0] ? row[0] : "NULL", 
                   row[1] ? row[1] : "NULL", 
                   row[2] ? row[2] : "NULL", 
                   row[3] ? row[3] : "NULL", 
                   row[4] ? row[4] : "NULL", 
                   row[5] ? row[5] : "NULL", 
                   row[6] ? row[6] : "NULL");
        }
        
        mysql_free_result(result);
        return 0;
    } else {
        // 使用搜索函数进行交互式搜索
        int route_id = search_and_select_route(conn, "Enter route information to search: ");
        if (route_id == -1) {
            printf("Failed to select route for viewing.\n");
            return -1;
        }
        
        // 获取路径的详细信息
        char query[512];
        snprintf(query, sizeof(query),
                 "SELECT r.route_id, l1.loca_name, l2.loca_name, r.route_distance, r.route_crowded_rate "
                 "FROM routes r "
                 "JOIN locations l1 ON r.start_id = l1.loca_id "
                 "JOIN locations l2 ON r.end_id = l2.loca_id "
                 "WHERE r.route_id = %d", route_id);
        
        if (mysql_query(conn, query)) {
            printf("Error getting route details: %s\n", mysql_error(conn));
            return -1;
        }
        
        MYSQL_RES *result = mysql_store_result(conn);
        if (result == NULL || mysql_num_rows(result) == 0) {
            printf("Error: Route not found.\n");
            if (result) mysql_free_result(result);
            return -1;
        }
        
        MYSQL_ROW row = mysql_fetch_row(result);
        printf("\n=== Route Details ===\n");
        printf("Route ID: %s\n", row[0] ? row[0] : "NULL");
        printf("From: %s\n", row[1] ? row[1] : "NULL");
        printf("To: %s\n", row[2] ? row[2] : "NULL");
        printf("Distance: %s meters\n", row[3] ? row[3] : "NULL");
        printf("Crowded Rate: %s\n", row[4] ? row[4] : "NULL");
        
        mysql_free_result(result);
        return 0;
    }
}

// 更新路径
int update_route(MYSQL *conn) {
    printf("\n=== Update Route ===\n");
    
    // 使用搜索函数让用户选择要更新的路径
    int route_id = search_and_select_route(conn, "Enter route information to search for update: ");
    if (route_id == -1) {
        printf("Failed to select route for update.\n");
        return -1;
    }
    
    // 获取当前路径信息
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT r.start_id, l1.loca_name, r.end_id, l2.loca_name, "
             "r.route_distance, r.route_crowded_rate "
             "FROM routes r "
             "JOIN locations l1 ON r.start_id = l1.loca_id "
             "JOIN locations l2 ON r.end_id = l2.loca_id "
             "WHERE r.route_id = %d", route_id);
    
    if (mysql_query(conn, query)) {
        printf("Error getting route info: %s\n", mysql_error(conn));
        return -1;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL || mysql_num_rows(result) == 0) {
        printf("Error: Route not found.\n");
        if (result) mysql_free_result(result);
        return -1;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int current_start_id = atoi(row[0]);
    char *current_start_name = row[1];
    int current_end_id = atoi(row[2]);
    char *current_end_name = row[3];
    int current_distance = atoi(row[4]);
    int current_crowded_rate = atoi(row[5]);
    mysql_free_result(result);
    
    printf("\nCurrent route information:\n");
    printf("From: %s (ID: %d)\n", current_start_name, current_start_id);
    printf("To: %s (ID: %d)\n", current_end_name, current_end_id);
    printf("Distance: %d meters\n", current_distance);
    printf("Crowded Rate: %d\n", current_crowded_rate);
    
    // 获取更新信息
    printf("\nEnter new route information (press Enter to keep current value):\n");
    
    // 选择新的起点
    printf("Select new start location (current: %s):\n", current_start_name);
    int new_start_id = -1;
    char start_choice[10];
    printf("Keep current start location? (y/n): ");
    safe_input(start_choice, sizeof(start_choice));
    
    if (strcasecmp(start_choice, "y") == 0 || strcasecmp(start_choice, "yes") == 0) {
        new_start_id = current_start_id;
    } else {
        new_start_id = search_and_select_location(conn, "Enter new start location: ");
        if (new_start_id == -1) {
            printf("Failed to select new start location.\n");
            return -1;
        }
    }
    
    // 选择新的终点
    printf("Select new end location (current: %s):\n", current_end_name);
    int new_end_id = -1;
    char end_choice[10];
    printf("Keep current end location? (y/n): ");
    safe_input(end_choice, sizeof(end_choice));
    
    if (strcasecmp(end_choice, "y") == 0 || strcasecmp(end_choice, "yes") == 0) {
        new_end_id = current_end_id;
    } else {
        new_end_id = search_and_select_location(conn, "Enter new end location: ");
        if (new_end_id == -1) {
            printf("Failed to select new end location.\n");
            return -1;
        }
    }
    
    // 检查起点和终点是否相同
    if (new_start_id == new_end_id) {
        printf("Error: Start and end locations cannot be the same.\n");
        return -1;
    }
    
    // 检查新路径是否已存在（排除当前路径）
    char check_query[512];
    snprintf(check_query, sizeof(check_query),
             "SELECT route_id FROM routes WHERE "
             "((start_id = %d AND end_id = %d) OR (start_id = %d AND end_id = %d)) "
             "AND route_id != %d",
             new_start_id, new_end_id, new_end_id, new_start_id, route_id);
    
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
    
    // 获取新的距离和拥挤度
    int new_distance, new_crowded_rate;
    char distance_input[20], crowded_input[20];
    
    printf("Enter new distance (current: %d meters): ", current_distance);
    safe_input(distance_input, sizeof(distance_input));
    if (strlen(distance_input) == 0) {
        new_distance = current_distance;
    } else {
        new_distance = atoi(distance_input);
        if (new_distance <= 0) {
            printf("Invalid distance. Must be a positive number.\n");
            return -1;
        }
    }
    
    printf("Enter new crowded rate 1-5 (current: %d): ", current_crowded_rate);
    safe_input(crowded_input, sizeof(crowded_input));
    if (strlen(crowded_input) == 0) {
        new_crowded_rate = current_crowded_rate;
    } else {
        new_crowded_rate = atoi(crowded_input);
        if (new_crowded_rate < 1 || new_crowded_rate > 5) {
            printf("Invalid crowded rate. Must be between 1 and 5.\n");
            return -1;
        }
    }
    
    // 获取新的地点名称用于显示
    char new_start_name[50], new_end_name[50];
    snprintf(check_query, sizeof(check_query),
             "SELECT l1.loca_name, l2.loca_name FROM locations l1, locations l2 "
             "WHERE l1.loca_id = %d AND l2.loca_id = %d", new_start_id, new_end_id);
    
    if (mysql_query(conn, check_query)) {
        printf("Error getting location names: %s\n", mysql_error(conn));
        return -1;
    }
    
    result = mysql_store_result(conn);
    if (result == NULL || mysql_num_rows(result) == 0) {
        printf("Error: Location not found.\n");
        if (result) mysql_free_result(result);
        return -1;
    }
    
    row = mysql_fetch_row(result);
    strncpy(new_start_name, row[0], 49);
    new_start_name[49] = '\0';
    strncpy(new_end_name, row[1], 49);
    new_end_name[49] = '\0';
    mysql_free_result(result);
    
    // 确认更新
    printf("\nRoute to be updated:\n");
    printf("From: %s (ID: %d)\n", new_start_name, new_start_id);
    printf("To: %s (ID: %d)\n", new_end_name, new_end_id);
    printf("Distance: %d meters\n", new_distance);
    printf("Crowded Rate: %d\n", new_crowded_rate);
    printf("Update this route? (y/n): ");
    
    char confirm;
    scanf("%c", &confirm);
    getchar();
    
    if (confirm != 'y' && confirm != 'Y') {
        printf("Route update cancelled.\n");
        return -1;
    }
    
    // 更新路径
    char update_query[512];
    snprintf(update_query, sizeof(update_query),
             "UPDATE routes SET start_id = %d, end_id = %d, "
             "route_distance = %d, route_crowded_rate = %d "
             "WHERE route_id = %d",
             new_start_id, new_end_id, new_distance, new_crowded_rate, route_id);
    
    if (mysql_query(conn, update_query)) {
        printf("Error updating route: %s\n", mysql_error(conn));
        return -1;
    }
    
    printf("Route updated successfully!\n");
    return 0;
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


// 数据管理主函数
// 数据管理主函数
void handle_data_management(MYSQL *conn) {
    int choice;
    
    do {
        printf("\n=== Data Management System ===\n");
        printf("1. Locations Management\n");
        printf("2. Routes Management\n");
        printf("3. Travel Modes Management\n");
        printf("0. Back to Main Menu\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // 清除输入缓冲区
            continue;
        }
        getchar(); // 清除换行符
        
        switch(choice) {
            case 1:
                handle_locations_management(conn);
                break;
            case 2:
                handle_routes_management(conn);
                break;
            case 3:
                handle_travel_modes_management(conn);
                break;
            case 0:
                printf("Returning to main menu...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);
}

// 地点管理子菜单
void handle_locations_management(MYSQL *conn) {
    int choice;
    
    do {
        printf("\n--- Locations Management ---\n");
        printf("1. Add New Location\n");
        printf("2. Delete Location\n");
        printf("3. Search Location (or 'all' to list all)\n");
        printf("4. Update Location\n");
        printf("0. Back to Data Management\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();
        
        switch(choice) {
            case 1:
                if (add_location(conn) == 0) {
                    printf("Location added successfully!\n");
                } else {
                    printf("Failed to add location.\n");
                }
                break;
            case 2:
                if (delete_location(conn) == 0) {
                    printf("Location deleted successfully!\n");
                } else {
                    printf("Failed to delete location.\n");
                }
                break;
            case 3:
                if (search_location(conn) == 0) {
                    printf("Location search completed.\n");
                } else {
                    printf("Location search failed.\n");
                }
                break;
            case 4:
                if (update_location(conn) == 0) {
                    printf("Location updated successfully!\n");
                } else {
                    printf("Failed to update location.\n");
                }
                break;
            case 0:
                printf("Returning to data management...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);
}

// 路径管理子菜单
void handle_routes_management(MYSQL *conn) {
    int choice;
    
    do {
        printf("\n--- Routes Management ---\n");
        printf("1. Add New Route\n");
        printf("2. Delete Route\n");
        printf("3. Search Route (or 'all' to list all)\n");
        printf("4. Update Route\n");
        printf("0. Back to Data Management\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();
        
        switch(choice) {
            case 1:
                if (add_route(conn) == 0) {
                    printf("Route added successfully!\n");
                } else {
                    printf("Failed to add route.\n");
                }
                break;
            case 2:
                if (delete_route(conn) == 0) {
                    printf("Route deleted successfully!\n");
                } else {
                    printf("Failed to delete route.\n");
                }
                break;
            case 3:
                if (search_route(conn) == 0) {
                    printf("Route search completed.\n");
                } else {
                    printf("Route search failed.\n");
                }
                break;
            case 4:
                if (update_route(conn) == 0) {
                    printf("Route updated successfully!\n");
                } else {
                    printf("Failed to update route.\n");
                }
                break;
            case 0:
                printf("Returning to data management...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);
}

// 出行方式管理子菜单
void handle_travel_modes_management(MYSQL *conn) {
    int choice;
    
    do {
        printf("\n--- Travel Modes Management ---\n");
        printf("1. Add Travel Mode\n");
        printf("2. Delete Travel Mode\n");
        printf("3. Update Travel Mode Speed\n");
        printf("4. View All Travel Modes\n"); // 保留单独查看功能
        printf("0. Back to Data Management\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();
        
        switch(choice) {
            case 1: {
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
            case 4:
                display_all_travel_modes(conn);
                break;
            case 0:
                printf("Returning to data management...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);
}



#endif

