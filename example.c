
#include <stdio.h>

#include "modbus.h"
// TODO: make examples into functions
int main(){

  /* Request generation test */
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

  /* Response parser test*/
  // uint8_t response[16] = {0x01, 0x81, 0x01, 0x81, 0x90}; // Exception: Illegal function
  // uint8_t response[16] = {0x02, 0x03, 0x06, 0x02, 0x2B, 0x00, 0x00, 0x00, 0x64, 0x11, 0x8a}; // Read holding registers
  uint8_t response[16] = {0x02, 0x04, 0x08, 0x00, 0x0a, 0x12, 0xfe, 0x12, 0xda, 0x7a, 0x8a, 0x2d, 0xab}; // Read input registers
  int ret_code = 0;
  modbus_res_frame_t res;
  modbus_res_data_t data_dest;
  uint8_t bits[10];
  uint16_t regs[10];
  data_dest.bits = bits;
  data_dest.registers = regs;
  res.data = &data_dest;

  res.ADU = response;
  ret_code = modbus_ADU_parser(&res);
  switch (ret_code) {
  case 0:
    printf("Registers[]=");
    for(int i=0; i<res.num_reads; i++)    printf("0x%04X ", res.data->registers[i]);
    printf("\nresponse[]=0x");
    for(int i=0; i<res.ADU_len; i++)      printf("%02X", response[i]);
    printf("\n");
    printf("ret_code=%d, unit=%02X, fn_code=%02X, \n", 
            ret_code, res.unit, res.fn_code);
    break;
  case -1:
    printf("Error CRC incorrect.\n");
    break;
  default:
    printf("Exception response \"%s\", exeception code=%d\n", 
      modbus_strerror(MODBUS_ENOBASE+ret_code), ret_code);
  }

  printf("The end of program.");
}

