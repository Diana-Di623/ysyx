**1.观察预处理结果**
![alt text](image.png)  
核心代码：
```c
int main() {
  printf("Hello World!\n" );



  printf("RISC-V");
  return 0;
}
```
- 1.头文件被展开
- 2.反斜杠续行被合并
- 3.对象宏被展开
- 4.注释被删除
- 5.条件编译被处理
- 6.## 完成记号拼接
- 7.# 完成记号字符串化

**2.如何寻找头文件**
![alt text](image-1.png)
GCC 会依次尝试:
```c
/usr/lib/gcc/x86_64-linux-gnu/11/include/stdio.h
/usr/local/include/stdio.h
/usr/include/x86_64-linux-gnu/stdio.h
/usr/include/stdio.h
```
创建myheader.h文件，并用gcc -I . -E a.c -o a.i命令进行预处理，查看myheader.h是否被正确包含。
![alt text](image-2.png)
符合要求

**2.观察预处理结果(2)**
![alt text](image-3.png)
![alt text](image-4.png)
现在
```c
#ifdef __riscv
  printf("Hello RISC-V!\n");
#endif
```
__riscv被识别了
**3.对比gcc和riscv64-linux-gnu-gcc的预定义宏**
![alt text](image-5.png)
diff -y gcc.txt riscv64-linux-gnu-gcc.txt 
![alt text](image-6.png)
**3.了解编译的过程**
![alt text](image-7.png)
```txt
clang  is  a  C, C++, and Objective-C compiler which encompasses preprocessing, parsing, optimiza‐tion, code generation, assembly, and linking.  Depending  on  which  high-level  mode  setting  is passed,  Clang will stop before doing a full link
```
- -E: 只进行预处理
- -S: 只进行编译，生成汇编代码
- -c: 只进行编译，生成目标文件
- -o: 指定输出文件名

**4.词法分析**
![alt text](image-8.png)
词法分析的结果通过文件名:行号:列号的格式记录了每一个token的位置  
**5.语法分析**
![alt text](image-9.png)
**6.语义分析**
![alt text](image-10.png)
**7.对比编译优化的结果**
![alt text](image-11.png)
![alt text](image-13.png)
可以看到优化后消除了死代码，不再为x,y,z分配栈针，printf语句里面直接用了常量30
**8.对比编译优化的结果(2)**
![alt text](image-14.png)
![alt text](image-12.png)
加了volatile之后，编译给x,y分配了栈针，printf语句里面用的x+y  
**9.理解C代码与riscv指令序列的关联**
![alt text](image-16.png)
![alt text](image-15.png)
**10.理解C代码与riscv指令序列的关联(2)**
![alt text](image-17.png)
![alt text](image-18.png)
可以看到直接加载30立即数，只剩下printf语句  
**11.查看riscv64目标文件的反汇编结果**
![alt text](image-19.png)
反汇编：
![alt text](image-20.png)
汇编：
![alt text](image-15.png)
不难发现，反汇编结果和汇编结果是一样的，只是反汇编结果多了地址和机器码,少了源代码信息  
**12.查看riscv64可执行文件的反汇编结果**
![alt text](image-21.png)
objdump of a.out:
![alt text](image-23.png)
可执行文件已分配最终地址，而且已经完成符号解析  
**13.对比编译优化前后的性能差异**
![alt text](image-22.png)
![alt text](image-24.png)
从计算为主导到系统调用为主导，优化后性能提升明显  
原因是直接加载计算结果 
```asm
 1076:       48 ba 00 65 7f f1 59    movabs $0x6f05b59f17f6500,%rdx
 ```
 直接加载了计算结果，而不是计算过程，减少了指令数和内存访问次数
 **14.程序真的从main()开始执行吗?**  
 ![alt text](image-26.png)
 ![alt text](image-25.png)
 **15.程序真的从main()返回后结束吗?**
 ![alt text](image-27.png)
 ![alt text](image-28.png)
 ret后，程序并没有结束，而是跳转到了__libc_start_call_main（main的调用者）
 ![alt text](image-29.png)
 之后调用了exit,__run_exit_handlers函数等，由系统调用退出
 **16.体验未指定行为**
 ![alt text](image-30.png)
 ![alt text](image-31.png)
