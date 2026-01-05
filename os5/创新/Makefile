# 智能阅览室管理系统 Makefile
# 适配 library.c 和 read.c (支持C99标准)

CC = gcc
CFLAGS = -Wall -g -std=c99 -D_SVID_SOURCE -D_DEFAULT_SOURCE

# 目标文件
MANAGER = library
READER = read
TEST_FULL = test_full
CLEAR_USERS = clear_users

# 源文件
MANAGER_SRC = library.c
READER_SRC = read.c
TEST_FULL_SRC = test_full.c
CLEAR_USERS_SRC = clear_users.c

# 默认目标
all: $(MANAGER) $(READER) $(TEST_FULL) $(CLEAR_USERS)

# 编译管理程序
$(MANAGER): $(MANAGER_SRC)
	$(CC) $(CFLAGS) -o $(MANAGER) $(MANAGER_SRC)
	@echo "阅览室管理程序编译完成"

# 编译读者程序  
$(READER): $(READER_SRC)
	$(CC) $(CFLAGS) -o $(READER) $(READER_SRC)
	@echo "读者程序编译完成"

# 编译测试程序
$(TEST_FULL): $(TEST_FULL_SRC)
	$(CC) $(CFLAGS) -o $(TEST_FULL) $(TEST_FULL_SRC)
	@echo "满员测试程序编译完成"

# 编译清理程序
$(CLEAR_USERS): $(CLEAR_USERS_SRC)
	$(CC) $(CFLAGS) -o $(CLEAR_USERS) $(CLEAR_USERS_SRC)
	@echo "用户清理程序编译完成"

# 初始化阅览室
init: $(MANAGER)
	@echo "正在初始化阅览室系统..."
	./$(MANAGER)

# 启动读者进程
start: $(READER)
	@echo "启动读者进程..."
	./$(READER)

# 一键填满座位（测试满员情况）
fulltest: $(TEST_FULL)
	@echo "开始满员测试：自动添加5个用户..."
	./$(TEST_FULL)
	@echo ""
	@echo "满员测试完成！现在可以："
	@echo "1. 运行 'make start' 测试新用户等待座位"
	@echo "2. 运行 'make clear_test' 清空所有测试用户"

# 清空所有测试用户
clear_test: $(CLEAR_USERS)
	@echo "清空所有测试用户..."
	./$(CLEAR_USERS)

# 完整的满员测试流程
demo_full: $(READER) $(TEST_FULL) $(CLEAR_USERS)
	@echo "=== 完整满员测试演示 ==="
	@echo "1. 先清空现有用户..."
	-./$(CLEAR_USERS) 2>/dev/null
	@echo "2. 填满所有座位..."
	./$(TEST_FULL)
	@echo ""
	@echo "3. 现在启动新的读者进程会需要等待座位："
	@echo "   请在另一个终端运行: make start"
	@echo "   或直接运行: ./$(READER)"
	@echo ""
	@echo "4. 测试完成后，运行 'make clear_test' 清理"

# 多个读者进程演示
demo: $(READER)
	@echo "启动多读者演示"
	@echo "提示：请在多个终端中分别运行 ./$(READER)"
	@echo ""
	@echo "演示步骤："
	@echo "1. 终端1: ./$(READER)  # 第一个读者"
	@echo "2. 终端2: ./$(READER)  # 第二个读者" 
	@echo "3. 终端3: ./$(READER)  # 第三个读者"
	@echo "4. 在每个读者中选择 's' 查看实时状态"

# 清理编译文件
clean:
	rm -f $(MANAGER) $(READER) $(TEST_FULL) $(CLEAR_USERS) *.o
	@echo "清理完成"

# 清理系统资源
reset:
	@echo "清理系统资源..."
	@ipcs -s | grep -E "(12346|12347)" | awk '{print $$2}' | xargs -r ipcrm -s 2>/dev/null || true
	@ipcs -m | grep 12345 | awk '{print $$2}' | xargs -r ipcrm -m 2>/dev/null || true
	@echo "系统资源已清理"

# 查看系统资源状态
status:
	@echo "系统资源状态："
	@echo "共享内存："
	@ipcs -m | grep -E "(12345|key.*shmid)" || echo "  无相关共享内存"
	@echo "信号量："
	@ipcs -s | grep -E "(12346|12347|key.*semid)" || echo "  无相关信号量"

# 编译信息
info:
	@echo "编译配置："
	@echo "  编译器: $(CC)"
	@echo "  编译选项: $(CFLAGS)"
	@echo "  管理程序: $(MANAGER_SRC) -> $(MANAGER)"
	@echo "  读者程序: $(READER_SRC) -> $(READER)"
	@echo "  测试程序: $(TEST_FULL_SRC) -> $(TEST_FULL)"
	@echo "  清理程序: $(CLEAR_USERS_SRC) -> $(CLEAR_USERS)"

# 帮助信息
help:
	@echo "智能阅览室管理系统使用指南"
	@echo ""
	@echo "文件说明："
	@echo "  library.c     - 阅览室管理程序源码"
	@echo "  read.c        - 读者程序源码"
	@echo "  test_full.c   - 满员测试程序源码"
	@echo "  clear_users.c - 用户清理程序源码"
	@echo ""
	@echo "编译命令："
	@echo "  make all      - 编译所有程序"
	@echo "  make clean    - 清理编译文件"
	@echo "  make info     - 显示编译配置信息"
	@echo ""
	@echo "基本运行命令："  
	@echo "  make init     - 初始化阅览室（必须先执行）"
	@echo "  make start    - 启动读者进程"
	@echo "  make demo     - 多读者演示指南"
	@echo ""
	@echo "测试命令："
	@echo "  make fulltest    - 一键填满所有座位（5个测试用户）"
	@echo "  make clear_test  - 清空所有测试用户"
	@echo "  make demo_full   - 完整满员测试演示"
	@echo ""
	@echo "系统维护："
	@echo "  make status   - 查看系统资源状态"
	@echo "  make reset    - 清理系统资源"
	@echo ""
	@echo "完整使用流程："
	@echo "  1. make all       # 编译程序"
	@echo "  2. make init      # 初始化阅览室"  
	@echo "  3. make fulltest  # 填满座位测试满员情况"
	@echo "  4. make start     # 测试新用户等待座位"
	@echo "  5. make clear_test # 清理测试用户"
	@echo ""
	@echo "满员测试场景："
	@echo "  1. make fulltest     # 自动添加5个用户占满座位"
	@echo "  2. make start        # 第6个用户会等待座位"
	@echo "  3. make clear_test   # 清理所有测试用户"

# 快速测试
test: all
	@echo "编译测试完成！"
	@echo "请按顺序执行："
	@echo "1. make init        # 初始化阅览室"
	@echo "2. make fulltest    # 测试满员情况"
	@echo "3. make start       # 测试等待座位"

.PHONY: all clean init start demo fulltest clear_test demo_full reset status help test info

