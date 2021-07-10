/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library implements the Modbus protocol.
 * http://libmodbus.org/
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "modbus.h"
#include "modbus-rtu-private.h"

/* Internal use */
#define MSG_LENGTH_UNDEFINED -1


/* Max between RTU and TCP max adu length (so TCP) */
#define MAX_MESSAGE_LENGTH 260

/* 3 steps are used to parse the query */
typedef enum {
  _STEP_FUNCTION,
  _STEP_META,
  _STEP_DATA
} _step_t;

const char *modbus_strerror(int errnum) {
  switch (errnum) {
    // Modbus error code
    case EMBXILFUN:     return "Illegal function";
    case EMBXILADD:     return "Illegal data address";
    case EMBXILVAL:     return "Illegal data value";
    case EMBXSFAIL:     return "Slave device or server failure";
    case EMBXACK:       return "Acknowledge";
    case EMBXSBUSY:     return "Slave device or server is busy";
    case EMBXNACK:      return "Negative acknowledge";
    case EMBXMEMPAR:    return "Memory parity error";
    case EMBXGPATH:     return "Gateway path unavailable";
    case EMBXGTAR:      return "Target device failed to respond";
    
    // Native error code
    case EMBBADCRC:     return "Invalid CRC";
    case EMBBADDATA:    return "Invalid data";
    case EMBBADEXC:     return "Invalid exception code";
    case EMBMDATA:      return "Too many data";
    case EMBBADSLAVE:   return "Response not from requested slave";
    default:
      return strerror(errnum);
    }
}


/* Builds a RTU request header */
static int _modbus_rtu_build_request_basis(uint8_t unit, uint8_t function, uint16_t addr, uint8_t nb, uint8_t *req){
  req[0] = unit;
  req[1] = function;
  req[2] = addr >> 8;
  req[3] = addr & 0x00ff;
  req[4] = nb >> 8;
  req[5] = nb & 0x00ff;

  return _MODBUS_RTU_PRESET_REQ_LENGTH;
}

/** This method calculates CRC-16/modus and returns in (hi-byte, lo-byte)
 * @param buf[]: Target to calculates crc from
 * @param len: The length of buf[]
 * @return CRC-16/modbus: (hi-byte, lo-byte)
 *         CRC-Hi = return >> 8
 *         CRC-Lo = return & 0x00FF
 */
static uint16_t _calc_CRC(uint8_t buf[], uint8_t len){
  unsigned int crc, flag;
  crc = 0xFFFF;
  for(uint8_t i = 0; i < len; i++)
  {
    crc = crc ^ buf[i];
    for(uint8_t j = 1; j <= 8; j++)
    {
      flag = crc & 0x0001;
      crc >>=1;
      if (flag)
        crc ^= 0xA001;
    }
  }
  return crc;
}

/** This method calculates CRC-16/modus and concatenate it to the end of buf[]
 * @param buf[]: Make sure that buf has the length of len+2. 
 *               The last 2 bytes are for (CRC_L, CRC_H)
 * @param len: The length of payload consist in buf[]
 * @return len+2, the length of entire packet, including crc checksum
 */
static int _CRC_concatenate(uint8_t buf[], uint8_t len){
  
  // Calculate CRC-16/modbus
  unsigned int temp;
  temp = _calc_CRC(buf, len);
  
  // Concatenate crc to the end of buf[]
  buf[len]    = temp & 0x00FF;    // CRC-Lo
  buf[len+1]  = temp >> 8;        // CRC-Hi

  return len+2;
}

void _error_print(modbus_t *ctx, const char *context)
{
  if(MODBUS_DEBUG){
    fprintf(stderr, "ERROR %s", modbus_strerror(errno));
    if (context != NULL) {
      fprintf(stderr, ": %s\n", context);
    } else {
      fprintf(stderr, "\n");
    }
  }
}

