/* Author: Landon Fuller <landonf@plausiblelabs.com>
 * Copyright (c) 2008-2011 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 *
 * See the IOSSIM_LICENSE file in this directory for the license on the source code in this file.
 */

#import <Foundation/Foundation.h>
#import "iphonesimulatorremoteclient/iphonesimulatorremoteclient.h"
#import "version.h"

@interface iPhoneSimulator : NSObject <DTiPhoneSimulatorSessionDelegate> {
@private
  DTiPhoneSimulatorSystemRoot *sdkRoot;
  NSFileHandle *stdoutFileHandle;
  NSFileHandle *stderrFileHandle;
  DTiPhoneSimulatorSession *session;
  NSTimer *pidCheckingTimer;
  BOOL startOnly;
  BOOL exitOnStartup;
  BOOL shouldWaitDebugger;
  BOOL shouldStartDebugger;
  BOOL useGDB;
  BOOL verbose;
  BOOL alreadyPrintedData;
  BOOL retinaDevice;
  BOOL tallDevice;
}

- (id)init;
- (void)dealloc;
- (void)runWithArgc:(int)argc argv:(char **)argv;

- (void)createStdioFIFO:(NSFileHandle **)fileHandle ofType:(NSString *)type atPath:(NSString **)path;
- (void)removeStdioFIFO:(NSFileHandle *)fileHandle atPath:(NSString *)path;
- (void)stop;
- (void)checkPid:(NSTimer *)timer;
- (void)doExit:(int)errorCode;
- (void)changeDeviceType:(NSString *)family retina:(BOOL)retina isTallDevice:(BOOL)isTallDevice;

@end
