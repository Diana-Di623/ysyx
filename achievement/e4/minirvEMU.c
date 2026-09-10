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
        if(funct12==0b000000000001&&funct3==0&&rd==0&&rs1==0)  
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
{   char* filename="sum.bin";
    uint32_t halt=0x224;
    if(load(filename)==0)
    {
    M[halt/4+1]=0x00100073;
    printf("load program successful\n");
    while(end==false) { inst_cycle(); }
    }
}