/** Generate a modbus RTU payload to read coils and stored the payload in ADU
 * @param unit: Unit of slave, aka additional address
 * @param addr: Start from this physical address (0~65535)
 * @param nb: Quantity of bits(coils)
 * @param ADU: byte array to keep the payload
 * 
 * @param return: length of ADU[]. Accessible: ADU[0~retrun-1]
*/
int modbus_read_bits_gen(uint8_t unit, uint16_t addr, uint8_t nb, uint8_t ADU[]){
  int len = 0;  // The length of ADU

  // Check parameters  
  if (nb > MODBUS_MAX_READ_BITS) {
    if (MODBUS_DEBUG) {
        fprintf(stderr,
                "ERROR Too many bits requested (%d > %d)\n",
                nb, MODBUS_MAX_READ_BITS);
    }
    errno = EMBMDATA;
    return -1;
  }

  // Payload generation
  len = _modbus_rtu_build_request_basis(unit, MODBUS_FC_READ_COILS, addr, nb, ADU);
  len = _CRC_concatenate(ADU, len);
  
  return len;
}

/** Generate a modbus RTU payload to read discretes and stored the payload in ADU
 * @param unit: Unit of slave, aka additional address
 * @param addr: Start from this physical address (0~65535)
 * @param nb: Quantity of bits(discretes)
 * @param ADU: byte array to keep the payload
 * 
 * @param return: length of ADU[]. Accessible: ADU[0~retrun-1]
*/
int modbus_read_input_bits_gen(uint8_t unit, uint16_t addr, uint8_t nb, uint8_t ADU[]){
  int len = 0;  // The length of ADU

  // Check parameters  
  if (nb > MODBUS_MAX_READ_BITS) {
    if (MODBUS_DEBUG) {
        fprintf(stderr,
                "ERROR Too many bits requested (%d > %d)\n",
                nb, MODBUS_MAX_READ_BITS);
    }
    errno = EMBMDATA;
    return -1;
  }

  // Payload generation
  len = _modbus_rtu_build_request_basis(unit, MODBUS_FC_READ_DISCRETE_INPUTS, addr, nb, ADU);
  len = _CRC_concatenate(ADU, len);
  
  return len;

}

/** Generate a modbus RTU payload to read holding registers and stored the payload in ADU
 * @param unit: Unit of slave, aka additional address
 * @param addr: Start from this physical address (0~65535)
 * @param nb: Quantity of words(registers, 2 bytes)
 * @param ADU: byte array to keep the payload
 * 
 * @param return: length of ADU[]. Accessible: ADU[0~retrun-1]
*/
int modbus_read_registers_gen(uint8_t unit, uint16_t addr, uint8_t nb, uint8_t ADU[]){
  int len = 0;  // The length of ADU

  // Check parameters  
  if (nb > MODBUS_MAX_READ_REGISTERS) {
    if (MODBUS_DEBUG) {
      fprintf(stderr,
              "ERROR Too many registers requested (%d > %d)\n",
              nb, MODBUS_MAX_READ_REGISTERS);
    }
    errno = EMBMDATA;
    return -1;
  }

  // Payload generation
  len = _modbus_rtu_build_request_basis(unit, MODBUS_FC_READ_HOLDING_REGISTERS, addr, nb, ADU);
  len = _CRC_concatenate(ADU, len);
  
  return len;
}

/** Generate a modbus RTU payload to read input registers and stored the payload in ADU
 * @param unit: Unit of slave, aka additional address
 * @param addr: Start from this physical address (0~65535)
 * @param nb: Quantity of words(registers, 2 bytes)
 * @param ADU: byte array to keep the payload
 * 
 * @param return: length of ADU[]. Accessible: ADU[0~retrun-1]
*/
int modbus_read_input_registers_gen(uint8_t unit, uint16_t addr, uint8_t nb, uint8_t ADU[]){
  int len = 0;  // The length of ADU

  // Check parameters  
  if (nb > MODBUS_MAX_READ_REGISTERS) {
    if (MODBUS_DEBUG) {
      fprintf(stderr,
              "ERROR Too many registers requested (%d > %d)\n",
              nb, MODBUS_MAX_READ_REGISTERS);
    }
    errno = EMBMDATA;
    return -1;
  }

  // Payload generation
  len = _modbus_rtu_build_request_basis(unit, MODBUS_FC_READ_INPUT_REGISTERS, addr, nb, ADU);
  len = _CRC_concatenate(ADU, len);
  
  return len;
}

