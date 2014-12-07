//
//  crc32c.h
//  iSCSI_Initiator
//
//  Created by Nareg Sinenian on 12/6/14.
//
//

#ifndef __iSCSI_Initiator__crc32c__
#define __iSCSI_Initiator__crc32c__

#include <IOKit/IOLib.h>

void crc32c_init(void);

uint32_t crc32c(uint32_t crc, const void *buf, size_t len);

#endif /* defined(__iSCSI_Initiator__crc32c__) */
