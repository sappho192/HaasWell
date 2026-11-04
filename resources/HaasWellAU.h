
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1 || TARGET_OS_VISION == 1
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#define IPLUG_AUVIEWCONTROLLER IPlugAUViewController_vHaasWell
#define IPLUG_AUAUDIOUNIT IPlugAUAudioUnit_vHaasWell
#import <HaasWellAU/IPlugAUViewController.h>
#import <HaasWellAU/IPlugAUAudioUnit.h>

//! Project version number for HaasWellAU.
FOUNDATION_EXPORT double HaasWellAUVersionNumber;

//! Project version string for HaasWellAU.
FOUNDATION_EXPORT const unsigned char HaasWellAUVersionString[];

@class IPlugAUViewController_vHaasWell;
