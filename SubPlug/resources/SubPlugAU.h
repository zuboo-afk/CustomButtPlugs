
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#define IPLUG_AUVIEWCONTROLLER IPlugAUViewController_vSubPlug
#define IPLUG_AUAUDIOUNIT IPlugAUAudioUnit_vSubPlug
#import <SubPlugAU/IPlugAUViewController.h>
#import <SubPlugAU/IPlugAUAudioUnit.h>

//! Project version number for SubPlugAU.
FOUNDATION_EXPORT double SubPlugAUVersionNumber;

//! Project version string for SubPlugAU.
FOUNDATION_EXPORT const unsigned char SubPlugAUVersionString[];

@class IPlugAUViewController_vSubPlug;
