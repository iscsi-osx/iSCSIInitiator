/*!
 * @author		Nareg Sinenian (implementation in crc32c.c by Mark Adler)
 * @file		crc32c.h
 * @date		December 10, 2014
 * @version		1.0
 * @copyright	(c) 2014 Nareg Sinenian. All rights reserved.
 *
 * Header for Mark Adler's CRC32C computation (crc32c.c)
 */

#ifndef __ISCSI_INITIATOR_CRC32C_H__
#define __ISCSI_INITIATOR_CRC32C_H__

#include <IOKit/IOLib.h>

/*! Call once to initialize CRC32C. */
void crc32c_init();

/*! Computes the CRC32C checksum of data.
 *  @param crc the existing crc for prior data, if any,.
 *  @param buffer the buffer to compute
 *  @param length the length of the buffer
 *  @return the new CRC32C checksum. */
uint32_t crc32c(uint32_t crc,const void * buffer,size_t length);

#endif
