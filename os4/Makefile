ifneq ($(KERNELRELEASE),)
	obj-m := os4.o
else
	KDIR := /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 新增的便利命令
install: default
	@sudo rmmod os4 2>/dev/null || true
	sudo insmod os4.ko
	@echo "模块已安装! 使用方法:"
	@echo "  查看统计: cat /proc/os4/pfcount"
	@echo "  查看配置: cat /proc/os4/config"
	@echo "  重置统计: echo 'reset' > /proc/os4/config"

uninstall:
	@sudo rmmod os4 2>/dev/null || true
	@echo "模块已卸载"

test: install
	@echo "=== 快速功能测试 ==="
	@echo "1. 当前统计:"
	@cat /proc/os4/pfcount || echo "无法读取统计文件"
	@echo ""
	@echo "2. 产生内存活动..."
	@dd if=/dev/zero of=/tmp/test bs=1M count=10 2>/dev/null && rm -f /tmp/test
	@echo ""
	@echo "3. 活动后统计:"
	@cat /proc/os4/pfcount || echo "无法读取统计文件"
	@echo ""
	@echo "4. 测试重置功能..."
	@echo 'reset' > /proc/os4/config 2>/dev/null && echo "重置成功!" || echo "重置失败"

# 查看当前统计
stats:
	@cat /proc/os4/pfcount 2>/dev/null || echo "模块未安装，请先运行: make install"

# 查看配置
config:
	@cat /proc/os4/config 2>/dev/null || echo "模块未安装，请先运行: make install"

# 重置统计
reset:
	@echo 'reset' > /proc/os4/config 2>/dev/null && echo "统计已重置" || echo "重置失败，模块可能未安装"

# 创建简单的监控脚本
monitor: install
	@echo "#!/bin/bash" > /tmp/os4_monitor.sh
	@echo "echo '开始实时监控 (按Ctrl+C退出):'" >> /tmp/os4_monitor.sh
	@echo "while true; do" >> /tmp/os4_monitor.sh
	@echo "  clear" >> /tmp/os4_monitor.sh
	@echo "  echo '=== OS4 实时监控 \$$(date) ==='" >> /tmp/os4_monitor.sh
	@echo "  cat /proc/os4/pfcount 2>/dev/null || echo '模块未加载'" >> /tmp/os4_monitor.sh
	@echo "  sleep 2" >> /tmp/os4_monitor.sh
	@echo "done" >> /tmp/os4_monitor.sh
	@chmod +x /tmp/os4_monitor.sh
	@/tmp/os4_monitor.sh

# 显示帮助
help:
	@echo "可用命令:"
	@echo "  make install   - 编译并安装模块"
	@echo "  make test      - 完整功能测试"
	@echo "  make stats     - 查看当前统计"
	@echo "  make config    - 查看配置选项"
	@echo "  make reset     - 重置统计数据"
	@echo "  make monitor   - 实时监控模式"
	@echo "  make uninstall - 卸载模块"
	@echo "  make clean     - 清理编译文件"
	@echo "  make help      - 显示此帮助"

.PHONY: default clean install uninstall test stats config reset monitor help

endif

