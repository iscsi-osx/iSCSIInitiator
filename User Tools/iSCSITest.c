
//
//  main.c
//  iSCSITest
//
//  Created by Nareg Sinenian on 12/13/14.
//
//

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <CoreFoundation/CoreFoundation.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>


#include <stdio.h>
#include <IOKit/IOKitLib.h>






int main(int argc, const char * argv[]) {
    
    CFStringRef k[] = {CFSTR("1"),CFSTR("2")};
    CFStringRef v[] = {CFSTR("3"),CFSTR("4")};
    CFDictionaryRef x = CFDictionaryCreate(kCFAllocatorDefault,&k,&v,2,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    const void * keys, * values;
    CFDictionaryGetKeysAndValues(x,&keys,&values);
    
    CFArrayCreate(kCFAllocatorDefault,&keys,, <#const CFArrayCallBacks *callBacks#>)
    return 0;
}
