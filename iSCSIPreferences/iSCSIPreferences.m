//
//  iSCSIPreferences.m
//  iSCSIPreferences
//
//  Created by Nareg Sinenian on 10/20/13.
//
//

#import "iSCSIPreferences.h"
#include <IOKit/IOKitLib.h>
#include "iSCSIInitiatorInterface.h"

@implementation iSCSIPreferences

- (void)mainViewDidLoad
{
}

- (void)didSelect
{
	
}

- (IBAction)targetsModified:(id)sender {
	
	if([_addOrRemoveTargets selectedSegment] == 0)
		[_listOfTargets add:sender];
	else if([_addOrRemoveTargets selectedSegment] == 1)
		[_listOfTargets remove:sender];
}

@end
