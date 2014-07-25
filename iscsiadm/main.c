/*!
 * @author		Nareg Sinenian
 * @file		main.h
 * @date		June 29, 2014
 * @version		1.0
 * @copyright	(c) 2014 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI management utility.
 */

#include <CoreFoundation/CoreFoundation.h>

CFMutableDictionaryRef args = NULL;

/*! Parses command line arguments into a lookup dictionary.
 *  @param argc argument count.
 *  @param argv command-line arguments.
 *  @param argDict a dictionary of command-line arguments. */
void parseArguments(int argc,char * argv[],CFMutableDictionaryRef * argDict)
{
    if(argc == 0)
        return;
    
    argDict = CFDictionaryCreateMutable(kCFAllocatorDefault,argc,NULL,NULL);
    
    for(unsigned int i = 0; i < argc; i++)
    {
        argv[i]
        
        
    }
    
}

/*! Displays a list of valid command-line arguments. */
void displayHelp()
{
    
    
}

/*! Entry point.  Parses command line arguments, establishes a connection to the
 *  iSCSI deamon and executes requested iSCSI tasks. */
int main(int argc, const char * argv[])
{
    // Validate arguments
    if(argc == 0)
        displayHelp();
    else
        parseArguments(argc,argv);

    // insert code here...
    CFShow(CFSTR("Hello, World!\n"));
    return 0;
}




