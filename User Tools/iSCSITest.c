
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
    
    
    CFStringRef test = CFSTR("hello");
    CFStringRef blah = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%@"),test);
    int c = 0;

    
    return 0;
}
