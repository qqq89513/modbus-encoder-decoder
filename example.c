
#include <stdio.h>

#include "modbus.h"

int main(){
  uint8_t payload[128];
  int len = 0;
  
  // len = modbus_read_bits_gen(0, 0x1234, 0x45, payload);
  // len = modbus_read_input_bits_gen(0, 0x1234, 0x45, payload);
  // len = modbus_read_registers_gen(0, 0x1234, 0x45, payload);
  // len = modbus_read_input_registers_gen(0x32, 0x1234, 0x45, payload);
  // len = modbus_write_bit_gen(0x12, 0x1234, TRUE, payload);
  len = modbus_write_register_gen(0x76, 0x5678, 0x2233, payload);
  printf("payload[]=0x");
  for(int i=0; i<len; i++){
    printf("%02X", payload[i]);
  }
  printf("\n");

  printf("The end of program.");
}

