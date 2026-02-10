#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>

// 数据库配置
#define DB_HOST "localhost"
#define DB_USER "campus_nav_user"
#define DB_PASS "@Superoad001"
#define DB_NAME "campus_navigation"

// 函数声明
MYSQL* connect_database();
void disconnect_database(MYSQL *conn);

// 函数实现
MYSQL* connect_database() {
    MYSQL *conn = mysql_init(NULL);
    
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return NULL;
    }
    
    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }
    
    printf("Connected to MySQL database: %s\n", DB_NAME);
    return conn;
}

void disconnect_database(MYSQL *conn) {
    if (conn) {
        mysql_close(conn);
        printf("Database connection closed.\n");
    }
}

#endif
