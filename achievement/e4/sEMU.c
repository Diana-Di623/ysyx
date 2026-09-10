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
int main(int argc,char *argv[])
{  
     R[0]=atoi(argv[1]);
    while(PC<=6) { inst_cycle(); }
}