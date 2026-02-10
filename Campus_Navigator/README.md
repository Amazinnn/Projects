# Campus Navigation System

## Overview

A comprehensive campus navigation system developed in C with MySQL integration that provides route planning, nearest facility search, and data management capabilities for campus environments.

## Features

### 🗺️ Route Planning

- Find optimal paths between campus locations
- Support for multiple travel modes (walking, cycling, vehicle)
- Real-time distance and time calculations
- Consideration of crowdedness factors

### 🔍 Nearest Facility Search

- Locate nearest facilities by category (library, cafeteria, etc.)
- BFS-based search algorithm for efficiency
- Multiple travel mode support
- Detailed path information with nearby facilities

### 📊 Data Management

- **Locations Management**: Add, delete, update, and search campus locations
- **Routes Management**: Manage paths between locations with distance and crowdedness data
- **Travel Modes Management**: Configure different transportation methods with custom speeds

## System Requirements

### Software Requirements

- MySQL Server 5.7+
- C Compiler (GCC recommended)
- MySQL C Connector

### Hardware Requirements

- Minimum 2GB RAM
- 100MB free disk space
- Network connectivity for database access

## Installation & Setup

### 1. Database Configuration

```mysql
CREATE DATABASE campus_navigation;
CREATE USER 'campus_nav_user'@'localhost' IDENTIFIED BY '@Superoad001';
GRANT ALL PRIVILEGES ON campus_navigation.* TO 'campus_nav_user'@'localhost';
FLUSH PRIVILEGES;
```

### 2. Database Tables

Execute the following SQL scripts to create required tables:

```mysql
-- Locations table
CREATE TABLE locations (
    loca_id INT AUTO_INCREMENT PRIMARY KEY,
    loca_name VARCHAR(40) NOT NULL UNIQUE,
    category VARCHAR(20),
    nearby_facilities VARCHAR(200)
);

-- Routes table  
CREATE TABLE routes (
    route_id INT AUTO_INCREMENT PRIMARY KEY,
    start_id INT NOT NULL,
    end_id INT NOT NULL,
    route_distance INT NOT NULL,
    route_crowded_rate INT CHECK (route_crowded_rate BETWEEN 1 AND 5),
    FOREIGN KEY (start_id) REFERENCES locations(loca_id),
    FOREIGN KEY (end_id) REFERENCES locations(loca_id)
);

-- Travel modes table
CREATE TABLE travel_modes (
    model_id INT AUTO_INCREMENT PRIMARY KEY,
    model_name VARCHAR(20) NOT NULL UNIQUE,
    speed_kmh DECIMAL(5,2) NOT NULL
);
```

### 3. Build Instructions

```bash
# Install MySQL connector
sudo apt-get install libmysqlclient-dev

# Compile the program
gcc -o campus_nav main.c -lmysqlclient -lm

# Run the application
./campus_nav
```

## Usage Guide

### Main Menu Options

1. **Route Planning**: Find paths between two locations
2. **Nearest Facility Search**: Locate closest facilities by category
3. **Data Management**: Administer location and route data

### Route Planning

1. Select starting location by name search
2. Choose destination location
3. Pick travel mode (walking, cycling, etc.)
4. View optimized route with distance and time estimates

### Nearest Facility Search

1. Enter your current location
2. Select facility category (library, cafeteria, etc.)
3. Choose travel mode
4. Get directions to nearest matching facility

### Data Management

- **Add Locations**: Register new campus spots with categories
- **Manage Routes**: Create and update paths between locations
- **Configure Travel Modes**: Set speeds for different transportation methods

## File Structure

```text
campus_navigator/
├── main.c                 # Main application entry point
├── database.h             # Database connection handling
├── data_management.h      # CRUD operations for locations/routes
├── route_planning.h       # Path finding algorithms
├── nearest_search.h       # Facility search functionality
└── README.md             # This file
```

## Technical Details

### Algorithms

- **Dijkstra's Algorithm**: For optimal route planning
- **Breadth-First Search**: For nearest facility discovery
- **Weighted Graph Processing**: Considering crowdedness and travel modes

### Database Integration

- Secure MySQL connections
- Parameterized queries to prevent SQL injection
- Efficient data retrieval and caching

## Troubleshooting

### Common Issues

1. **Database Connection Failed** Verify MySQL service is running Check credentials in `database.h` Ensure database and tables exist
2. **Compilation Errors** Install MySQL connector library Check GCC installation Verify header file paths
3. **Runtime Errors** Validate database schema matches expectations Check user permissions for database operations

## Support

