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
    case EMBXILFUN:
      return "Illegal function";
    case EMBXILADD:
      return "Illegal data address";
    case EMBXILVAL:
      return "Illegal data value";
    case EMBXSFAIL:
      return "Slave device or server failure";
    case EMBXACK:
      return "Acknowledge";
    case EMBXSBUSY:
      return "Slave device or server is busy";
    case EMBXNACK:
      return "Negative acknowledge";
    case EMBXMEMPAR:
      return "Memory parity error";
    case EMBXGPATH:
      return "Gateway path unavailable";
    case EMBXGTAR:
      return "Target device failed to respond";
    case EMBBADCRC:
      return "Invalid CRC";
    case EMBBADDATA:
      return "Invalid data";
    case EMBBADEXC:
      return "Invalid exception code";
    case EMBMDATA:
      return "Too many data";
    case EMBBADSLAVE:
      return "Response not from requested slave";
    default:
      return strerror(errnum);
    }
}


/* Builds a RTU request header */
static int _modbus_rtu_build_request_basis(int unit, int function, int addr, int nb, uint8_t *req){
  req[0] = unit;
  req[1] = function;
  req[2] = addr >> 8;
  req[3] = addr & 0x00ff;
  req[4] = nb >> 8;
  req[5] = nb & 0x00ff;

  return _MODBUS_RTU_PRESET_REQ_LENGTH;
}

/** This method calculates CRC-16/modus and concatenate it to the end of buf[]
 * @param buf[]: Make sure that buf has the length of len+2. 
 *               The last 2 bytes are for (CRC_L, CRC_H)
 * @param len: The length of payload consist in buf[]
 * @return len+2, the length of entire packet, including crc checksum
 */
static int _calcCRC(uint8_t buf[], uint8_t len){
  unsigned int temp, flag;
  temp = 0xFFFF;
  for(uint8_t i = 0; i < len; i++)
  {
    temp = temp ^ buf[i];
    for(uint8_t j = 1; j <= 8; j++)
    {
      flag = temp & 0x0001;
      temp >>=1;
      if (flag)
        temp ^= 0xA001;
    }
  }
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

/** Generate a modbus RTU payload for read coils and stored it in ADU
 * @param unit: Unit of slave, aka additional address
 * @param addr: Start from this physical address (0~65535)
 * @param nb: Quantity of bits(coils)
*/
int modbus_read_bits_gen(int unit, int addr, int nb, uint8_t *ADU){
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
  len = _calcCRC(ADU, len);
  
  return len;
}

/** Generate a modbus RTU payload for read discretes and stored it in ADU
 * @param unit: Unit of slave, aka additional address
 * @param addr: Start from this physical address (0~65535)
 * @param nb: Quantity of bits(discretes)
*/
int modbus_read_input_bits_gen(int unit, int addr, int nb, uint8_t *ADU){
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
  len = _calcCRC(ADU, len);
  
  return len;

}

int modbus_read_registers_gen(int unit, int addr, int nb, uint8_t *ADU){
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
  len = _calcCRC(ADU, len);
  
  return len;

}
int modbus_read_input_registers_gen(int unit, int addr, int nb, uint8_t *ADU){

}