/** Generate a modbus RTU payload to wrtie single bit to coil status and stored the payload in ADU
 * @param unit: Unit of slave, aka additional address
 * @param addr: Start from this physical address (0~65535)
 * @param status: 1 for true and 0 for false
 * @param ADU: byte array to keep the payload
 * 
 * @param return: length of ADU[]. Accessible: ADU[0~retrun-1]
*/
int modbus_write_bit_gen(uint8_t unit, uint16_t addr, int status, uint8_t ADU[]){
  int len = 0;  // The length of ADU
  uint16_t value = status ? 0xFF00 : 0x0000; // 0xFF00 for true, 0x0000 for false
  // Payload generation
  len = _modbus_rtu_build_request_basis(unit, MODBUS_FC_WRITE_SINGLE_COIL, addr, value, ADU);
  len = _CRC_concatenate(ADU, len);
  
  return len;
}

/** Generate a modbus RTU payload to wrtie 2 bytes to single register and stored the payload in ADU
 * @param unit: Unit of slave, aka additional address
 * @param addr: Start from this physical address (0~65535)
 * @param value: Value of 2 bytes to write to a single register
 * @param ADU: byte array to keep the payload
 * 
 * @param return: length of ADU[]. Accessible: ADU[0~retrun-1]
*/
int modbus_write_register_gen(uint8_t unit, uint16_t addr, const uint16_t value, uint8_t ADU[]){
  int len = 0;  // The length of ADU
  // Payload generation
  len = _modbus_rtu_build_request_basis(unit, MODBUS_FC_WRITE_SINGLE_REGISTER, addr, value, ADU);
  len = _CRC_concatenate(ADU, len);
  
  return len;
}

/** Generate a modbus RTU payload to write bits to multiple coil statuses and stored the payload in ADU
 * @param unit: Unit of slave, aka additional address
 * @param addr: Start from this physical address (0~65535)
 * @param nb: Quantity of bits (coils)
 * @param data: Bits to write. A byte array, 1 byte for 1 boolean
 * @param ADU: byte array to keep the payload
 * 
 * @param return: length of ADU[]. Accessible: ADU[0~retrun-1]
*/
int modbus_write_bits_gen(uint8_t unit, uint16_t addr, uint8_t nb, const uint8_t data[], uint8_t ADU[]){
  int byte_count;
  int len;
  int bit_check = 0;
  int pos = 0;

  // Check parameters  
  if (nb > MODBUS_MAX_WRITE_BITS) {
    if (MODBUS_DEBUG) {
      fprintf(stderr, "ERROR Writing too many bits (%d > %d)\n",
              nb, MODBUS_MAX_WRITE_BITS);
    }
    errno = EMBMDATA;
    return -1;
  }

  // Payload generation
  len = _modbus_rtu_build_request_basis(unit, MODBUS_FC_WRITE_MULTIPLE_COILS, addr, nb, ADU);
  byte_count = (nb / 8) + ((nb % 8) ? 1 : 0);
  ADU[len++] = byte_count;
  
  // Make booleans byte-array, data[], to bit-positioned byte-array
  // and follows modbus format (Hi byte first)
  for (int i=0; i<byte_count; i++) {
    int bit;

    bit = 0x01;
    ADU[len] = 0;

    while ((bit & 0xFF) && (bit_check++ < nb)) {
      if (data[pos++])
        ADU[len] |= bit;
      else
        ADU[len] &= ~bit;

      bit = bit << 1;
    }
    len++;
  }

  len = _CRC_concatenate(ADU, len);
  
  return len;
}

