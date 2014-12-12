/* Author: Landon Fuller <landonf@plausiblelabs.com>
 * Copyright (c) 2008-2011 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 *
 * See the IOSSIM_LICENSE file in this directory for the license on the source code in this file.
 */

#import "iphonesimulator.h"
#import "nsstringexpandpath.h"
#import "nsprintf.h"
#import <sys/types.h>
#import <sys/stat.h>
#import <Foundation/NSTask.h>
#import <Foundation/NSFileManager.h>
@class DTiPhoneSimulatorSystemRoot;

NSString *simulatorPrefrencesName = @"com.apple.iphonesimulator";
NSString *deviceProperty = @"SimulateDevice";
NSString *deviceIphoneRetina3_5Inch = @"iPhone Retina (3.5-inch)";
NSString *deviceIphoneRetina4_0Inch = @"iPhone Retina (4-inch)";
NSString *deviceIphoneRetina4_0Inch_64bit = @"iPhone Retina (4-inch 64-bit)";
NSString *deviceIphone = @"iPhone";
NSString *deviceIpad = @"iPad";
NSString *deviceIpadRetina = @"iPad Retina";
NSString *deviceIpadRetina_64bit = @"iPad Retina (64-bit)";

NSString* deviceTypeIdIphone4s = @"com.apple.CoreSimulator.SimDeviceType.iPhone-4s";
NSString* deviceTypeIdIphone5 = @"com.apple.CoreSimulator.SimDeviceType.iPhone-5";
NSString* deviceTypeIdIphone5s = @"com.apple.CoreSimulator.SimDeviceType.iPhone-5s";
NSString* deviceTypeIdIphone6 = @"com.apple.CoreSimulator.SimDeviceType.iPhone-6";
NSString* deviceTypeIdIphone6Plus = @"com.apple.CoreSimulator.SimDeviceType.iPhone-6-Plus";
NSString* deviceTypeIdIpad2 = @"com.apple.CoreSimulator.SimDeviceType.iPad-2";
NSString* deviceTypeIdIpadRetina = @"com.apple.CoreSimulator.SimDeviceType.iPad-Retina";
NSString* deviceTypeIdIpadAir = @"com.apple.CoreSimulator.SimDeviceType.iPad-Air";
NSString* deviceTypeIdResizableIphone = @"com.apple.CoreSimulator.SimDeviceType.Resizable-iPhone";
NSString* deviceTypeIdResizeableIpad = @"com.apple.CoreSimulator.SimDeviceType.Resizable-iPad";

// The path within the developer dir of the private Simulator frameworks.
NSString* const kSimulatorFrameworkRelativePathLegacy = @"Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks/DVTiPhoneSimulatorRemoteClient.framework";
NSString* const kSimulatorFrameworkRelativePath = @"../SharedFrameworks/DVTiPhoneSimulatorRemoteClient.framework";
NSString* const kDVTFoundationRelativePath = @"../SharedFrameworks/DVTFoundation.framework";
NSString* const kDevToolsFoundationRelativePath = @"../OtherFrameworks/DevToolsFoundation.framework";
//NSString* const kSimulatorRelativePath = @"Platforms/iPhoneSimulator.platform/Developer/Applications/iPhone Simulator.app";
NSString* const kCoreSimulatorRelativePath = @"Library/PrivateFrameworks/CoreSimulator.framework";

static pid_t gDebuggerProcessId = 0;

static const char *gDevDir = 0;

@interface DVTPlatform : NSObject

+ (BOOL)loadAllPlatformsReturningError:(NSError **)error;
+ (id)platformForIdentifier:(NSString *)identifier;

@end

/**
 * A simple iPhoneSimulatorRemoteClient framework.
 */
@implementation iPhoneSimulator

- (id)init {
    self = [super init];
    deviceTypeId = 0;
    m_stderrPath = 0;
    m_stdoutPath = 0;
    dataPath = 0;
    xcodeVersionInt = 0;

    mySession = nil;
    return self;
}

- (void)dealloc {
    [mySession release];
    [super dealloc];
}

- (void)doExit:(int)errorCode {
    if (stderrFileHandle != nil) {
      [self removeStdioFIFO:stderrFileHandle atPath:m_stderrPath];
    }

    if (stdoutFileHandle != nil) {
      [self removeStdioFIFO:stdoutFileHandle atPath:m_stdoutPath];
    }
    nsprintf(@"<exit code=\"%d\"/>", errorCode);
    nsprintf(@"</query_result>");
    fflush(stdout);
    fflush(stderr);
    exit(errorCode);
}

// Helper to find a class by name and die if it isn't found.
-(Class) FindClassByName:(NSString*) nameOfClass {
    Class theClass = NSClassFromString(nameOfClass);
    if (!theClass) {
        nsfprintf(stderr,@"Failed to find class %@ at runtime.", nameOfClass);
        [self doExit:EXIT_FAILURE];
    }
    return theClass;
}