For technical support or feature requests, please contact the development team.

------

# 校园导航系统

## 概述

一个基于C语言和MySQL开发的综合校园导航系统，提供路径规划、最近设施搜索和数据管理功能，专为校园环境设计。

## 功能特性

### 🗺️ 路径规划

- 查找校园地点之间的最优路径
- 支持多种出行方式（步行、骑行、车辆）
- 实时距离和时间计算
- 考虑拥挤度因素

### 🔍 最近设施搜索

- 按类别查找最近设施（图书馆、食堂等）
- 基于BFS的高效搜索算法
- 多种出行模式支持
- 详细的路径信息和周边设施

### 📊 数据管理

- **地点管理**：添加、删除、更新和搜索校园地点
- **路线管理**：管理地点间的路径，包含距离和拥挤度数据
- **出行方式管理**：配置不同交通方式的自定义速度

## 系统要求

### 软件要求

- MySQL服务器 5.7+
- C编译器（推荐GCC）
- MySQL C连接器

### 硬件要求

- 最低2GB内存
- 100MB可用磁盘空间
- 数据库访问的网络连接

## 安装与设置

### 1. 数据库配置

```mysql
CREATE DATABASE campus_navigation;
CREATE USER 'campus_nav_user'@'localhost' IDENTIFIED BY '@Superoad001';
GRANT ALL PRIVILEGES ON campus_navigation.* TO 'campus_nav_user'@'localhost';
FLUSH PRIVILEGES;
```

### 2. 数据库表结构

执行以下SQL脚本创建所需表：

```mysql
-- 地点表
CREATE TABLE locations (
    loca_id INT AUTO_INCREMENT PRIMARY KEY,
    loca_name VARCHAR(40) NOT NULL UNIQUE,
    category VARCHAR(20),
    nearby_facilities VARCHAR(200)
);

-- 路线表
CREATE TABLE routes (
    route_id INT AUTO_INCREMENT PRIMARY KEY,
    start_id INT NOT NULL,
    end_id INT NOT NULL,
    route_distance INT NOT NULL,
    route_crowded_rate INT CHECK (route_crowded_rate BETWEEN 1 AND 5),
    FOREIGN KEY (start_id) REFERENCES locations(loca_id),
    FOREIGN KEY (end_id) REFERENCES locations(loca_id)
);

-- 出行方式表
CREATE TABLE travel_modes (
    model_id INT AUTO_INCREMENT PRIMARY KEY,
    model_name VARCHAR(20) NOT NULL UNIQUE,
    speed_kmh DECIMAL(5,2) NOT NULL
);
```

### 3. 编译指南

```bash
# 安装MySQL连接器
sudo apt-get install libmysqlclient-dev

# 编译程序
gcc -o campus_nav main.c -lmysqlclient -lm

# 运行应用程序
./campus_nav
```

## 使用指南

### 主菜单选项

1. **路径规划**：查找两个地点之间的路径
2. **最近设施搜索**：按类别查找最近设施
3. **数据管理**：管理地点和路线数据

### 路径规划

1. 通过名称搜索选择起点
2. 选择目的地
3. 选择出行方式（步行、骑行等）
4. 查看优化路线及距离时间估算

### 最近设施搜索

1. 输入当前位置
2. 选择设施类别（图书馆、食堂等）
3. 选择出行方式
4. 获取到最近匹配设施的路线

### 数据管理

- **添加地点**：注册新的校园地点及类别
- **管理路线**：创建和更新地点间路径
- **配置出行方式**：设置不同交通方式的速度

## 文件结构

```
campus_navigator/
├── main.c                 # 主应用程序入口
├── database.h             # 数据库连接处理
├── data_management.h      # 地点/路线的CRUD操作
├── route_planning.h       # 路径查找算法
├── nearest_search.h       # 设施搜索功能
└── README.md             # 本文件
```

## 技术细节

### 算法

- **Dijkstra算法**：用于最优路径规划
- **广度优先搜索**：用于最近设施发现
- **加权图处理**：考虑拥挤度和出行方式

### 数据库集成

- 安全的MySQL连接
- 参数化查询防止SQL注入
- 高效的数据检索和缓存

## 故障排除

### 常见问题

1. **数据库连接失败** 验证MySQL服务是否运行 检查`database.h`中的凭据 确保数据库和表存在
2. **编译错误** 安装MySQL连接器库 检查GCC安装 验证头文件路径
3. **运行时错误** 验证数据库架构是否符合预期 检查用户对数据库操作的权限

## 技术支持

如需技术支持或功能请求，请联系开发团队。