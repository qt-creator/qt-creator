/* Author: Landon Fuller <landonf@plausiblelabs.com>
 * Copyright (c) 2008-2011 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 *
 * See the IOSSIM_LICENSE file in this directory for the license on the source code in this file.
 */
/* derived from https://github.com/phonegap/ios-sim */

#import <AppKit/AppKit.h>

#import "iphonesimulator.h"

/*
 * Runs the iPhoneSimulator backed by a main runloop.
 */
int main (int argc, char *argv[]) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    iPhoneSimulator *sim = [[iPhoneSimulator alloc] init];
    
    /* Execute command line handler */
    [sim runWithArgc: argc argv: argv];

    /* Run the loop to handle added input sources, if any */
    [[NSRunLoop mainRunLoop] run];

    [pool release];
    return 0;
}
