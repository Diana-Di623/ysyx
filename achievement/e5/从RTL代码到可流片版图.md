**1.安装Verilator**
![alt text](image.png)
![alt text](image-1.png)
**2.运行示例**
![alt text](image-2.png)
[Verilator C++ 示例](https://verilator.org/guide/latest/example_cc.html)
![alt text](image-3.png)
**3.对双控开关模块进行仿真**
![alt text](image-4.png)
```c++
// DESCRIPTION: Verilator: Verilog example module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2017 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0
//======================================================================

// Include common routines
#include <verilated.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
// Include model header, generated from Verilating "top.v"
#include "Vtop.h"

int main(int argc, char** argv) {
    // See a similar example walkthrough in the verilator manpage.

    // This is intended to be a minimal example.  Before copying this to start a
    // real project, it is better to start with a more complete example,
    // e.g. examples/c_tracing.

    // Construct a VerilatedContext to hold simulation time, etc.
    VerilatedContext* const contextp = new VerilatedContext;

    // Pass arguments so Verilated code can see them, e.g. $value$plusargs
    // This needs to be called before you create any model
    contextp->commandArgs(argc, argv);

    // Construct the Verilated model, from Vtop.h generated from Verilating "top.v"
    Vtop* const top = new Vtop{contextp};

    // Simulate until $finish
    for(int count=0;count<100;count++)
    {
        int a=rand()&1;
        int b=rand()&1;
        top->a=a;
        top->b=b;
        top->eval();
        printf("a = %d, b = %d, f = %d\n",
             a, b, static_cast<int>(top->f));

      assert(top->f == (a ^ b));
    }
    // Final model cleanup
    top->final();

    // Destroy model
    delete top;

    // Return good completion status
    return 0;
}

```
![alt text](image-5.png)
**4.生成波形并查看**
![alt text](image-6.png)
![alt text](image-7.png)
在官网找到手册：
![alt text](image-8.png)
修改代码：  
```c++
// DESCRIPTION: Verilator: Verilog example module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2017 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0
//======================================================================

// Include common routines
#include <verilated.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
// Include model header, generated from Verilating "top.v"
#include "Vtop.h"
#include <verilated_vcd_c.h>
int main(int argc, char** argv) {
    // See a similar example walkthrough in the verilator manpage.

    // This is intended to be a minimal example.  Before copying this to start a
    // real project, it is better to start with a more complete example,
    // e.g. examples/c_tracing.

    // Construct a VerilatedContext to hold simulation time, etc.
    VerilatedContext* const contextp = new VerilatedContext;

    // Pass arguments so Verilated code can see them, e.g. $value$plusargs
    // This needs to be called before you create any model
    contextp->commandArgs(argc, argv);

    // Construct the Verilated model, from Vtop.h generated from Verilating "top.v"
    Vtop* const top = new Vtop{contextp};
    contextp->traceEverOn(true);

    VerilatedVcdC* const tfp = new VerilatedVcdC;
    top->trace(tfp, 99);       // 最多跟踪 99 层模块层次
    tfp->open("waveform.vcd");
    // Simulate until $finish
    for(int count=0;count<100;count++)
    {
        int a=rand()&1;
        int b=rand()&1;
        top->a=a;
        top->b=b;
        top->eval();
        tfp->dump(contextp->time());
        printf("a = %d, b = %d, f = %d\n",
             a, b, static_cast<int>(top->f));

      assert(top->f == (a ^ b));
    contextp->timeInc(1);
    }
    // Final model cleanup
    top->final();
    tfp->close();
    // Destroy model
    delete top;
    delete tfp;
    // Return good completion status
    return 0;
}
```
![alt text](image-9.png)
![alt text](image-10.png)
**5.一键仿真**
![alt text](image-11.png)
![alt text](image-12.png)
```makefile

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	verilator --cc --exe --build -j 0 -Wall csrc/main.cpp vsrc/example.v
	./obj_dir/Vexample
```
**6.运行NVBoard示例**
![alt text](image-14.png)
查看~./bashrc是否有环境变量:  
![alt text](image-13.png)

阅读示例项目readme.md: 
![alt text](image-16.png)
![alt text](image-15.png)

**7.不知道NVBoard如何工作**
![alt text](image-17.png)
makefile找到 src/ 下所有 .cpp,分别编译成 build/*.o,top.v:1 会被转换成类 Vtop，顶层端口成为它的公开成员,通过top.nxdc映射，而main.cpp里面：
  1. nvboard_bind_all_pins(&dut)
     先把所有虚拟引脚指向 dut 的端口。

  2. nvboard_init()
     初始化 SDL 窗口和全部组件。组件初始化时已经可以访问绑定好的端口。

  3. reset(10)
     让 rst=1 跑十个周期，然后撤销复位。clk 和 rst 因为由 C++ 主程序直接驱动，所以不需要写进 .nxdc。

  4. nvboard_update()
     处理鼠标、键盘、VGA、UART，并根据 RTL 输出刷新界面。

  5. single_cycle()
     依次令时钟为 0、1，并各调用一次 dut.eval()

**8.在NVBoard上实现双控开关**

![alt text](image-21.png)
添加约束文件：
```txt
  npc/
  ├── Makefile
  ├── csrc/
  │   └── main.cpp          # Verilator + NVBoard 仿真入口
  ├── vsrc/
  │   └── top.v             # 双控开关 RTL
  ├── constr/
      └── top.nxdc          # 开关、LED 引脚约束
```
照抄nv-board/example里面makefile:
```makefile
TOPNAME = top
NXDC_FILES = constr/top.nxdc
INC_PATH ?=

VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc  \
				-O3 --x-assign fast --x-initial fast --noassert

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

default: $(BIN)

$(shell mkdir -p $(BUILD_DIR))

# constraint file
SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

# project source
VSRCS = $(shell find $(abspath ./vsrc) -name "*.v")
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
CSRCS += $(SRC_AUTO_BIND)

# rules for NVBoard
include $(NVBOARD_HOME)/scripts/nvboard.mk

# rules for verilator
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

$(BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

sim: $(BIN)
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@$^
clean:
	rm -rf $(BUILD_DIR)
include ../Makefile
.PHONY: sim clean
```
修改main.cpp:

```cpp
#include <nvboard.h>
#include <Vtop.h>

static TOP_NAME dut;

void nvboard_bind_all_pins(TOP_NAME* top);

static void single_cycle() {
  dut.eval();
}

static void reset(int n) {
  while (n -- > 0) single_cycle();
}

int main() {
  nvboard_bind_all_pins(&dut);
  nvboard_init();


  while(1) {
    nvboard_update();
    single_cycle();
  }
}

```
修改约束文件：
```txt
top=top
a SW0
b SW1
f LD0
```

![alt text](image-18.png)
![alt text](image-19.png)
![alt text](image-22.png)
![alt text](image-20.png)

**9.将流水灯接入NVBoard**
```v
module led(
  input clk,
  input rst,
  output reg[15:0] ledr
);  
    reg [3:0] count;
    reg [31:0] step;
  always @(posedge clk) begin
    if (rst) begin ledr <= 0; count<=0; step<=0;end
    else begin
        if(step==500000)begin
        ledr<=0;
        ledr[count]<=1;
        if(count==4'hf)count<=0;
        else count<=count+1'b1;
        step<=0;
        end
        else step<=step+1;
    end
  end
endmodule

module top(
  input clk,
  input rst,
  output [15:0] ledr
);
 led my_led(
    .clk(clk),
    .rst(rst),
    .ledr(ledr)
 );
endmodule
```
约束文件：
```txt
top=top
ledr (LD15, LD14, LD13, LD12, LD11, LD10, LD9, LD8, LD7, LD6, LD5, LD4, LD3, LD2, LD1, LD0)
```
![alt text](image-23.png)

**10.通过Verilator进行静态代码检查**
![alt text](image-24.png)
我修改了一些警告，比如文件名要和第一个模块相同，endmodule之后要ENTER

![alt text](image-25.png)
**11.我会写Verilog不就行了吗? 为什么要知道这些**
![alt text](image-26.png)
 1. “#0 将赋值强制延迟到当前仿真时刻末尾”——错误

  #0 只是把后续操作放进当前时刻的 Inactive 区，不是当前时刻的末尾。后面至少还有 NBA 等区域。

  a = 0;
  #0 a = 1;

  仿真时间没有前进，但 a = 1 会晚于当前 Active 区执行。不要用 #0 修补竞争问题。

  2. “同一 begin-end 中多次非阻塞赋值结果未定义”——错误

  同一个过程块中按确定顺序执行的 NBA 会保持赋值顺序，最后一次赋值获胜：

  always @(posedge clk) begin
    a <= 1'b0;
    a <= 1'b1;
  end

  本次 NBA 更新结束后，a 为 1。这种写法常用于设置默认值后按条件覆盖：

  always @(posedge clk) begin
    state <= IDLE;
    if (start)
      state <= RUN;
  end

  但若不同 always 块在同一时刻对 a 赋值，它们的先后顺序没有保证，可能产生竞争。

  3. “组合逻辑 always 中不能使用非阻塞赋值”√

  4. “不能在多个 always 块中给同一个变量赋值” √

  5. “不建议使用 $display，因为它有时不能正确输出”——错误

  $display 总是输出它执行那一刻变量的当前值。所谓“不正确”，通常是没有理解 NBA 尚未更新：
    q <= d;
    $display("q=%b", q);
  end

  这里 $display 在 Active 区执行，而 q <= d 到 NBA 区才更新，所以看到旧的 q 完全符合语义，不是 $display
  或仿真器出错。

  6. “$display 无法输出非阻塞赋值结果”——错误或至少表述不完整

  它只是无法在同一时间片的 NBA 更新之前看到新值。可以使用 $strobe 在当前时间片末尾观察：

  always @(posedge clk) begin
    q <= d;
    $display("display: q=%b", q); // 更新前的值
    $strobe ("strobe:  q=%b", q); // NBA 更新后的值
  end

  也可以在后续仿真时刻用 $display 输出，当然能看到之前 NBA 的结果。

  核心原则是：阻塞赋值立即更新，非阻塞赋值先计算右值、稍后在 NBA 区更新左值；$display 显示执行当下的
  值，$strobe 显示当前时间片结束时的值。
  **12.用事件模型分析Verilog代码的行为**
  ![alt text](image-27.png)
```txt
假设在 t+1 时刻出现 clk 上升沿，且没有其他过程修改这些变量。

  初始值：

  a=1, b=2, c=3, d=4, e=5

  在 Active 区按顺序执行：

  b = a;

  阻塞赋值立即更新：

  b=1

  c <= b;

  立即读取当前 b=1，但延迟到 NBA 区更新：

  排队：c ← 1
  当前 c 仍为 3

  d = c;

  此时 NBA 尚未执行，所以读取旧的 c=3：

  d=3

  e <= d;

  读取当前 d=3：

  排队：e ← 3
  当前 e 仍为 5

  a = e;

  此时 e 还没有执行 NBA 更新，所以读取旧值 5：

  a=5

  Active 区结束时：

  a=5, b=1, c=3, d=3, e=5

  随后进入 NBA 区：

  c ← 1
  e ← 3

  因此 t+1 时间片全部执行完毕后的最终结果是：

  a=5
  b=1
  c=1
  d=3
  e=3

  关键在于：

  = 会立即修改左值。
  <= 会立即计算右值，但等到 NBA 区才修改左值。
```

**13.理解Verilator生成的仿真程序的行为**
![alt text](image-28.png)
```cpp
  #include "Vtop.h"
  #include "verilated.h"

  #include <cstdint>
  #include <iomanip>
  #include <iostream>

  static void tick(Vtop* top, VerilatedContext* contextp) {
      top->clk = 0;
      top->eval();
      contextp->timeInc(1);

      top->clk = 1;
      top->eval();  
      contextp->timeInc(1);//产生时钟
  }

  int main(int argc, char** argv) {
      VerilatedContext context;
      context.commandArgs(argc, argv);

      Vtop top{&context};

      top.rst = 1;
      tick(&top, &context);
      tick(&top, &context);

      top.rst = 0;

      std::uint16_t previous = top.ledr;
      constexpr std::uint64_t maxCycles = 500000ULL * 16;

      for (std::uint64_t cycle = 1; cycle <= maxCycles; ++cycle) {
          tick(&top, &context);

          if (top.ledr != previous) {
              std::cout << "cycle=" << cycle
                        << " ledr=0x"
                        << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<unsigned>(top.ledr)
                        << std::dec << '\n';

              previous = top.ledr;
          }
      }

      top.final();
      return 0;
  }
  ```
![alt text](image-29.png)

 **14.用事件模型分析Verilog代码的行为(2)**

 ![alt text](image-30.png)
 ```txt
 两个 always 块的执行先后顺序不确定，但结果相同。

  可能顺序一：先执行第一个块

  读取 b = B_old，排队 NBA：a ← B_old
  读取 a = A_old，排队 NBA：b ← A_old

  可能顺序二：先执行第二个块

  读取 a = A_old，排队 NBA：b ← A_old
  读取 b = B_old，排队 NBA：a ← B_old

  因为 Active 区内非阻塞赋值没有立即修改 a 或 b，所以两个块读取的始终是时钟沿到来前的旧值。

  NBA 区执行后：

  a = B_old
  b = A_old

  也就是交换 a 和 b。

  复位时，两个块分别排队：

  a ← 0
  b ← 1

  NBA 区结束后得到：

  a=0, b=1

  之后每个时钟周期交换一次：

  复位后：      a=0, b=1
  第1个上升沿： a=1, b=0
  第2个上升沿： a=0, b=1
  第3个上升沿： a=1, b=0

  这段 RTL 不存在导致仿真结果不确定的数据竞争
```
 **15.用事件模型分析Verilog代码的行为(3)**
 ![alt text](image-31.png)
 ```txt
 $strobe 不会在调用语句执行时立即输出，而是把输出安排到当前时间片最后的 Postponed 区。因此，它会看到该
  时间片中更新完成后的 a。

  假设 rstn=1，在 clk 上升沿，两个 always 块的 Active 区执行顺序不确定。

  可能顺序一：

  第一个 always：立即执行 a = 1
  第二个 always：安排 $strobe
  Postponed 区：输出 a = 1

  可能顺序二：

  第二个 always：安排 $strobe
  第一个 always：立即执行 a = 1
  Postponed 区：输出 a = 1

  两种顺序最终都输出：

  a = 1
  ```
  
**16.尝试使用综合器**
![alt text](image-32.png)
iEDA安装成功：
![alt text](image-33.png)
![alt text](image-34.png)
**17.查看结构图**
![alt text](image-36.png)
![alt text](image-35.png)
**18. 查看结构图(2)**
![alt text](image-38.png)
区别在于：proc 将 Verilog 中的行为级 always 块转换成 Yosys 内部的逻辑单元。

  执行 proc 前：

  - 图中存在多个 PROC 节点。
  - 它们对应 always @(*)、always @(posedge clk) 等过程块。
  - if/case 等控制逻辑仍以行为级过程表示。

  执行 proc 后：

  - 所有 PROC 节点消失。
  - 组合逻辑被转换为 $mux、$pmux、$and、$eq、$sub 等单元。
  - 时序逻辑被转换为 $dff 等寄存器单元。
![alt text](image-37.png)

 **19.查看结构图(3)**
 ![alt text](image-40.png)
 opt 会删除冗余逻辑、合并等价信号，并将部分常见结构合并为更简洁的单元，但不会改变电路功能
![alt text](image-39.png)

**20. 查看结构图(4)**
![alt text](image-42.png)
techmap 将较高级、字宽级的通用单元展开成更基础的位级逻辑单元

![alt text](image-41.png)

**21.查看结构图(5)**
![alt text](image-43.png)
dfflibmap 将 Yosys 内部的通用触发器映射成目标工艺库中真实存在的标准单元  
![alt text](image-44.png)
**22.查看结构图(6)**
![alt text](image-46.png)
abc 将剩余的通用组合逻辑映射到 icsprout55 工艺库中的真实标准单元，并进行面积/逻辑优化
![alt text](image-45.png)
**23.通过yosys的日志文件了解综合过程**
![alt text](image-47.png)
  SystemVerilog RTL
    → 选择 led 为顶层、删除未使用模块
    → 行为过程转换为寄存器/组合逻辑
    → 通用逻辑优化
    → 第一次 ABC 通用门映射
    → 时钟门控尝试
    → 触发器映射
    → 第二次 ABC 标准单元映射及尺寸优化
    → 清理、检查、面积统计
    → 输出门级网表
**24.RTFM**
![alt text](image-48.png)
组合逻辑使用完整赋值和阻塞赋值，时序逻辑使用边沿事件和非阻塞赋值；除非确实需要锁存器、三态或综合“不关心”，否则不要依赖不完整赋值、x 或 z

**25.用事件模型分析Verilog代码的行为(4)**

```v
always @(posedge clock) begin
  a = 0;
  a = 1;
end

always @(posedge clock)
  b = a;

```
![alt text](image-49.png)
![alt text](image-50.png)
**26.RTFM(2)**
![alt text](image-51.png)

比如不完整的敏感列表：
```v
  always @(a)
    y = a & b;
```
综合器通常仍根据表达式生成二输入与门；RTL 仿真却只在 a 变化时重新计算，b 单独变化不会更新 y。
**27.评估电路的性能**
![alt text](image-52.png)
电路在当前综合级 STA 模型下的理论最高频率约为 1.80 GHz:  
![alt text](image-53.png)
**28.评估电路的功耗**
![alt text](image-54.png)
![alt text](image-55.png)
**29.了解ICsprout55的金属层**
![alt text](image-56.png)
![alt text](image-57.png)
**30.根据CDL文件画出晶体管结构**
![alt text](image-59.png)
![alt text](image-58.png)
验证一样： 
![alt text](image-60.png)
**31.理解复杂逻辑门单元的功能**
![alt text](image-61.png)
![alt text](image-62.png)
**32.理解驱动能力**
![alt text](image-63.png)
在pdk/icsprout55/IP/STD_cell/ics55_LLSC_H7C_V1p10C100/ics55_LLSC_H7CL/cdl/：  
  - NAND2X1H7L：面积和功耗最低，适合负载小、时序宽松的路径。
  - NAND2X2H7L：面积、功耗和驱动能力居中。
  - NAND2X4H7L：能驱动更大负载并改善延迟，但面积为 X1 的 3 倍，漏电约为 X1 的 3.45 倍，输入动态负担也更
    大。

**33.了解I/O单元的尺寸**
![alt text](image-64.png)
![alt text](image-65.png)
 **34.复杂单元的全定制电路**
 ![alt text](image-66.png)
 ![alt text](image-67.png)
 ![alt text](image-68.png)
 **35.了解的所有标准单元**
 ![alt text](image-69.png)
 ![alt text](image-70.png)
 **36.尝试不同PVT角的评估结果**
 ![alt text](image-71.png)
 ![alt text](image-73.png)
 **37.理解ICsprout55的轨道数**
 ![alt text](image-72.png)
 ![alt text](image-74.png)
 **38.填充单元的尺寸**
 ![alt text](image-75.png)
 ![alt text](image-77.png)
 **39.尝试用negedge综合**
![alt text](image-76.png)
![alt text](image-82.png)
**40.借助NVBoard完成数字电路实验**
![alt text](image-78.png)
项目在ysyx-workbranch/npc/lab  
![alt text](image-79.png)
lab2:
![alt text](image-80.png)
lab3:
![alt text](image-81.png)
lab6:
![alt text](image-83.png)
lab7:
![alt text](image-84.png)
lab8:
![alt text](image-85.png)

 **用RTL实现sCPU**
 ![alt text](image-86.png)
 ![alt text](image-87.png)