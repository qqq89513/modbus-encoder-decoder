
#include <stdio.h>

#include "modbus.h"

int main(){
  uint8_t payload[128];
  // uint8_t data_bits[] = {1,0,0,0,0,1,0,0, 0,0,1,0,0,0,0,1};
  uint16_t data_regs[] = {0xABCD, 0xEF01, 0x1234};
  int len = 0;
  
  // len = modbus_read_bits_gen(0, 0x1234, 0x45, payload);
  // len = modbus_read_input_bits_gen(0, 0x1234, 0x45, payload);
  // len = modbus_read_registers_gen(0, 0x1234, 0x45, payload);
  // len = modbus_read_input_registers_gen(0x32, 0x1234, 0x45, payload);
  // len = modbus_write_bit_gen(0x12, 0x1234, TRUE, payload);
  // len = modbus_write_register_gen(0x76, 0x5678, 0x2233, payload);
  // len = modbus_write_bits_gen(0x56, 0x1234, sizeof(data_bits), data_bits, payload);
  len = modbus_write_registers_gen(0x37, 0x8888, sizeof(data_regs)/sizeof(uint16_t), data_regs, payload);
  printf("sizeof(data)=%I64d\n", sizeof(data_regs));
  printf("payload[]=0x");
  for(int i=0; i<len; i++){
    printf("%02X", payload[i]);
  }
  printf("\n");

  printf("The end of program.");
}