编译器通过随机方式决定函数调用时的参数求值顺序
**17.体验未定义行为**
![alt text](image-33.png)
![alt text](image-32.png)
输出都是垃圾值
**18.实现sEMU**
![alt text](image-34.png)
sISA:
```txt
 7  6 5  4 3   2 1   0
+----+----+-----+-----+
| 00 | rd | rs1 | rs2 | R[rd]=R[rs1]+R[rs2]       add指令, 寄存器相加
+----+----+-----+-----+
| 10 | rd |    imm    | R[rd]=imm                 li指令, 装入立即数, 高位补0
+----+----+-----+-----+
| 11 |   addr   | rs2 | if (R[0]!=R[rs2]) PC=addr bner0指令, 若不等于R[0]则跳转
+----+----------+-----+
```
指令： 
```txt
10001010    # 0: li r0, 10
10010000    # 1: li r1, 0
10100000    # 2: li r2, 0
10110001    # 3: li r3, 1
00010111    # 4: add r1, r1, r3
00101001    # 5: add r2, r2, r1
11010001    # 6: bner0 r1, 4
11011111    # 7: bner0 r3, 7
```
```c
#include<stdio.h>
#include <stdint.h>
uint8_t PC = 0;
uint8_t R[4]={0};
  uint8_t M[16] = { 
        0b10001010,    // 0: li r0, 10
        0b10010000,    // 1: li r1, 0
        0b10100000,    // 2: li r2, 0
        0b10110001,    // 3: li r3, 1
        0b00010111,    // 4: add r1, r1, r3
        0b00101001,    // 5: add r2, r2, r1
        0b11010001,    // 6: bner0 r1, 4
        0b11011111    // 7: bner0 r3, 7
     };
void inst_cycle()
{
    uint8_t inst=(M[PC]>>6)&0x3;
    uint8_t rd=(M[PC]>>4)&0x3;
    uint8_t rs1= (M[PC]>>2)&0x3;
    uint8_t rs2=(M[PC])&0x3;
    uint8_t imm=(M[PC])&0xf;
    uint8_t addr=(M[PC]>>2)&0xf;
    switch (inst)
    {
    case 0b00: //add R[rd]=R[rs1]+R[rs2] 
        R[rd]=R[rs1]+R[rs2];
        PC++;
        break;
    case 0b10: // li R[rd]=imm
        R[rd]=imm;
        PC++;
        break;
    case 0b11: //bner0 if (R[0]!=R[rs2]) PC=addr 
         if (R[rs2] != R[0])
                PC = addr;
            else
                PC++;
            break;
    default:
        printf("unknown inst");
        break;
    }
}
int main()
{
    for (int i=0;i<100;i++) { inst_cycle(); }
    printf("%d",R[2]);
}
```
**19.实现输出功能**
```txt
 7  6 5  4 3   2 1   0
+----+----+-----+-----+
| 00 | rd | rs1 | rs2 | R[rd]=R[rs1]+R[rs2]       add指令, 寄存器相加
+----+----+-----+-----+
| 10 | rd |    imm    | R[rd]=imm                 li指令, 装入立即数, 高位补0
+----+----------+-----+
| 01 | rd |           | R[rd]=imm                 读R[rd]到七段数码管显示器
+----+----+-----+-----+
| 11 |   addr   | rs2 | if (R[0]!=R[rs2]) PC=addr bner0指令, 若不等于R[0]则跳转
+----+----------+-----+
```
指令：
```txt
10001010    # 0: li r0, 10
10010000    # 1: li r1, 0
10100000    # 2: li r2, 0
10110001    # 3: li r3, 1
00010111    # 4: add r1, r1, r3
00101001    # 5: add r2, r2, r1
11010001    # 6: bner0 r1, 4
01011111    # 7: out r2
```
```c
#include<stdio.h>
#include <stdint.h>
uint8_t PC = 0;
uint8_t R[4]={0};
  uint8_t M[16] = { 
        0b10001010,    // 0: li r0, 10
        0b10010000,    // 1: li r1, 0
        0b10100000,    // 2: li r2, 0
        0b10110001,    // 3: li r3, 1
        0b00010111,    // 4: add r1, r1, r3
        0b00101001,    // 5: add r2, r2, r1
        0b11010001,    // 6: bner0 r1, 4
        0b01101111       // 7: out r2
     };
void inst_cycle()
{
    uint8_t inst=(M[PC]>>6)&0x3;
    uint8_t rd=(M[PC]>>4)&0x3;
    uint8_t rs1= (M[PC]>>2)&0x3;
    uint8_t rs2=(M[PC])&0x3;
    uint8_t imm=(M[PC])&0xf;
    uint8_t addr=(M[PC]>>2)&0xf;
    switch (inst)
    {
    case 0b00: //add R[rd]=R[rs1]+R[rs2] 
        R[rd]=R[rs1]+R[rs2];
        PC++;
        break;
    case 0b01: //out rd
        printf("%d",R[rd]);
        PC++;
        break;
    case 0b10: // li R[rd]=imm
        R[rd]=imm;
        PC++;
        break;
    case 0b11: //bner0 if (R[0]!=R[rs2]) PC=addr 
         if (R[rs2] != R[0])
                PC = addr;
            else
                PC++;
            break;
    default:
        printf("unknown inst");
        break;
    }
}
int main()
{
    while(PC<=7) { inst_cycle(); }
}
```
**20.实现参数化的数列求和**
![alt text](image-36.png)
```c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> 
uint8_t PC = 0;
uint8_t R[4]={0};
  uint8_t M[16] = { 
        0b10010000,    // 0: li r1, 0
        0b10100000,    // 1: li r2, 0
        0b10110001,    // 2: li r3, 1
        0b00010111,    // 3: add r1, r1, r3
        0b00101001,    // 4: add r2, r2, r1
        0b11001101,    // 5: bner0 r1, 3
        0b01101111     // 6: out r2
     };
void inst_cycle()
{
    uint8_t inst=(M[PC]>>6)&0x3;
    uint8_t rd=(M[PC]>>4)&0x3;
    uint8_t rs1= (M[PC]>>2)&0x3;
    uint8_t rs2=(M[PC])&0x3;
    uint8_t imm=(M[PC])&0xf;
    uint8_t addr=(M[PC]>>2)&0xf;
    switch (inst)
    {
    case 0b00: //add R[rd]=R[rs1]+R[rs2] 
        R[rd]=R[rs1]+R[rs2];
        PC++;
        break;
    case 0b01: //out rd
        printf("%d\n",R[rd]);
        PC++;
        break;
    case 0b10: // li R[rd]=imm
        R[rd]=imm;
        PC++;
        break;
    case 0b11: //bner0 if (R[0]!=R[rs2]) PC=addr 
         if (R[rs2] != R[0])
                PC = addr;
            else
                PC++;
            break;
    default:
        printf("unknown inst");
        break;
    }
}
int main(int argc,char *argv[])
{  
     R[0]=atoi(argv[1]);
    while(PC<=6) { inst_cycle(); }
}
```
![alt text](image-37.png)
**21.实现两条指令的minirvEMU**
![alt text](image-38.png)
minirv有16个GPR
![alt text](image-40.png)
![alt text](image-39.png)
is_addi = (inst[6:0] == 0010011) && (inst[14:12] == 000)  
is_jalr = (inst[6:0] == 1100111) && (inst[14:12] == 000)  
```c
#include <stdint.h>
#include <stdio.h>
uint32_t PC = 0;
uint32_t R[16]={0};
uint32_t M[16]={
//00000000 <_start>:
  /*0:*/	0x01400513,          	//addi	a0,zero,20
  /*4:*/	0x010000e7,         	//jalr	ra,16(zero) # 10 <fun>
  /*8:*/	0x00c000e7,          	//jalr	ra,12(zero) # c <halt>

//0000000c <halt>:
  /*c:*/	0x00c00067,          	//jalr	zero,12(zero) # c <halt>

//00000010 <fun>:
  /*10:*/	0x00a50513,          	//addi	a0,a0,10
  /*14:*/	0x00008067          	//jalr	zero,0(ra)
};
void inst_cycle()
   {
    uint32_t inst=M[PC/4];
    uint32_t imm=inst>>20&0xfff;
    uint32_t opcode=inst&0x7f;
    uint32_t rd=inst>>7&0x1f;
    uint32_t funct3=inst>>12&0x7;
    uint32_t rs1=inst>>15&0x1f;
    switch(opcode)
    {
        case 0b0010011:
          switch(funct3)
          {
            case 0b000://is_addi = (inst[6:0] == 0010011) && (inst[14:12] == 000)，addi rd, rs1, imm
               if(imm&0x800)
              {imm=imm|0xfffff000;}
              R[rd]=R[rs1]+imm;
              PC+=4;
              break;
            default:
              printf("unknown inst\n");
              break;
          }
          break;
        case 0b1100111:
        switch(funct3)
          {
            case 0b000:  //is_jalr = (inst[6:0] == 1100111) && (inst[14:12] == 000),X[rd] = PC + 4,PC = (X[rs1] + sign_extend(offset)) & ~1
              if(rd!=0)R[rd]=PC+4;
              if(imm&0x800)
              {imm=imm|0xfffff000;}
              PC=(R[rs1]+imm)&(~1);
              break;
            default:
              printf("unknown inst\n");
              break;
          }
          break;
        default:
          printf("unknown inst\n");


    }
}
int main(int argc,char *argv[])
{  
    while(1) { inst_cycle(); }
}
```
![alt text](image-41.png)
**22.实现完整的minirvEMU**
![alt text](image-42.png)
![alt text](image-43.png)
![alt text](image-44.png)
![alt text](image-45.png)
![alt text](image-47.png)
![alt text](image-46.png)
![alt text](image-48.png)
```c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
uint32_t PC = 0;
uint32_t R[16]={0};
uint32_t M[100000]={0};
int load(char* filename)
{
  FILE * fp=fopen(filename,"rb");
  if(!fp)
  {
    printf("unable to load %s",filename);
    return -1;
  }
  fseek(fp,0,SEEK_END);
  long file_size=ftell(fp);
  fseek(fp,0,SEEK_SET);
  if(file_size>sizeof(M))
  {
    printf("file is too big");
    fclose(fp);
    return -1;
  }
  fread(M,4,file_size/4,fp);
  return 0;
}
void inst_cycle()
   {
    uint32_t inst=M[PC/4];
    uint32_t imm_I=inst>>20&0xfff;
    uint32_t sign_extend_imm_I=(imm_I&0x800)?imm_I|0xfffff000:imm_I;
    uint32_t imm_U=inst>>12&0xfffff;
    uint32_t imm_S=((inst>>25&0x7f)<<5)+(inst>>7&0x1f);
    uint32_t sign_extend_imm_S=(imm_S&0x800)?imm_S|0xfffff000:imm_S;
    uint32_t opcode=inst&0x7f;
    uint32_t rs1=inst>>15&0x1f;
    uint32_t rs2=inst>>20&0x1f;
    uint32_t rd=inst>>7&0x1f;
    uint32_t funct3=inst>>12&0x7;
    uint32_t funct7=inst>>25&0x7f;
    switch(opcode)
    {   case 0b0000011:
            if(funct3==0b010)//lw
            {    uint32_t addr=R[rs1]+sign_extend_imm_I;
                if((addr&0x3)==0)
                {
                  if(rd!=0){
                  R[rd]=M[addr/4];
                  }
               }
                else{
                  printf("misaligned!");
                }
               PC+=4;
            }
            else if(funct3==0b100)//lbu
            {
              if(rd!=0)
              { uint32_t addr=R[rs1]+sign_extend_imm_I;
                uint32_t byte_offset = addr & 0x3;
                uint32_t word=M[addr/4];
                R[rd]=word>>(byte_offset*8)&0xff;
              }
              PC+=4;
            }
            else{
                printf("unknown funct3\n");
            }
            break;
        case 0b0010011:
            if(funct3==0b000)//is_addi = (inst[6:0] == 0010011) && (inst[14:12] == 000)，addi rd, rs1, imm
              { if(rd!=0)
                {R[rd]=R[rs1]+sign_extend_imm_I;}
                PC+=4;
              }
              else{
                printf("unknown funct3\n");
              }
              break;
        case 0b0100011:
              if(funct3==0b000)//sb
              { uint32_t addr=R[rs1]+sign_extend_imm_S;
                uint32_t offset=addr&0x3;
                M[addr/4]=(R[rs2]<<(8*offset))+(M[addr/4]&(~(0xff<<(8*offset))));
                PC+=4;
              }
              else if(funct3==0b010)//sw
              {   
                uint32_t addr=R[rs1]+sign_extend_imm_S;
                 if((addr&0x3)==0)
                {
                  M[addr/4]=R[rs2];
                  PC+=4;
              }
                else{
                  printf("misaligned!");
                  PC+=4;
                }
              }
              else{
                 printf("unknown funct3\n");
              }
              break;
        case 0b0110011://add
             if(funct3==0b000&&funct7==0b0000000)
            {   if(rd!=0)
                {R[rd]=R[rs1]+R[rs2];}
                PC+=4;
            }
               else
               { printf("unknown funct3 or funct7\n");}
                break;
        case 0b0110111://lui R[rd]=imm20
            if(rd!=0)
            {R[rd]=imm_U<<12;}
            PC+=4;
            break;
        case 0b1100111:
        if(funct3==0b000)  //is_jalr = (inst[6:0] == 1100111) && (inst[14:12] == 000),X[rd] = PC + 4,PC = (X[rs1] + sign_extend(offset)) & ~1
            {  
              uint32_t addr=(R[rs1]+sign_extend_imm_I)&(~1);
              if(rd!=0)R[rd]=PC+4;
              PC=addr;
               }
           else{
              printf("unknown funct3\n");}
              break;
        default:
          printf("unknown inst\n");


    }
}
int main(int argc,char *argv[])
{   char* filename="sum.bin";
    if(load(filename)==0)
    {
    printf("load program successful");
    while(1) { inst_cycle(); }
    }
}
```
运行sum:  
![alt text](image-50.png)
![alt text](image-49.png)
运行mem:
![alt text](image-51.png)
![alt text](image-52.png)
**23.实现程序结束的自动判断**
![alt text](image-53.png)
![alt text](image-54.png)
![alt text](image-55.png)
```c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
uint32_t PC = 0;
uint32_t R[16]={0};
uint32_t M[100000]={0};
bool end=false;
int load(char* filename)
{
  FILE * fp=fopen(filename,"rb");
  if(!fp)
  {
    printf("unable to load %s",filename);
    return -1;
  }
  fseek(fp,0,SEEK_END);
  long file_size=ftell(fp);
  fseek(fp,0,SEEK_SET);
  if(file_size>sizeof(M))
  {
    printf("file is too big");
    fclose(fp);
    return -1;
  }
  fread(M,4,file_size/4,fp);
  return 0;
}
void inst_cycle()
   {
    uint32_t inst=M[PC/4];
    uint32_t imm_I=inst>>20&0xfff;
    uint32_t funct12=imm_I;
    uint32_t sign_extend_imm_I=(imm_I&0x800)?imm_I|0xfffff000:imm_I;
    uint32_t imm_U=inst>>12&0xfffff;
    uint32_t imm_S=((inst>>25&0x7f)<<5)+(inst>>7&0x1f);
    uint32_t sign_extend_imm_S=(imm_S&0x800)?imm_S|0xfffff000:imm_S;
    uint32_t opcode=inst&0x7f;
    uint32_t rs1=inst>>15&0x1f;
    uint32_t rs2=inst>>20&0x1f;
    uint32_t rd=inst>>7&0x1f;
    uint32_t funct3=inst>>12&0x7;
    uint32_t funct7=inst>>25&0x7f;
    switch(opcode)
    {   case 0b0000011:
            if(funct3==0b010)//lw
            {    uint32_t addr=R[rs1]+sign_extend_imm_I;
                if((addr&0x3)==0)
                {
                  if(rd!=0){
                  R[rd]=M[addr/4];
                  }
               }
                else{
                  printf("misaligned!");
                }
               PC+=4;
            }
            else if(funct3==0b100)//lbu
            {
              if(rd!=0)
              { uint32_t addr=R[rs1]+sign_extend_imm_I;
                uint32_t byte_offset = addr & 0x3;
                uint32_t word=M[addr/4];
                R[rd]=word>>(byte_offset*8)&0xff;
              }
              PC+=4;
            }
            else{
                printf("unknown funct3\n");
            }
            break;
        case 0b0010011:
            if(funct3==0b000)//is_addi = (inst[6:0] == 0010011) && (inst[14:12] == 000)，addi rd, rs1, imm
              { if(rd!=0)
                {R[rd]=R[rs1]+sign_extend_imm_I;}
                PC+=4;
              }
              else{
                printf("unknown funct3\n");
              }
              break;
        case 0b0100011:
              if(funct3==0b000)//sb
              { uint32_t addr=R[rs1]+sign_extend_imm_S;
                uint32_t offset=addr&0x3;
                M[addr/4]=(R[rs2]<<(8*offset))+(M[addr/4]&(~(0xff<<(8*offset))));
                PC+=4;
              }
              else if(funct3==0b010)//sw
              {   
                uint32_t addr=R[rs1]+sign_extend_imm_S;
                 if((addr&0x3)==0)
                {
                  M[addr/4]=R[rs2];
                  PC+=4;
              }
                else{
                  printf("misaligned!");
                  PC+=4;
                }
              }
              else{
                 printf("unknown funct3\n");
              }
              break;
        case 0b0110011://add
             if(funct3==0b000&&funct7==0b0000000)
            {   if(rd!=0)
                {R[rd]=R[rs1]+R[rs2];}
                PC+=4;
            }
               else
               { printf("unknown funct3 or funct7\n");}
                break;
        case 0b0110111://lui R[rd]=imm20
            if(rd!=0)
            {R[rd]=imm_U<<12;}
            PC+=4;
            break;
        case 0b1100111:
        if(funct3==0b000)  //is_jalr = (inst[6:0] == 1100111) && (inst[14:12] == 000),X[rd] = PC + 4,PC = (X[rs1] + sign_extend(offset)) & ~1
            {  
              uint32_t addr=(R[rs1]+sign_extend_imm_I)&(~1);
              if(rd!=0)R[rd]=PC+4;
              PC=addr;
               }
           else{
              printf("unknown funct3\n");}
              break;
        case 0b1110011://ebreak
        if(funct12==0b000000000001&&funct3==0&&R[rd]==0&&R[rs1]==0)  
            {
              printf("end of the program!\n");     
              end=true;
            }
            break;  
        default:
          printf("unknown inst\n");


    }
}
int main(int argc,char *argv[])
{   char* filename="mem.bin";
    uint32_t halt=0x1218;
    if(load(filename)==0)
    {
    M[halt/4+1]=0x00100073;
    printf("load program successful\n");
    while(end==false) { inst_cycle(); }
    }
}
```
![alt text](image-56.png)
**24.运行红白机游戏**
![alt text](image-57.png)
![alt text](image-62.png)
![alt text](image-58.png)
**25.体验时钟功能**
![alt text](image-59.png)
![alt text](image-60.png)
```c
#include <amtest.h>

void rtc_test() {
  AM_TIMER_RTC_T rtc;
  int sec = 1;
  while (1) {
    while(io_read(AM_TIMER_UPTIME).us / 1000000 < sec) ;
    rtc = io_read(AM_TIMER_RTC);
    printf("%d-%d-%d %02d:%02d:%02d GMT (", rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second);
    if (sec == 1) {
      printf("%d second).\n", sec);
    } else {
      printf("%d seconds).\n", sec);
    }
    sec ++;
  }
}
```
通过频繁us获取来判断到秒没有
**26.体验按键功能**
![alt text](image-61.png)
![alt text](image-58.png)
```c
#include <amtest.h>

#define NAMEINIT(key)  [ AM_KEY_##key ] = #key,
static const char *names[] = {
  AM_KEYS(NAMEINIT)
};

static bool has_uart, has_kbd;

static void drain_keys() {
  if (has_uart) {
    while (1) {
      char ch = io_read(AM_UART_RX).data;
      if (ch == (char)-1) break;
      printf("Got (uart): %c (%d)\n", ch, ch & 0xff);
    }
  }

  if (has_kbd) {
    while (1) {
      AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
      if (ev.keycode == AM_KEY_NONE) break;
      printf("Got  (kbd): %s (%d) %s\n", names[ev.keycode], ev.keycode, ev.keydown ? "DOWN" : "UP");
    }
  }
}

void keyboard_test() {
  printf("Try to press any key (uart or keyboard)...\n");
  has_uart = io_read(AM_UART_CONFIG).present;
  has_kbd  = io_read(AM_INPUT_CONFIG).present;
  while (1) {
    drain_keys();
  }
}
```
```c
#define AM_KEYS(_) \
  _(ESCAPE) _(F1) _(F2) _(F3) _(F4) _(F5) _(F6) _(F7) _(F8) _(F9) _(F10) _(F11) _(F12) \
  _(GRAVE) _(1) _(2) _(3) _(4) _(5) _(6) _(7) _(8) _(9) _(0) _(MINUS) _(EQUALS) _(BACKSPACE) \
  _(TAB) _(Q) _(W) _(E) _(R) _(T) _(Y) _(U) _(I) _(O) _(P) _(LEFTBRACKET) _(RIGHTBRACKET) _(BACKSLASH) \
  _(CAPSLOCK) _(A) _(S) _(D) _(F) _(G) _(H) _(J) _(K) _(L) _(SEMICOLON) _(APOSTROPHE) _(RETURN) \
  _(LSHIFT) _(Z) _(X) _(C) _(V) _(B) _(N) _(M) _(COMMA) _(PERIOD) _(SLASH) _(RSHIFT) \
  _(LCTRL) _(APPLICATION) _(LALT) _(SPACE) _(RALT) _(RCTRL) \
  _(UP) _(DOWN) _(LEFT) _(RIGHT) _(INSERT) _(DELETE) _(HOME) _(END) _(PAGEUP) _(PAGEDOWN)

#define AM_KEY_NAMES(key) AM_KEY_##key,
enum {
  AM_KEY_NONE = 0,
  AM_KEYS(AM_KEY_NAMES)
};
```
AM_KEYS(AM_KEY_NAMES)=AM_KEYS_NAMES(F1)...
AM_KEYS(NAMEINIT)= NAMEINIT(F1)...=[ AM_KEY_F1 ] ="F1";
 ![alt text](image-63.png)
