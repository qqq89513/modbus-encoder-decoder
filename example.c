
#include <stdio.h>

#include "modbus.h"

enum TEST_FUNCS {
  TEST_READ_BITS = 0,
  TEST_READ_INPUT_BITS,
  TEST_READ_REGISTERS,
  TEST_READ_INPUT_REGISTERS,
  TEST_WRITE_BIT,
  TEST_WRITE_BITS,
  TEST_WRITE_REGISTER,
  TEST_WRITE_REGISTERS,
};

enum TEST_FUNCS FUNCTION_TO_TEST = TEST_WRITE_BIT;

int main(){
  int len = 0;
  int unit = 0x3F;
  int start_addr = 0x3212;
  int number = 10;
  uint8_t payload[128];
  uint8_t *response;
  uint8_t  bits_to_write[] = {1,0,0,0,0,1,0,0, 0,0,1,0,0,0,0,1};
  uint16_t regs_to_write[] = {0xABCD, 0xEF01, 0x1234};
  int ret_code = 0;
  modbus_res_frame_t res;
  modbus_res_data_t data_dest;
  uint8_t bits_to_read[64];
  uint16_t regs_to_read[10];
  data_dest.bits = bits_to_read;
  data_dest.registers = regs_to_read;
  res.data = &data_dest;

  // Request generation
  switch(FUNCTION_TO_TEST){
    case TEST_READ_BITS:
      len = modbus_read_bits_gen(unit, start_addr, number, payload);
      break;
    case TEST_READ_INPUT_BITS:
      len = modbus_read_input_bits_gen(unit, start_addr, number, payload);
      break;
    case TEST_READ_REGISTERS:
      len = modbus_read_registers_gen(unit, start_addr, number, payload);
      break;
    case TEST_READ_INPUT_REGISTERS:
      len = modbus_read_input_registers_gen(unit, start_addr, number, payload);
      break;
    case TEST_WRITE_BIT:
      len = modbus_write_bit_gen(unit, start_addr, 1, payload);
      break;
    case TEST_WRITE_BITS:
      len = modbus_write_bits_gen(unit, start_addr, sizeof(bits_to_write), bits_to_write, payload);
      break;
    case TEST_WRITE_REGISTER:
      len = modbus_write_register_gen(unit, start_addr, 0x2233, payload);
      break;
    case TEST_WRITE_REGISTERS:
      len = modbus_write_registers_gen(unit, start_addr, sizeof(regs_to_write)/sizeof(uint16_t), regs_to_write, payload);
      break;

    default:
      printf("Error, FUNCTION_TO_TEST=%d, which is unknown.\n", FUNCTION_TO_TEST);
  }

  // Print the request payload generated
  printf("payload[]=0x");
  for(int i=0; i<len; i++){
    printf("%02X", payload[i]);
  }
  printf("\n");

  // Mock response
  switch(FUNCTION_TO_TEST){
    case TEST_READ_BITS:
      response = (uint8_t*)"\x12\x01\x03\xCD\x68\x05\x40\xD1";  // Read Coils
      break;
    case TEST_READ_INPUT_BITS:
      response = (uint8_t*)"\x32\x02\x03\xAC\xDB\x35\x27\x4B"; // Read discrete inputs
      break;
    case TEST_READ_REGISTERS:
      response = (uint8_t*)"\x02\x03\x06\x02\x2B\x00\x00\x00\x64\x11\x8A"; // Read holding registers
      break;
    case TEST_READ_INPUT_REGISTERS:
      response = (uint8_t*)"\x02\x04\x08\x00\x0A\x12\xFE\x12\xDA\x7A\x8A\x2D\xAB"; // Read input registers
      break;
    case TEST_WRITE_BIT:
      response = (uint8_t*)"\x01\x05\x12\x34\xFF\x00\xC8\x8C"; // Write single coil status
      break;
    case TEST_WRITE_BITS:
      response = (uint8_t*)"\x01\x0F\x12\x34\x02\x12\x90\x10"; // Write multiple coil status
      break;
    case TEST_WRITE_REGISTER:
      response = (uint8_t*)"\x01\x06\x12\x34\xFF\xE3\xCD\x05"; // Write single register
      break;
    case TEST_WRITE_REGISTERS:
      response = (uint8_t*)"\x01\x10\xAB\xCD\x00\x32\xF0\x07"; // Write multiple registers
      break;

    default:
      response = (uint8_t*)"\x01\x81\x01\x81\x90"; // Exception: Illegal function
  }

  res.num_reads = 24;

  res.ADU = response;
  ret_code = modbus_ADU_parser(&res);
  
  if(ret_code == 0){

    switch (FUNCTION_TO_TEST) {
      case TEST_READ_BITS:
      case TEST_READ_INPUT_BITS:
        printf("Bits[]=");
        for(int i=0; i<res.num_reads; i++)    printf("%d ", res.data->bits[i]);
        break;

      case TEST_READ_REGISTERS:
      case TEST_READ_INPUT_REGISTERS:
        printf("Registers[]=");
        for(int i=0; i<res.num_reads; i++)    printf("0x%04X ", res.data->registers[i]);
        break;
      default:
        break;
    }
    printf("\nresponse[]=0x");
    for(int i=0; i<res.ADU_len; i++)      printf("%02X", response[i]);
    printf("\n");
    printf("ret_code=%d, unit=%02X, fn_code=%02X, \n", 
            ret_code, res.unit, res.fn_code);
  }
  else if (ret_code == -1)
    printf("Error: CRC incorrect.\n");
  else
    printf("Exception response \"%s\", exeception code=%d\n", 
      modbus_strerror(MODBUS_ENOBASE+ret_code), ret_code);

  printf("The end of program.");
}

