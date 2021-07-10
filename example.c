
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
  // uint8_t response[16] = {0x02, 0x02, 0x03, 0xac, 0xdb, 0x35, 0x22, 0xbb};
  // uint8_t response[16] = {0x02, 0x04, 0x02, 0x00, 0x00, 0xfd, 0x20};
  uint8_t response[16] ={0x01, 0x81, 0x01, 0x81, 0x90};
  int ret_code = 0;
  modbus_res_frame_t res;
  res.ADU = response;
  ret_code = modbus_ADU_parser(&res);
  switch (ret_code) {
  case 0:
    printf("response[]=0x");
    for(int i=0; i<res.ADU_len; i++)
      printf("%02X", response[i]);
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

