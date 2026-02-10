# Five In A Row (Gomoku) Game

## Introduction

This is a Gomoku (Five in a Row) game developed in C++ with the EasyX graphics library. It features an intelligent AI opponent implemented using the Minimax algorithm with Alpha-Beta pruning.

## Features

- 🎯 Standard 15×15 game board
- 🤖 Intelligent AI opponent (Minimax algorithm)
- 🎨 Graphical user interface (EasyX)
- ⚡ Alpha-Beta pruning optimization
- 🎮 Support for choosing to play first or second

## System Requirements

- Windows operating system
- EasyX graphics library
- C++ compiler

## Game Setup

After launching the game, you can choose:

- Piece color: Black (b) / White (w)
- Playing order: First move (Y) / Second move (N)

## Game Rules

- Players take turns placing stones
- Win by forming five stones in a row horizontally, vertically, or diagonally
- AI uses a search depth of 7 levels for decision making

## Technical Features

- **AI Algorithm**: Minimax with Alpha-Beta pruning
- **Evaluation Function**: Considers multiple patterns including live fours, blocked fours, and live threes
- **Optimization Strategy**: Only searches positions adjacent to existing pieces

## Controls

- **Mouse Click**: Place a stone at the intersection point on the board
- **Automatic Judgment**: The game automatically detects win/lose conditions

---

# Five In A Row 五子棋游戏 #

## 简介

基于C++和EasyX图形库开发的五子棋人机对战游戏，采用Minimax算法和Alpha-Beta剪枝实现智能AI。

## 功能特性

- 🎯 15×15标准棋盘
- 🤖 智能AI对手（Minimax算法）
- 🎨 图形化界面（EasyX）
- ⚡ Alpha-Beta剪枝优化
- 🎮 支持先手/后手选择

## 环境要求

- Windows系统
- EasyX图形库
- C++编译器

## 游戏设置

运行后选择：

- 棋子颜色：黑棋(b) / 白棋(w)
- 先手顺序：先手(Y) / 后手(N)

## 游戏规则

- 黑白双方轮流落子
- 横、竖、斜任意方向连成五子即获胜
- AI使用7层搜索深度进行决策

## 技术特性

- **AI算法**：Minimax + Alpha-Beta剪枝
- **评估函数**：考虑活四、冲四、活三等多种棋型
- **优化策略**：只搜索有棋子的相邻位置

## 操作说明

- **鼠标点击**：在棋盘交叉点落子
- **自动判断**：游戏自动检测胜负

体验经典五子棋，挑战智能AI！ 🎲