**27.实现单种颜色的显示**
![alt text](image-64.png)
为了高复用性，写在/home/kirin/Desktop/linux/ysyx-workbench/am-kernels/tests/am-tests/src/tests里，并在main里添加规则  
![alt text](image-65.png)
因为
```c
#define IOE ({ ioe_init();  })
```
在/home/kirin/Desktop/linux/ysyx-workbench/am-kernels/tests/am-tests/include/amtest.h，所以可以删掉框架对IOE初始化
```c
#include <am.h>
#include <klib-macros.h>
static uint32_t color_buf[400*300];
void draw(uint32_t color) {
  // change the code below
  int w = 400;
  int h = 300;
  int x, y;
  for (y = 0; y < h; y ++) {
    for (x = 0; x < w; x ++) {
        color_buf[x+y*w] = color;
    }
  }
  io_write(AM_GPU_FBDRAW, 0, 0, color_buf, w, h, true);//AM_DEVREG(11, GPU_FBDRAW,   WR, int x, y; void *pixels; int w, h; bool sync);
}
int screensaver_test() {
  while (1) {
    draw(0x000000ff);
  }
  return 0;
}
```
![alt text](image-66.png)
**28.实现颜色渐变效果**
![alt text](image-67.png)
```c
#include <am.h>
#include <klib-macros.h>
#include <stdlib.h>
static uint32_t color_buf[400*300];
static uint32_t color[8]={0x000000, 0xff0000, 0x00ff00, 0x0000ff, 0xffff00, 0xff00ff, 0x00ffff, 0xffffff};
int change_fre=8;
void draw(uint32_t color) {
  // change the code below
  int w = 400;
  int h = 300;
  int x, y;
  for (y = 0; y < h; y ++) {
    for (x = 0; x < w; x ++) {
        color_buf[x+y*w] = color;
    }
  }
  io_write(AM_GPU_FBDRAW, 0, 0, color_buf, w, h, true);//AM_DEVREG(11, GPU_FBDRAW,   WR, int x, y; void *pixels; int w, h; bool sync);
}
uint32_t choose(int sec)
  { static int num=0;
    static int R=0;
    static int R1=0;
    static int G=0;
    static int G1=0;
    static int B=0;
    static int B1=0;
    int i=sec%change_fre;
    int Ri=R+(R1-R)*i/change_fre;
    int Gi=G+(G1-G)*i/change_fre;
    int Bi=B+(B1-B)*i/change_fre;
    if(i==1)
    {
      num=rand()%8;
      R=R1;
      G=G1;
      B=B1;
      B1=color[num]&0xff;
      G1=color[num]>>8&0xff;
      R1=color[num]>>16&0xff;
    }
    return (Ri<<16)+(Gi<<8)+Bi;
  }
int screensaver_test() {
  int sec=1;
  srand(io_read(AM_TIMER_UPTIME).us);
  while (1) {
    uint32_t color;
    while(io_read(AM_TIMER_UPTIME).us / 1000000 < sec);
    color=choose(sec);
    draw(color);
    sec++;
  } 
  return 0;
}
```
![alt text](image-68.png)
**29.添加按键效果**
![alt text](image-69.png)
```c
#include <am.h>
#include <klib-macros.h>
#include <stdlib.h>
static uint32_t color_buf[400*300];
static uint32_t color[8]={0x000000, 0xff0000, 0x00ff00, 0x0000ff, 0xffff00, 0xff00ff, 0x00ffff, 0xffffff};
int change_fre=8;
static bool has_kbd;
void draw(uint32_t color) {
  // change the code below
  int w = 400;
  int h = 300;
  int x, y;
  for (y = 0; y < h; y ++) {
    for (x = 0; x < w; x ++) {
        color_buf[x+y*w] = color;
    }
  }
  io_write(AM_GPU_FBDRAW, 0, 0, color_buf, w, h, true);//AM_DEVREG(11, GPU_FBDRAW,   WR, int x, y; void *pixels; int w, h; bool sync);
}
uint32_t choose(int step)
  { static int num=0;
    static int R=0;
    static int R1=0;
    static int G=0;
    static int G1=0;
    static int B=0;
    static int B1=0;
    int i=step%change_fre;
    int Ri=R+(R1-R)*i/change_fre;
    int Gi=G+(G1-G)*i/change_fre;
    int Bi=B+(B1-B)*i/change_fre;

    if(i==1)
    {
      num=rand()%8;
      R=R1;
      G=G1;
      B=B1;
      B1=color[num]&0xff;
      G1=color[num]>>8&0xff;
      R1=color[num]>>16&0xff;
    }
    return (Ri<<16)+(Gi<<8)+Bi;
  }
int screensaver_test() {
  has_kbd= io_read(AM_INPUT_CONFIG).present;
  int step=0;
  int speed=1;
  int sec=2;
  uint64_t end=0;
  uint64_t start=io_read(AM_TIMER_UPTIME).us;
  srand(start);
  while (1) {
    uint32_t color;
    if (has_kbd) {
      AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
      if (ev.keycode == AM_KEY_ESCAPE) break;
      if((ev.keycode!=AM_KEY_NONE)&&ev.keydown)
      {
        speed=4;
      }
    if((ev.keycode==AM_KEY_NONE)&&(!ev.keydown))
    {
      speed=1;
    }
  }
     end=io_read(AM_TIMER_UPTIME).us;
     if((end-start)>= 1000000*sec/speed){
      start=end;
      color=choose(step);
      draw(color);
      step++;
     }
  
}
  return 0;
}
```
**30.添加图形显示功能**
![alt text](image-70.png)
依据f6,可以知道成功要求的是256x256,屏幕像素对应的存储区域是[0x20000000, 0x20040000).
```c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <am.h>
#include <klib-macros.h>
uint32_t PC = 0;
uint32_t R[16]={0};
uint32_t M[1000000]={0};
static uint32_t color_buf[256*256]={0};
bool end_the_program=false;
int load(char* filename)
{
  FILE * fp=fopen(filename,"rb");
  if(!fp)
  {
    printf("unable to load %s",filename);
    return -1;
  }
  fseek(fp,0,SEEK_END);
  long file_size=ftell(fp);
  fseek(fp,0,SEEK_SET);
  if(file_size>sizeof(M))
  {
    printf("file is too big");
    fclose(fp);
    return -1;
  }
  fread(M,4,file_size/4,fp);
  return 0;
}
void inst_cycle()
   {
    uint32_t inst=M[PC/4];
    uint32_t imm_I=inst>>20&0xfff;
    uint32_t funct12=imm_I;
    uint32_t sign_extend_imm_I=(imm_I&0x800)?imm_I|0xfffff000:imm_I;
    uint32_t imm_U=inst>>12&0xfffff;
    uint32_t imm_S=((inst>>25&0x7f)<<5)+(inst>>7&0x1f);
    uint32_t sign_extend_imm_S=(imm_S&0x800)?imm_S|0xfffff000:imm_S;
    uint32_t opcode=inst&0x7f;
    uint32_t rs1=inst>>15&0x1f;
    uint32_t rs2=inst>>20&0x1f;
    uint32_t rd=inst>>7&0x1f;
    uint32_t funct3=inst>>12&0x7;
    uint32_t funct7=inst>>25&0x7f;
    switch(opcode)
    {   case 0b0000011:
            if(funct3==0b010)//lw
            {    uint32_t addr=R[rs1]+sign_extend_imm_I;
                if((addr&0x3)==0)
                {
                  if(rd!=0){
                  R[rd]=M[addr/4];
                  }
               }
                else{
                  printf("misaligned!");
                }
               PC+=4;
            }
            else if(funct3==0b100)//lbu
            {
              if(rd!=0)
              { uint32_t addr=R[rs1]+sign_extend_imm_I;
                uint32_t byte_offset = addr & 0x3;
                uint32_t word=M[addr/4];
                R[rd]=word>>(byte_offset*8)&0xff;
              }
              PC+=4;
            }
            else{
                printf("unknown funct3\n");
            }
            break;
        case 0b0010011:
            if(funct3==0b000)//is_addi = (inst[6:0] == 0010011) && (inst[14:12] == 000)，addi rd, rs1, imm
              { if(rd!=0)
                {R[rd]=R[rs1]+sign_extend_imm_I;}
                PC+=4;
              }
              else{
                printf("unknown funct3\n");
              }
              break;
        case 0b0100011:
              if(funct3==0b000)//sb
              { uint32_t addr=R[rs1]+sign_extend_imm_S;
                uint32_t offset=addr&0x3;
                M[addr/4]=(R[rs2]<<(8*offset))+(M[addr/4]&(~(0xff<<(8*offset))));
                PC+=4;
              }
              else if(funct3==0b010)//sw
              {   
                uint32_t addr=R[rs1]+sign_extend_imm_S;
                 if((addr&0x3)==0)
                {
                  if(addr>=0x20000000&&addr<0x20040000)//[0x20000000, 0x20040000)
                  {
                    color_buf[(addr-0x20000000)/4]=R[rs2];
                  }
                  else
                  {
                    M[addr/4]=R[rs2];
                  }
                  PC+=4;
              }
                else{
                  printf("misaligned!");
                  PC+=4;
                }
              }
              else{
                 printf("unknown funct3\n");
              }
              break;
        case 0b0110011://add
             if(funct3==0b000&&funct7==0b0000000)
            {   if(rd!=0)
                {R[rd]=R[rs1]+R[rs2];}
                PC+=4;
            }
               else
               { printf("unknown funct3 or funct7\n");}
                break;
        case 0b0110111://lui R[rd]=imm20
            if(rd!=0)
            {R[rd]=imm_U<<12;}
            PC+=4;
            break;
        case 0b1100111:
        if(funct3==0b000)  //is_jalr = (inst[6:0] == 1100111) && (inst[14:12] == 000),X[rd] = PC + 4,PC = (X[rs1] + sign_extend(offset)) & ~1
            {  
              uint32_t addr=(R[rs1]+sign_extend_imm_I)&(~1);
              if(rd!=0)R[rd]=PC+4;
              PC=addr;
               }
           else{
              printf("unknown funct3\n");}
              break;
        case 0b1110011://ebreak
        if(funct12==0b000000000001&&funct3==0&&rd==0&&rs1==0)  
            {
              printf("end of the program!\n");     
              end_the_program=true;
            }
            break;  
        default:
          printf("unknown inst\n");


    }
}

void minirv_test()
{   char* filename="vga.bin";
    uint32_t halt=0xdb0;
    if(load(filename)==0)
    {
    M[halt/4+1]=0x00100073;
    printf("load program successful\n");
    while(end_the_program==false) { inst_cycle(); }
    }
    io_write(AM_GPU_FBDRAW, 200-128, 150-128, color_buf, 256, 256, true);//AM_DEVREG(11, GPU_FBDRAW,   WR, int x, y; void *pixels; int w, h; bool sync);
    while(1);
}
```
![alt text](image-71.png)