// Loads the Simulator framework from the given developer dir.
-(void) LoadSimulatorFramework:(NSString*) developerDir {
    // The Simulator framework depends on some of the other Xcode private
    // frameworks; manually load them first so everything can be linked up.
    NSString* dvtFoundationPath = [developerDir stringByAppendingPathComponent:kDVTFoundationRelativePath];

    NSBundle* dvtFoundationBundle =
    [NSBundle bundleWithPath:dvtFoundationPath];
    if (![dvtFoundationBundle load]){
        nsprintf(@"Unable to dvtFoundationBundle. Error: ");
        [self doExit:EXIT_FAILURE];
        return ;
    }
    NSString* devToolsFoundationPath = [developerDir stringByAppendingPathComponent:kDevToolsFoundationRelativePath];
    NSBundle* devToolsFoundationBundle =
    [NSBundle bundleWithPath:devToolsFoundationPath];
    if (![devToolsFoundationBundle load]){
        nsprintf(@"Unable to devToolsFoundationPath.");
        return ;
    }
    NSString* coreSimulatorPath = [developerDir stringByAppendingPathComponent:kCoreSimulatorRelativePath];
    if ([[NSFileManager defaultManager] fileExistsAtPath:coreSimulatorPath]) {
        NSBundle* coreSimulatorBundle = [NSBundle bundleWithPath:coreSimulatorPath];
        if (![coreSimulatorBundle load]){
            nsprintf(@"Unable to coreSimulatorPath.");
            return ;
        }
    }
    NSString* simBundlePath = [developerDir stringByAppendingPathComponent:kSimulatorFrameworkRelativePathLegacy];
    if (![[NSFileManager defaultManager] fileExistsAtPath:simBundlePath]){
        simBundlePath = [developerDir stringByAppendingPathComponent:kSimulatorFrameworkRelativePath];
    }
    NSBundle* simBundle = [NSBundle bundleWithPath:simBundlePath];
    if (![simBundle load]){
        nsprintf(@"Unable to load simulator framework.");
        return ;
    }
    NSError* error = 0;
    // Prime DVTPlatform.
    Class DVTPlatformClass = [self FindClassByName:@"DVTPlatform"];
    if (![DVTPlatformClass loadAllPlatformsReturningError:&error]) {
        nsprintf(@"Unable to loadAllPlatformsReturningError. Error: %@",[error localizedDescription]);
        return ;
    }
    Class systemRootClass = [self FindClassByName:@"DTiPhoneSimulatorSystemRoot"];
    // The following will fail if DVTPlatform hasn't loaded all platforms.
    NSAssert(systemRootClass && [systemRootClass knownRoots] != nil,
             @"DVTPlatform hasn't been initialized yet.");
    // DTiPhoneSimulatorRemoteClient will make this same call, so let's assert
    // that it's working.
    NSAssert([DVTPlatformClass platformForIdentifier:@"com.apple.platform.iphonesimulator"] != nil,
             @"DVTPlatform hasn't been initialized yet.");
    return ;
}

