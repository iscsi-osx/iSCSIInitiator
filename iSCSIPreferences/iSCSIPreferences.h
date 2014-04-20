//
//  iSCSIPreferences.h
//  iSCSIPreferences
//
//  Created by Nareg Sinenian on 10/20/13.
//
//

#import <PreferencePanes/PreferencePanes.h>

@interface iSCSIPreferences : NSPreferencePane

-(void)mainViewDidLoad;

@property (strong) IBOutlet NSSegmentedControl *addOrRemoveTargets;

@property (strong) IBOutlet NSArrayController *listOfTargets;

@end