/** Generate a modbus RTU payload to write words(word = 2 bytes) to multiple registers and stored the payload in ADU
 * @param unit: Unit of slave, aka additional address
 * @param addr: Start from this physical address (0~65535)
 * @param nb: Quantity of words (registers)
 * @param data: Words (register values) to write. 1 word = 2 byte = sizeof(uint16_t)
 * @param ADU: byte array to keep the payload
 * 
 * @param return: length of ADU[]. Accessible: ADU[0~retrun-1]
*/
int modbus_write_registers_gen(uint8_t unit, uint16_t addr, uint8_t nb, const uint16_t data[], uint8_t ADU[]){
  int len;
  int byte_count;

  // Check parameters  
  if (nb > MODBUS_MAX_WRITE_REGISTERS) {
    if (MODBUS_DEBUG) {
      fprintf(stderr, "ERROR Writing too many bits (%d > %d)\n",
              nb, MODBUS_MAX_WRITE_REGISTERS);
    }
    errno = EMBMDATA;
    return -1;
  }

  // Payload header generation
  len = _modbus_rtu_build_request_basis(unit, MODBUS_FC_WRITE_MULTIPLE_REGISTERS, addr, nb, ADU);
  byte_count = nb * 2;
  ADU[len++] = byte_count;

  // Makes word-array to byte-array that's 2 times longer
  for (int i = 0; i < nb; i++) {
    ADU[len++] = data[i] >> 8;
    ADU[len++] = data[i] & 0x00FF;
  }

  len = _CRC_concatenate(ADU, len);
  
  return len;
}

// Return 0 if ok, -1 on if error, exception code otherwise
int modbus_ADU_parser(modbusframe_t *frame){

  unsigned int crc_expect = 0;
  unsigned int crc_receive = 0;

  frame->unit    = frame->ADU[0];
  frame->fn_code = frame->ADU[1];

  // Get total ADU length
  if(frame->fn_code & 0x80){
    frame->ADU_len = 5;  // unit(1), fn_code(1), exeception code(1), crc(2)
  }
  else{  
    switch(frame->fn_code) { // get PDU length
      // Reading response
      case MODBUS_FC_READ_COILS:
      case MODBUS_FC_READ_DISCRETE_INPUTS:
      case MODBUS_FC_READ_HOLDING_REGISTERS:
      case MODBUS_FC_READ_INPUT_REGISTERS:
        frame->ADU_len = 2 + frame->ADU[2]; // fn_code(1), byte_cnt(1), bytes(N)
        break;

      // Writing response
      case MODBUS_FC_WRITE_SINGLE_COIL:
      case MODBUS_FC_WRITE_SINGLE_REGISTER:
      case MODBUS_FC_WRITE_MULTIPLE_COILS:
      case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
        frame->ADU_len = 5;  // fn_code(1), start addr(2), quantity(2)
        break;

      default:
        if(MODBUS_DEBUG)
          fprintf(stderr, "FATAL Unknow function code:0x%X\n", frame->fn_code);
    }
    frame->ADU_len += 3; // 3 more bytes: unit(1), CRC(2)
  }
  // Check for crc
  crc_expect = _calc_CRC(frame->ADU, frame->ADU_len-2); // -2 because the last 2 byte is crc received
  crc_receive = frame->ADU[frame->ADU_len-1]<<8 | frame->ADU[frame->ADU_len-2] ;
  if(crc_expect != crc_receive){ // CRC error
    errno = EMBBADCRC;
    if(MODBUS_DEBUG)
      fprintf(stderr, "ERROR Invalid CRC. Expect 0x%X, got 0x%X\n",
              crc_expect, crc_receive);
    return -1;
  }

  // Check for exception function code
  if(frame->fn_code & 0x80){
    errno = MODBUS_ENOBASE + frame->ADU[2];
    if(MODBUS_DEBUG){
      fprintf(stderr, "ERROR Exception code 0x%02X: %s, function code:0x%02X\n", 
              frame->ADU[2], modbus_strerror(errno), frame->fn_code & 0x7F);
    }
    return frame->ADU[2]; // The #2 byte in ADU is exception code
  }

  return 0;
}