NSString* GetXcodeVersion() {
    // Go look for it via xcodebuild.
    NSTask* xcodeBuildTask = [[[NSTask alloc] init] autorelease];
    [xcodeBuildTask setLaunchPath:@"/usr/bin/xcodebuild"];
    [xcodeBuildTask setArguments:[NSArray arrayWithObject:@"-version"]];

    NSPipe* outputPipe = [NSPipe pipe];
    [xcodeBuildTask setStandardOutput:outputPipe];
    NSFileHandle* outputFile = [outputPipe fileHandleForReading];

    [xcodeBuildTask launch];
    NSData* outputData = [outputFile readDataToEndOfFile];
    [xcodeBuildTask terminate];

    NSString* output =
    [[[NSString alloc] initWithData:outputData
                           encoding:NSUTF8StringEncoding] autorelease];
    output = [output stringByTrimmingCharactersInSet:
              [NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if ([output length] == 0) {
        output = nil;
    } else {
        NSArray* parts = [output componentsSeparatedByCharactersInSet: [NSCharacterSet whitespaceAndNewlineCharacterSet]];
        if ([parts count] >= 2) {
            return parts[1];
        }
    }
    return output;
}


// Finds the developer dir via xcode-select or the DEVELOPER_DIR environment
// variable.
NSString* FindDeveloperDir() {
    if (gDevDir)
        return [NSString stringWithCString:gDevDir encoding:NSUTF8StringEncoding];
    // Check the env first.
    NSDictionary* env = [[NSProcessInfo processInfo] environment];
    NSString* developerDir = [env objectForKey:@"DEVELOPER_DIR"];
    if ([developerDir length] > 0) {
        return developerDir;
    }

    // Go look for it via xcode-select.
    NSTask* xcodeSelectTask = [[[NSTask alloc] init] autorelease];
    [xcodeSelectTask setLaunchPath:@"/usr/bin/xcode-select"];
    [xcodeSelectTask setArguments:[NSArray arrayWithObject:@"-print-path"]];

    NSPipe* outputPipe = [NSPipe pipe];
    [xcodeSelectTask setStandardOutput:outputPipe];
    NSFileHandle* outputFile = [outputPipe fileHandleForReading];

    [xcodeSelectTask launch];
    NSData* outputData = [outputFile readDataToEndOfFile];
    [xcodeSelectTask terminate];

    NSString* output =
    [[[NSString alloc] initWithData:outputData
                           encoding:NSUTF8StringEncoding] autorelease];
    output = [output stringByTrimmingCharactersInSet:
              [NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if ([output length] == 0) {
        output = nil;
    }
    return output;
}

- (void) printUsage {
  fprintf(stdout, "<msg>Usage: ios-sim &lt;command&gt; &lt;options&gt; [--args ...]\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "Commands:\n");
  fprintf(stdout, "  showsdks                        List the available iOS SDK versions\n");
  fprintf(stdout, "  showdevicetypes                 List the available device types (Xcode6+)\n");
  fprintf(stdout, "  launch &lt;application path&gt;       Launch the application at the specified path on the iOS Simulator\n");
  fprintf(stdout, "  start                           Launch iOS Simulator without an app\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "Options:\n");
  fprintf(stdout, "  --version                       Print the version of ios-sim\n");
  fprintf(stdout, "  --developer-path &lt;developerDir&gt; path to the developer directory (in Xcode)");
  fprintf(stdout, "  --help                          Show this help text\n");
  fprintf(stdout, "  --verbose                       Set the output level to verbose\n");
  fprintf(stdout, "  --exit                          Exit after startup\n");
  fprintf(stdout, "  --wait-for-debugger             Wait for debugger to attach\n");
  fprintf(stdout, "  --debug                         Attach LLDB to the application on startup\n");
  fprintf(stdout, "  --use-gdb                       Use GDB instead of LLDB. (Requires --debug)\n");
  fprintf(stdout, "  --uuid &lt;uuid&gt;                   A UUID identifying the session (is that correct?)\n");
  fprintf(stdout, "  --env &lt;environment file path&gt;   A plist file containing environment key-value pairs that should be set\n");
  fprintf(stdout, "  --setenv NAME=VALUE             Set an environment variable\n");
  fprintf(stdout, "  --stdout &lt;stdout file path&gt;     The path where stdout of the simulator will be redirected to (defaults to stdout of ios-sim)\n");
  fprintf(stdout, "  --stderr &lt;stderr file path&gt;     The path where stderr of the simulator will be redirected to (defaults to stderr of ios-sim)\n");
  fprintf(stdout, "  --timeout &lt;seconds&gt;             The timeout time to wait for a response from the Simulator. Default value: 30 seconds\n");
  fprintf(stdout, "  --args &lt;...&gt;                    All following arguments will be passed on to the application</msg>\n");
  fprintf(stdout, "  --devicetypeid <device type>    The id of the device type that should be simulated (Xcode6+). Use 'showdevicetypes' to list devices.\n");
  fprintf(stdout, "                                  e.g \"com.apple.CoreSimulator.SimDeviceType.Resizable-iPhone6, 8.0\"\n");
  fprintf(stdout, "DEPRECATED in 3.x, use devicetypeid instead:\n");
  fprintf(stdout, "  --sdk <sdkversion>              The iOS SDK version to run the application on (defaults to the latest)\n");
  fprintf(stdout, "  --family <device family>        The device type that should be simulated (defaults to `iphone')\n");
  fprintf(stdout, "  --retina                        Start a retina device\n");
  fprintf(stdout, "  --tall                          In combination with --retina flag, start the tall version of the retina device (e.g. iPhone 5 (4-inch))\n");
  fprintf(stdout, "  --64bit                         In combination with --retina flag and the --tall flag, start the 64bit version of the tall retina device (e.g. iPhone 5S (4-inch 64bit))\n");
  fflush(stdout);
}

- (void) printDeprecation:(char*)option {
  (void)option;
  //fprintf(stdout, "Usage of '%s' is deprecated in 3.x. Use --devicetypeid instead.\n", option);
}


- (int) showSDKs {
  Class systemRootClass = [self FindClassByName:@"DTiPhoneSimulatorSystemRoot"];

  NSArray *roots = [systemRootClass knownRoots];

  nsprintf(@"<device_info>");
  for (NSUInteger i = 0; i < [roots count]; ++i) {
    DTiPhoneSimulatorSystemRoot *root = [roots objectAtIndex:i];
      nsprintf(@"<item><key>sdk%d_name</key><value>%@</value></item>", i, [root sdkDisplayName]);
      nsprintf(@"<item><key>sdk%d_version</key><value>%@</value></item>", i, [root sdkVersion]);
      nsprintf(@"<item><key>sdk%d_sysroot</key><value>%@</value></item>", i, [root sdkRootPath]);
  }
  nsprintf(@"</device_info>");

  return EXIT_SUCCESS;
}

- (int) showDeviceTypes {
    Class simDeviceSet = NSClassFromString(@"SimDeviceSet");
    nsprintf(@"<device_info>");
    bool hasDevices = false;
    if (simDeviceSet) {
        SimDeviceSet* deviceSet = [simDeviceSet defaultSet];
        NSArray* devices = [deviceSet availableDevices];
        for (SimDevice* device in devices) {
            hasDevices = true;
            nsfprintf(stdout, @"<item><key>%@, %@</key><value>%@</value></item>",device.deviceType.identifier, device.runtime.versionString, device.deviceType.name);
        }
    }
    if (!hasDevices) {
        // fallback devices for Xcode 5.x
        nsprintf(@"<item><key>com.apple.CoreSimulator.SimDeviceType.iPhone-4s</key><value>iPhone 3.5-inch Retina Display</value></item>");
        nsprintf(@"<item><key>com.apple.CoreSimulator.SimDeviceType.iPhone-5s</key><value>iPhone 4-inch Retina Display</value></item>");
        nsprintf(@"<item><key>com.apple.CoreSimulator.SimDeviceType.iPad-2</key><value>iPad</value></item>");
        nsprintf(@"<item><key>com.apple.CoreSimulator.SimDeviceType.iPad-Retina</key><value>iPad Retina Display</value></item>");
    }
    nsprintf(@"</device_info>");

    return EXIT_SUCCESS;
}

- (void)session:(DTiPhoneSimulatorSession *)session didEndWithError:(NSError *)error {
  (void)session;
  if (verbose) {
    msgprintf(@"Session did end with error %@", error);
  }

  if (error != nil) {
    [self doExit:EXIT_FAILURE];
  }

  [self doExit:EXIT_SUCCESS];
}

static void IgnoreSignal(int /*arg*/) {
}

static void ChildSignal(int /*arg*/) {
  int status;
  waitpid(gDebuggerProcessId, &status, 0);
  exit(EXIT_SUCCESS);
}

- (void)session:(DTiPhoneSimulatorSession *)session didStart:(BOOL)started withError:(NSError *)error {
  if (started) {
    // bring to front...
    [NSTask launchedTaskWithLaunchPath:@"/usr/bin/osascript"
        arguments:[NSArray arrayWithObjects:
            @"-e", @"tell application \"System Events\"",
            @"-e", @"  set listOfProcesses to (name of every process where background only is false)",
            @"-e", @"end tell",
            @"-e", @"repeat with processName in listOfProcesses",
            @"-e", @"  if processName starts with \"iOS Simulator\" or processName starts with \"iPhone Simulator\" then",
            @"-e", @"    tell application processName to activate",
            @"-e", @"  end if",
            @"-e", @"end repeat", nil]];
  }
  if (startOnly && session) {
      msgprintf(@"Simulator started (no session)");
      [self doExit:EXIT_SUCCESS];
      return;
  }
  if (started) {
      int pid  = [session simulatedApplicationPID];
      if (shouldStartDebugger) {
        char*args[4] = { NULL, NULL, (char*)[[[NSNumber numberWithInt:pid] description] UTF8String], NULL };
        if (useGDB) {
          args[0] = strdup("gdb");
          args[1] = strdup("program");
        } else {
          args[0] = strdup("lldb");
          args[1] = strdup("--attach-pid");
        }
        // The parent process must live on to process the stdout/stderr fifos,
        // so start the debugger as a child process.
        pid_t child_pid = fork();
        if (child_pid == 0) {
            execvp(args[0], args);
        } else if (child_pid < 0) {
            msgprintf(@"Could not start debugger process: %d", errno);
            [self doExit:EXIT_FAILURE];
            return;
        }
        gDebuggerProcessId = child_pid;
        signal(SIGINT, IgnoreSignal);
        signal(SIGCHLD, ChildSignal);
      }
    if (verbose) {
      msgprintf(@"Session started");
    }
    nsprintf(@"<inferior_pid>%d</inferior_pid>", pid);
    fflush(stdout);
    if (exitOnStartup) {
        [self doExit:EXIT_SUCCESS];
        return;
    }
  } else {
      msgprintf(@"Session could not be started: %@", [error localizedDescription]);
      [self doExit:EXIT_FAILURE];
  }
}

- (void)stop {
    if (mySession)
        [mySession requestEndWithTimeout: 0.1];
}

- (void)stdioDataIsAvailable:(NSNotification *)notification {
  [[notification object] readInBackgroundAndNotify];
  NSData *data = [[notification userInfo] valueForKey:NSFileHandleNotificationDataItem];
  NSString *str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]; /* dangerous if partially encoded data is at the boundary */
  if (!alreadyPrintedData) {
    if ([str length] == 0) {
      return;
    } else {
      alreadyPrintedData = YES;
    }
  }
  if ([notification object] == stdoutFileHandle) {
    printf("<app_output>%s</app_output>\n", [escapeString(str) UTF8String]);
  } else {
    nsprintf(@"<app_output>%@</app_output>", escapeString(str)); // handle stderr differently?
  }
  fflush(stdout);
}


- (void)createStdioFIFO:(NSFileHandle **)fileHandle ofType:(NSString *)type atPath:(NSString **)path {
  *path = [NSString stringWithFormat:@"%@/ios-sim-%@-pipe-%d", NSTemporaryDirectory(), type, (int)time(NULL)];
  if (mkfifo([*path UTF8String], S_IRUSR | S_IWUSR) == -1) {
    msgprintf(@"Unable to create %@ named pipe `%@'", type, *path);
    [self doExit:EXIT_FAILURE];
  } else {
    if (verbose) {
      msgprintf(@"Creating named pipe at `%@'", *path);
    }
    int fd = open([*path UTF8String], O_RDONLY | O_NDELAY);
    *fileHandle = [[[NSFileHandle alloc] initWithFileDescriptor:fd] retain];
    [*fileHandle readInBackgroundAndNotify];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(stdioDataIsAvailable:)
                                                 name:NSFileHandleReadCompletionNotification
                                               object:*fileHandle];
  }
}


- (void)removeStdioFIFO:(NSFileHandle *)fileHandle atPath:(NSString *)path {
  if (verbose) {
    msgprintf(@"Removing named pipe at `%@'", path);
  }
  [fileHandle closeFile];
  [fileHandle release];
  if (![[NSFileManager defaultManager] removeItemAtPath:path error:NULL]) {
    msgprintf(@"Unable to remove named pipe `%@'", path);
  }
  if (dataPath && ![[NSFileManager defaultManager] removeItemAtPath:[dataPath stringByAppendingPathComponent:path] error:NULL]) {
    msgprintf(@"Unable to remove simlink at `%@'", [dataPath stringByAppendingPathComponent:path]);
  }
}


- (int)launchApp:(NSString *)path withFamily:(NSString *)family
                                        uuid:(NSString *)uuid
                                 environment:(NSDictionary *)environment
                                  stdoutPath:(NSString *)stdoutPath
                                  stderrPath:(NSString *)stderrPath
                                     timeout:(NSTimeInterval)timeout
                                        args:(NSArray *)args {
  DTiPhoneSimulatorApplicationSpecifier *appSpec;
  DTiPhoneSimulatorSessionConfig *config;
  DTiPhoneSimulatorSession *session;
  NSError *error;

  NSFileManager *fileManager = [[[NSFileManager alloc] init] autorelease];
  if (!startOnly && ![fileManager fileExistsAtPath:path]) {
      msgprintf(@"Application path %@ doesn't exist!", path);
      return EXIT_FAILURE;
  }

  /* Create the app specifier */
    appSpec = startOnly ? nil : [[self FindClassByName:@"DTiPhoneSimulatorApplicationSpecifier"] specifierWithApplicationPath:path];

  if (verbose) {
      msgprintf(@"App Spec: %@\n", appSpec);
      msgprintf(@"SDK Root: %@\n", sdkRoot);
      msgprintf(@"SDK Version: %@\n", [sdkRoot sdkVersion]);

    for (id key in environment) {
      msgprintf(@"Env: %@ = %@", key, [environment objectForKey:key]);
    }
  }

  NSString *sdkVersion = [sdkRoot sdkVersion];
  NSString *appSupportDir = [NSString stringWithFormat:@"%@/Library/Application Support/iPhone Simulator/%@",
                             NSHomeDirectory(), sdkVersion];
  NSMutableDictionary *mutableEnv = [NSMutableDictionary dictionaryWithDictionary:environment];
  [mutableEnv setObject:appSupportDir forKey:@"CFFIXED_USER_HOME"];
  [mutableEnv setObject:appSupportDir forKey:@"IPHONE_SHARED_RESOURCES_DIRECTORY"];
  [mutableEnv setObject:appSupportDir forKey:@"HOME"];
  [mutableEnv setObject:[sdkRoot sdkRootPath] forKey:@"IPHONE_SIMULATOR_ROOT"];
  [mutableEnv setObject:[sdkRoot sdkRootPath] forKey:@"DYLD_ROOT_PATH"];
  [mutableEnv setObject:[sdkRoot sdkRootPath] forKey:@"DYLD_FRAMEWORK_PATH"];
  [mutableEnv setObject:[sdkRoot sdkRootPath] forKey:@"DYLD_LIBRARY_PATH"];
  [mutableEnv setObject:@"YES" forKey:@"NSUnbufferedIO"];
  environment = mutableEnv;

  /* Set up the session configuration */
  config = [[[[self FindClassByName:@"DTiPhoneSimulatorSessionConfig"] alloc] init] autorelease];
  [config setApplicationToSimulateOnStart:appSpec];
  [config setSimulatedSystemRoot:sdkRoot];
  [config setSimulatedApplicationShouldWaitForDebugger:shouldWaitDebugger];

  [config setSimulatedApplicationLaunchArgs:args];
  [config setSimulatedApplicationLaunchEnvironment:environment];

  if (stderrPath) {
    stderrFileHandle = nil;
  } else if (!exitOnStartup) {
    [self createStdioFIFO:&stderrFileHandle ofType:@"stderr" atPath:&stderrPath];
    m_stderrPath = stderrPath;
  }
  [config setSimulatedApplicationStdErrPath:stderrPath];

  if (stdoutPath) {
    stdoutFileHandle = nil;
  } else if (!exitOnStartup) {
    [self createStdioFIFO:&stdoutFileHandle ofType:@"stdout" atPath:&stdoutPath];
    m_stdoutPath = stdoutPath;
  }
  [config setSimulatedApplicationStdOutPath:stdoutPath];

  [config setLocalizedClientName: @"iossim"];

  // this was introduced in 3.2 of SDK
  if ([config respondsToSelector:@selector(setSimulatedDeviceFamily:)]) {
    if (family == nil) {
      family = @"iphone";
    }

    if (verbose) {
      nsprintf(@"using device family %@",family);
    }

    if ([family isEqualToString:@"ipad"]) {
[config setSimulatedDeviceFamily:[NSNumber numberWithInt:2]];
    } else{
      [config setSimulatedDeviceFamily:[NSNumber numberWithInt:1]];
    }
  }
  if ([config respondsToSelector:@selector(setDevice:)]) {
    // Xcode6+
    config.device = [self findDeviceWithFamily:family retina:retinaDevice isTallDevice:tallDevice is64Bit:is64BitDevice];

    // The iOS 8 simulator treats stdout/stderr paths relative to the simulator's data directory.
    // Create symbolic links in the data directory that points at the real stdout/stderr paths.
    if ([config.simulatedSystemRoot.sdkVersion compare:@"8.0" options:NSNumericSearch] != NSOrderedAscending) {
      dataPath = config.device.dataPath;
      NSString *simlinkStdout = [dataPath stringByAppendingPathComponent:stdoutPath];
      m_stdoutPath = stdoutPath;
      NSString *simlinkStderr = [dataPath stringByAppendingPathComponent:stderrPath];
      m_stderrPath = stderrPath;
      [[NSFileManager defaultManager] createSymbolicLinkAtPath:simlinkStdout withDestinationPath:stdoutPath error:NULL];
      [[NSFileManager defaultManager] createSymbolicLinkAtPath:simlinkStderr withDestinationPath:stderrPath error:NULL];
    }
  } else {
    // Xcode5 or older
    NSString* devicePropertyValue = [self changeDeviceType:family retina:retinaDevice isTallDevice:tallDevice is64Bit:is64BitDevice];
    [config setSimulatedDeviceInfoName:devicePropertyValue];
  }

  /* Start the session */
  session = [[[[self FindClassByName:@"DTiPhoneSimulatorSession"] alloc] init] autorelease];
  mySession = session;
  [session setDelegate:self];
  if (uuid != nil){
    [session setUuid:uuid];
  }

  if (![session requestStartWithConfig:config timeout:timeout error:&error]) {
    msgprintf(@"Could not start simulator session: %@", error);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

- (SimDevice*) findDeviceWithFamily:(NSString *)family retina:(BOOL)retina isTallDevice:(BOOL)isTallDevice is64Bit:(BOOL)is64Bit {
    NSString* devTypeId = self->deviceTypeId;

    if (!devTypeId) {
        devTypeId = deviceTypeIdIphone5;
        if (retina) {
            if ([family isEqualToString:@"ipad"]) {
                if (is64Bit) {
                    devTypeId = deviceTypeIdIpadAir;
                } else {
                    devTypeId = deviceTypeIdIpadRetina;
                }
            } else {
                if (isTallDevice) {
                    if (is64Bit) {
                        devTypeId = deviceTypeIdIphone5s;
                    } else {
                        devTypeId = deviceTypeIdIphone5;
                    }
                } else {
                    devTypeId = deviceTypeIdIphone4s;
                }
            }
        } else {
            if ([family isEqualToString:@"ipad"]) {
                devTypeId = deviceTypeIdIpad2;
            } else {
                devTypeId = deviceTypeIdIphone4s;
            }
        }
    }

    SimDeviceSet* deviceSet = [[self FindClassByName:@"SimDeviceSet"] defaultSet];
    NSArray* devices = [deviceSet availableDevices];
	NSArray* deviceTypeAndVersion = [devTypeId componentsSeparatedByString:@","];
	if(deviceTypeAndVersion.count == 2) {
		NSString* typeIdentifier = [[deviceTypeAndVersion objectAtIndex:0] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
		NSString* versionString = [[deviceTypeAndVersion objectAtIndex:1] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];;
		for (SimDevice* device in devices) {
			if ([device.deviceType.identifier isEqualToString:typeIdentifier] && [device.runtime.versionString isEqualToString:versionString]) {
				return device;
			}
		}
	}
	//maintain old behavior (if the device identifier doesn't have a version as part of the identifier, loop through to find the first matching)
	else
	{
		for (SimDevice* device in devices) {
			if ([device.deviceType.identifier isEqualToString:devTypeId]) {
				return device;
			}
		}
	}
    // Default to whatever is the first device
    return [devices count] > 0 ? [devices objectAtIndex:0] : nil;
}

- (NSString*) changeDeviceType:(NSString *)family retina:(BOOL)retina isTallDevice:(BOOL)isTallDevice is64Bit:(BOOL)is64Bit {
  NSString *devicePropertyValue;
  if (self->deviceTypeId) {
    if ([deviceTypeIdIphone4s isEqual: deviceTypeId])
        devicePropertyValue = deviceIphoneRetina3_5Inch;
    else if ([deviceTypeIdIphone5s isEqual: deviceTypeId])
        devicePropertyValue = deviceIphoneRetina4_0Inch;
    else if ([deviceTypeIdIpad2 isEqual: deviceTypeId])
        devicePropertyValue = deviceIpad;
    else if ([deviceTypeIdIpadRetina isEqual: deviceTypeId])
        devicePropertyValue = deviceIpadRetina;
    else {
        nsprintf(@"<msg>Unknown or unsupported device type: %@</msg>\n", deviceTypeId);
        [self doExit:EXIT_FAILURE];
        return nil;
    }
  } else if (retina) {
    if (verbose) {
      msgprintf(@"using retina");
    }
    if ([family isEqualToString:@"ipad"]) {
        if (is64Bit) {
            devicePropertyValue = deviceIpadRetina_64bit;
        } else {
            devicePropertyValue = deviceIpadRetina;
        }
    }
    else {
        if (isTallDevice) {
            if (is64Bit) {
                devicePropertyValue = deviceIphoneRetina4_0Inch_64bit;
            } else {
                devicePropertyValue = deviceIphoneRetina4_0Inch;
            }
        } else {
            devicePropertyValue = deviceIphoneRetina3_5Inch;
        }
    }
  } else {
    if ([family isEqualToString:@"ipad"]) {
      devicePropertyValue = deviceIpad;
    } else {
      devicePropertyValue = deviceIphone;
    }
  }
  CFPreferencesSetAppValue((CFStringRef)deviceProperty, (CFPropertyListRef)devicePropertyValue, (CFStringRef)simulatorPrefrencesName);
  CFPreferencesAppSynchronize((CFStringRef)simulatorPrefrencesName);

  return devicePropertyValue;
}


/**
 * Execute 'main'
 */
- (void)runWithArgc:(int)argc argv:(char **)argv {
  if (argc < 2) {
    [self printUsage];
    exit(EXIT_FAILURE);
  }
  
  NSString* xcodeVersion = GetXcodeVersion();
  if (!([xcodeVersion compare:@"6.0" options:NSNumericSearch] != NSOrderedAscending) && false) {
      nsprintf(@"You need to have at least Xcode 6.0 installed -- you have version %@.", xcodeVersion);
      nsprintf(@"Run 'xcode-select --print-path' to check which version of Xcode is enabled.");
      exit(EXIT_FAILURE);
  }

  retinaDevice = NO;
  tallDevice = NO;
  is64BitDevice = NO;
  exitOnStartup = NO;
  alreadyPrintedData = NO;
  startOnly = strcmp(argv[1], "start") == 0;
  deviceTypeId = nil;

  nsprintf(@"<query_result>");
  for (int i = 0; i < argc; ++i) {
      if (strcmp(argv[i], "--developer-path") == 0) {
          ++i;
          if (i < argc)
              gDevDir = argv[i];
      }
  }
  NSString* developerDir = FindDeveloperDir();
  if (!developerDir) {
    nsprintf(@"<msg>Unable to find developer directory.</msg>");
    [self doExit:EXIT_FAILURE];
  }
  NSString *xcodePlistPath = [developerDir stringByAppendingPathComponent:@"../Info.plist"];
  NSAssert([[NSFileManager defaultManager] fileExistsAtPath:xcodePlistPath isDirectory:NULL],
                @"Cannot find Xcode's plist at: %@", xcodePlistPath);

  NSDictionary *infoDict = [NSDictionary dictionaryWithContentsOfFile:xcodePlistPath];
  NSAssert(infoDict[@"DTXcode"], @"Cannot find the 'DTXcode' key in Xcode's Info.plist.");

  NSString *DTXcode = infoDict[@"DTXcode"];
  xcodeVersionInt = [DTXcode intValue];
  NSString* dvtFoundationPath = [developerDir stringByAppendingPathComponent:kDVTFoundationRelativePath];
  if (![[NSFileManager defaultManager] fileExistsAtPath:dvtFoundationPath]) {
      // execute old version
      char *argNew = new char[strlen(argv[0] + 7)];
      strcpy(argNew, argv[0]);
      strcat(argNew, "_1_8_2");
      char **argvNew = new char *[argc + 1];
      argvNew[0] = argNew;
      for (int iarg = 1; iarg < argc; ++iarg)
          argvNew[iarg] = argv[iarg];
      argvNew[argc] = 0;
      execv(argNew, argvNew);
  }
  if (strcmp(argv[1], "showsdks") == 0) {
	[self LoadSimulatorFramework:developerDir];
    [self doExit:[self showSDKs]];
  } else if (strcmp(argv[1], "showdevicetypes") == 0) {
    [self LoadSimulatorFramework:developerDir];
    [self doExit:[self showDeviceTypes]];
  } else if (strcmp(argv[1], "launch") == 0 || startOnly) {
    if (strcmp(argv[1], "launch") == 0 && argc < 3) {
      msgprintf(@"Missing application path argument");
      [self printUsage];
      [self doExit:EXIT_FAILURE];
    }

    NSString *appPath = nil;
    int argOffset;
    if (startOnly) {
      argOffset = 2;
    }
    else {
      argOffset = 3;
      appPath = [[NSString stringWithUTF8String:argv[2]] expandPath];
    }

    NSString *family = nil;
    NSString *uuid = nil;
    NSString *stdoutPath = nil;
    NSString *stderrPath = nil;
    NSString *xctest = nil;
    NSTimeInterval timeout = 60;
    NSMutableDictionary *environment = [NSMutableDictionary dictionary];

    int i = argOffset;
    for (; i < argc; i++) {
      if (strcmp(argv[i], "--version") == 0) {
        printf("%s\n", IOS_SIM_VERSION);
        exit(EXIT_SUCCESS);
      } else if (strcmp(argv[i], "--help") == 0) {
        [self printUsage];
        exit(EXIT_SUCCESS);
      } else if (strcmp(argv[i], "--verbose") == 0) {
        verbose = YES;
      } else if (strcmp(argv[i], "--exit") == 0) {
        exitOnStartup = YES;
      } else if (strcmp(argv[i], "--wait-for-debugger") == 0) {
        shouldWaitDebugger = YES;
      } else if (strcmp(argv[i], "--debug") == 0) {
        shouldWaitDebugger = YES;
        shouldStartDebugger = YES;
      } else if (strcmp(argv[i], "--use-gdb") == 0) {
        useGDB = YES;
      } else if (strcmp(argv[i], "--developer-path") == 0) {
        ++i;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --developer-path</msg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
      } else if (strcmp(argv[i], "--timeout") == 0) {
        if (i + 1 < argc) {
          timeout = [[NSString stringWithUTF8String:argv[++i]] doubleValue];
          NSLog(@"Timeout: %f second(s)", timeout);
        }
      }
      else if (strcmp(argv[i], "--sdk") == 0) {
        [self printDeprecation:argv[i]];
        i++;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --sdk</msg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
       [self LoadSimulatorFramework:developerDir];
        NSString* ver = [NSString stringWithCString:argv[i] encoding:NSUTF8StringEncoding];
        Class systemRootClass = [self FindClassByName:@"DTiPhoneSimulatorSystemRoot"];
        NSArray *roots = [systemRootClass knownRoots];
        for (DTiPhoneSimulatorSystemRoot *root in roots) {
          NSString *v = [root sdkVersion];
          if ([v isEqualToString:ver]) {
            sdkRoot = root;
            break;
          }
        }
        if (sdkRoot == nil) {
          msgprintf(@"Unknown or unsupported SDK version: %s\n",argv[i]);
          [self showSDKs];
          exit(EXIT_FAILURE);
        }
      } else if (strcmp(argv[i], "--family") == 0) {
        [self printDeprecation:argv[i]];
        i++;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --sdkfamilymsg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
        family = [NSString stringWithUTF8String:argv[i]];
      } else if (strcmp(argv[i], "--uuid") == 0) {
        i++;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --uuid</msg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
        uuid = [NSString stringWithUTF8String:argv[i]];
      } else if (strcmp(argv[i], "--devicetypeid") == 0) {
        i++;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --devicetypeid</msg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
        deviceTypeId = [NSString stringWithUTF8String:argv[i]];
      } else if (strcmp(argv[i], "--setenv") == 0) {
        i++;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --setenv</msg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
        NSArray *parts = [[NSString stringWithUTF8String:argv[i]] componentsSeparatedByString:@"="];
        [environment setObject:[parts objectAtIndex:1] forKey:[parts objectAtIndex:0]];
      } else if (strcmp(argv[i], "--env") == 0) {
        i++;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --env</msg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
        NSString *envFilePath = [[NSString stringWithUTF8String:argv[i]] expandPath];
        [environment setValuesForKeysWithDictionary:[NSDictionary dictionaryWithContentsOfFile:envFilePath]];
        if (!environment) {
          msgprintf(@"Could not read environment from file: %s", argv[i]);
          [self printUsage];
          exit(EXIT_FAILURE);
        }
      } else if (strcmp(argv[i], "--stdout") == 0) {
        i++;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --stdout</msg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
        stdoutPath = [[NSString stringWithUTF8String:argv[i]] expandPath];
        NSLog(@"stdoutPath: %@", stdoutPath);
      } else if (strcmp(argv[i], "--stderr") == 0) {
          i++;
          if (i == argc) {
              nsprintf(@"<msg>missing arg after --stderr</msg>");
              [self doExit:EXIT_FAILURE];
              return;
          }
          stderrPath = [[NSString stringWithUTF8String:argv[i]] expandPath];
          NSLog(@"stderrPath: %@", stderrPath);
      } else if (strcmp(argv[i], "--xctest") == 0) {
          i++;
          if (i == argc) {
              nsprintf(@"<msg>missing arg after --xctest</msg>");
              [self doExit:EXIT_FAILURE];
              return;
          }
          xctest = [[NSString stringWithUTF8String:argv[i]] expandPath];
          NSLog(@"xctest: %@", xctest);
      } else if (strcmp(argv[i], "--retina") == 0) {
          [self printDeprecation:argv[i]];
          retinaDevice = YES;
      } else if (strcmp(argv[i], "--tall") == 0) {
          [self printDeprecation:argv[i]];
          tallDevice = YES;
      } else if (strcmp(argv[i], "--64bit") == 0) {
          [self printDeprecation:argv[i]];
          is64BitDevice = YES;
      } else if (strcmp(argv[i], "--args") == 0) {
        i++;
        break;
      } else {
        msgprintf(@"unrecognized argument:%s", argv[i]);
        [self printUsage];
        [self doExit:EXIT_FAILURE];
        return;
      }
    }
    NSMutableArray *args = [NSMutableArray arrayWithCapacity:MAX(argc - i,0)];
    for (; i < argc; i++) {
      [args addObject:[NSString stringWithUTF8String:argv[i]]];
    }

    if (sdkRoot == nil) {
	   [self LoadSimulatorFramework:developerDir];
        Class systemRootClass = [self FindClassByName:@"DTiPhoneSimulatorSystemRoot"];
        sdkRoot = [systemRootClass defaultRoot];
    }
    if (xctest) {
        NSString *appName = [appPath lastPathComponent];
        NSString *executableName = [appName stringByDeletingPathExtension];
        NSString *injectionPath = @"/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks/IDEBundleInjection.framework/IDEBundleInjection";
        [environment setValuesForKeysWithDictionary:[NSDictionary dictionaryWithObjectsAndKeys:
          injectionPath,@"DYLD_INSERT_LIBRARIES",
          xctest, @"XCInjectBundle",
          [appPath stringByAppendingFormat:@"/%@", executableName],@"XCInjectBundleInto",
          nil]];
    }

    /* Don't exit, adds to runloop */
    int res = [self launchApp:appPath
         withFamily:family
               uuid:uuid
        environment:environment
         stdoutPath:stdoutPath
         stderrPath:stderrPath
            timeout:timeout
               args:args];
    nsprintf(@"<app_started status=\"%@\" />", ((res == 0) ? @"SUCCESS" : @"FAILURE"));
    fflush(stdout);
    fflush(stderr);
    if (res != 0)
      [self doExit:EXIT_FAILURE];
  } else {
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
      [self printUsage];
      [self doExit:EXIT_SUCCESS];
    } else if (argc == 2 && strcmp(argv[1], "--version") == 0) {
      printf("%s\n", IOS_SIM_VERSION);
      [self doExit:EXIT_SUCCESS];
    } else {
      fprintf(stderr, "Unknown command\n");
      [self printUsage];
      [self doExit:EXIT_FAILURE];
    }
  }
}

@end
