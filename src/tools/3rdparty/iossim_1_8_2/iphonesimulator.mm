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
#import <objc/runtime.h>
#import <AppKit/NSRunningApplication.h>

NSString *simulatorPrefrencesName = @"com.apple.iphonesimulator";
NSString *deviceProperty = @"SimulateDevice";
NSString *deviceIphoneRetina3_5Inch = @"iPhone Retina (3.5-inch)";
NSString *deviceIphoneRetina4_0Inch = @"iPhone Retina (4-inch)";
NSString *deviceIphone = @"iPhone";
NSString *deviceIpad = @"iPad";
NSString *deviceIpadRetina = @"iPad (Retina)";

/**
 * A simple iPhoneSimulatorRemoteClient framework.
 */
@implementation iPhoneSimulator

- (id)init {
    self = [super init];
    session = nil;
    pidCheckingTimer = nil;
    return self;
}

- (void)dealloc {
    [session release];
    [pidCheckingTimer release];
    [super dealloc];
}

- (void)doExit:(int)errorCode {
    nsprintf(@"<exit code=\"%d\"/>", errorCode);
    nsprintf(@"</query_result>");
    fflush(stdout);
    fflush(stderr);
    exit(errorCode);
}

- (void) printUsage {
  fprintf(stdout, "<msg>Usage: iossim <command> <options> [--args ...]\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "Commands:\n");
  fprintf(stdout, "  showsdks                        List the available iOS SDK versions\n");
  fprintf(stdout, "  launch <application path>       Launch the application at the specified path on the iOS Simulator\n");
  fprintf(stdout, "  start                           Launch iOS Simulator without an app\n");
  fprintf(stdout, "\n");
  fprintf(stdout, "Options:\n");
  fprintf(stdout, "  --version                       Print the version of ios-sim\n");
  fprintf(stdout, "  --developer-path <developerDir> path to the developer directory (in Xcode)");
  fprintf(stdout, "  --help                          Show this help text\n");
  fprintf(stdout, "  --verbose                       Set the output level to verbose\n");
  fprintf(stdout, "  --exit                          Exit after startup\n");
  fprintf(stdout, "  --wait-for-debugger             Wait for debugger to attach\n");
  fprintf(stdout, "  --debug                         Attach LLDB to the application on startup\n");
  fprintf(stdout, "  --use-gdb                       Use GDB instead of LLDB. (Requires --debug)\n");
  fprintf(stdout, "  --sdk <sdkversion>              The iOS SDK version to run the application on (defaults to the latest)\n");
  fprintf(stdout, "  --family <device family>        The device type that should be simulated (defaults to `iphone')\n");
  fprintf(stdout, "  --retina                        Start a retina device\n");
  fprintf(stdout, "  --tall                          In combination with --retina flag, start the tall version of the retina device (e.g. iPhone 5 (4-inch))\n");
  fprintf(stdout, "  --uuid <uuid>                   A UUID identifying the session (is that correct?)\n");
  fprintf(stdout, "  --env <environment file path>   A plist file containing environment key-value pairs that should be set\n");
  fprintf(stdout, "  --setenv NAME=VALUE             Set an environment variable\n");
  fprintf(stdout, "  --stdout <stdout file path>     The path where stdout of the simulator will be redirected to (defaults to stdout of ios-sim)\n");
  fprintf(stdout, "  --stderr <stderr file path>     The path where stderr of the simulator will be redirected to (defaults to stderr of ios-sim)\n");
  fprintf(stdout, "  --timeout <seconds>             The timeout time to wait for a response from the Simulator. Default value: 30 seconds\n");
  fprintf(stdout, "  --args <...>                    All following arguments will be passed on to the application</msg>\n");
  fflush(stdout);
}


- (int) showSDKs {
  NSUInteger i;
  id tClass = objc_getClass("DTiPhoneSimulatorSystemRoot");
  if (tClass == nil) {
    nsprintf(@"<msg>DTiPhoneSimulatorSystemRoot class is nil.</msg>");
    return EXIT_FAILURE;
  }
  NSArray *roots = [tClass knownRoots];

  nsprintf(@"<device_info>");
  for (i = 0; i < [roots count]; ++i) {
    DTiPhoneSimulatorSystemRoot *root = [roots objectAtIndex:i];
    nsprintf(@"<item><key>sdk%d_name</key><value>%@</value></item>", i, [root sdkDisplayName]);
    nsprintf(@"<item><key>sdk%d_version</key><value>%@</value></item>", i, [root sdkVersion]);
    nsprintf(@"<item><key>sdk%d_sysroot</key><value>%@</value></item>", i, [root sdkRootPath]);
  }
  nsprintf(@"</device_info>");
  return EXIT_SUCCESS;
}


- (void)session:(DTiPhoneSimulatorSession *)mySession didEndWithError:(NSError *)error {
  if (verbose) {
    nsprintf(@"<msg>Session did end with error %@</msg>", error);
  }

  if (stderrFileHandle != nil) {
    NSString *stderrPath = [[mySession sessionConfig] simulatedApplicationStdErrPath];
    [self removeStdioFIFO:stderrFileHandle atPath:stderrPath];
  }

  if (stdoutFileHandle != nil) {
    NSString *stdoutPath = [[mySession sessionConfig] simulatedApplicationStdOutPath];
    [self removeStdioFIFO:stdoutFileHandle atPath:stdoutPath];
  }

  if (error != nil)
    [self doExit:EXIT_FAILURE];
  else
    [self doExit:EXIT_SUCCESS];
}


- (void)session:(DTiPhoneSimulatorSession *)mySession didStart:(BOOL)started withError:(NSError *)error {
  if (startOnly && mySession) {
    [NSTask launchedTaskWithLaunchPath:@"/usr/bin/osascript"
          arguments:[NSArray arrayWithObjects:@"-e", @"tell application \"iPhone Simulator\"  to activate", nil]];
    nsprintf(@"<msg>Simulator started (no session)</msg>");
    [self doExit:EXIT_SUCCESS];
    return;
  }
  if (started) {
      [NSTask launchedTaskWithLaunchPath:@"/usr/bin/osascript"
          arguments:[NSArray arrayWithObjects:@"-e", @"tell application \"iPhone Simulator\"  to activate", nil]];
      if (shouldStartDebugger) {
        char*args[4] = { NULL, NULL, (char*)[[[mySession simulatedApplicationPID] description] UTF8String], NULL };
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
            nsprintf(@"<msg>Could not start debugger process: %@</msg>", errno);
            [self doExit:EXIT_FAILURE];
            return;
        }
      }
    if (verbose) {
      nsprintf(@"<msg>Session started</msg>");
    }
    nsprintf(@"<inferior_pid>%@</inferior_pid>", [session simulatedApplicationPID]);
    fflush(stdout);
    if (exitOnStartup) {
      [self doExit:EXIT_SUCCESS];
      return;
    }
    pidCheckingTimer = [[NSTimer scheduledTimerWithTimeInterval:5.0 target:self
            selector:@selector(checkPid:) userInfo:nil repeats: TRUE] retain];
  } else {
    nsprintf(@"<msg>Session could not be started: %@</msg>", error);
    [self doExit:EXIT_FAILURE];
  }
}

- (void)stop {
    if (session)
        [session requestEndWithTimeout: 0.1];
}

- (void)checkPid:(NSTimer *)timer {
    (void)timer;
    if (session && [[session simulatedApplicationPID]intValue] > 0) {
        if (kill((pid_t)[[session simulatedApplicationPID]intValue], 0) == -1) {
            nsprintf(@"<msg>app stopped</msg>");
            [self doExit:EXIT_SUCCESS];
            return;
        }
    }
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
    printf("<app_output>%s</app_output>\n", [str UTF8String]);
  } else {
    nsprintf(@"<app_output>%@</app_output>", str); // handle stderr differently?
  }
  fflush(stdout);
}


- (void)createStdioFIFO:(NSFileHandle **)fileHandle ofType:(NSString *)type atPath:(NSString **)path {
  *path = [NSString stringWithFormat:@"%@/ios-sim-%@-pipe-%d", NSTemporaryDirectory(), type, (int)time(NULL)];
  if (mkfifo([*path UTF8String], S_IRUSR | S_IWUSR) == -1) {
    nsprintf(@"<msg>Unable to create %@ named pipe `%@'</msg>", type, *path);
    [self doExit:EXIT_FAILURE];
  } else {
    if (verbose) {
      nsprintf(@"<msg>Creating named pipe at `%@'</msg>", *path);
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
    nsprintf(@"<msg>Removing named pipe at `%@'</msg>", path);
  }
  [fileHandle closeFile];
  [fileHandle release];
  if (![[NSFileManager defaultManager] removeItemAtPath:path error:NULL]) {
    nsprintf(@"<msg>Unable to remove named pipe `%@'</msg>", path);
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
  NSError *error = 0;
  id tClass;

  NSFileManager *fileManager = [[[NSFileManager alloc] init] autorelease];
  if (!startOnly && ![fileManager fileExistsAtPath:path]) {
    nsprintf(@"<msg>Application path %@ doesn't exist!</msg>", path);
    return EXIT_FAILURE;
  }

  /* Create the app specifier */
  tClass = objc_getClass("DTiPhoneSimulatorApplicationSpecifier");
  if (tClass == nil) {
    nsprintf(@"<msg>DTiPhoneSimulatorApplicationSpecifier class is nil.</msg>");
    return EXIT_FAILURE;
  }
  appSpec = startOnly ? nil : [tClass specifierWithApplicationPath:path];

  if (verbose) {
    nsprintf(@"<msg>App Spec: %@</msg>", appSpec);
    nsprintf(@"SDK Root: %@", sdkRoot);

    for (id key in environment) {
      nsprintf(@"<msg>Env: %@ = %@</msg>", key, [environment objectForKey:key]);
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
  tClass = objc_getClass("DTiPhoneSimulatorSessionConfig");
  if (tClass == nil) {
    nsprintf(@"<msg>DTiPhoneSimulatorApplicationSpecifier class is nil.</msg>");
    return EXIT_FAILURE;
  }
  config = [[[tClass alloc] init] autorelease];
  [config setApplicationToSimulateOnStart:appSpec];
  [config setSimulatedSystemRoot:sdkRoot];
  [config setSimulatedApplicationShouldWaitForDebugger:shouldWaitDebugger];

  [config setSimulatedApplicationLaunchArgs:args];
  [config setSimulatedApplicationLaunchEnvironment:environment];

  if (stderrPath) {
    stderrFileHandle = nil;
  } else if (!exitOnStartup) {
    [self createStdioFIFO:&stderrFileHandle ofType:@"stderr" atPath:&stderrPath];
  }
  [config setSimulatedApplicationStdErrPath:stderrPath];

  if (stdoutPath) {
    stdoutFileHandle = nil;
  } else if (!exitOnStartup) {
    [self createStdioFIFO:&stdoutFileHandle ofType:@"stdout" atPath:&stdoutPath];
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

  [self changeDeviceType:family retina:retinaDevice isTallDevice:tallDevice];

  /* Start the session */
  tClass = objc_getClass("DTiPhoneSimulatorSession");
  if (tClass == nil) {
    nsprintf(@"<msg>DTiPhoneSimulatorSession class is nil.</msg>");
    return EXIT_FAILURE;
  }
  session = [[tClass alloc] init];
  [session setDelegate:self];
  if (uuid != nil){
    [session setUuid:uuid];
  }

  if (![session requestStartWithConfig:config timeout:timeout error:&error]) {
    nsprintf(@"<msg>Could not start simulator session: %@</msg>", error);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

- (void) changeDeviceType:(NSString *)family retina:(BOOL)retina isTallDevice:(BOOL)isTallDevice {
  NSString *devicePropertyValue;
  if (retina) {
    if (verbose) {
      nsprintf(@"<msg>using retina</msg>");
    }
    if ([family isEqualToString:@"ipad"]) {
      devicePropertyValue = deviceIpadRetina;
    }
    else {
        if (isTallDevice) {
            devicePropertyValue = deviceIphoneRetina4_0Inch;
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
}


/**
 * Execute 'main'
 */
- (void)runWithArgc:(int)argc argv:(char **)argv {
  if (argc < 2) {
    [self printUsage];
    exit(EXIT_FAILURE);
  }

  retinaDevice = NO;
  tallDevice = NO;
  exitOnStartup = NO;
  alreadyPrintedData = NO;
  startOnly = strcmp(argv[1], "start") == 0;
  nsprintf(@"<query_result>");

  if (strcmp(argv[1], "showdevicetypes") == 0) {
     nsprintf(@"<deviceinfo>");
     nsprintf(@"<item><key>com.apple.CoreSimulator.SimDeviceType.iPhone-4s</key><value>iPhone 3.5-inch Retina Display</value></item>");
     nsprintf(@"<item><key>com.apple.CoreSimulator.SimDeviceType.iPhone-5s</key><value>iPhone 4-inch Retina Display</value></item>");
     nsprintf(@"<item><key>com.apple.CoreSimulator.SimDeviceType.iPad-2</key><value>iPad</value></item>");
     nsprintf(@"<item><key>com.apple.CoreSimulator.SimDeviceType.iPad-Retina</key><value>iPad Retina Display</value></item>");
     nsprintf(@"</deviceinfo>");
    [self doExit:0];
    return;
  } else if (strcmp(argv[1], "showsdks") == 0) {
    [self doExit:[self showSDKs]];
    return;
  } else if (strcmp(argv[1], "launch") == 0 || startOnly) {
    if (strcmp(argv[1], "launch") == 0 && argc < 3) {
      nsprintf(@"<msg>Missing application path argument</msg>");
      [self printUsage];
      [self doExit:EXIT_FAILURE];
      return;
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
          nsprintf(@"<msg>Timeout: %f second(s)</msg>", timeout);
        }
      }
      else if (strcmp(argv[i], "--sdk") == 0) {
        i++;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --sdk</msg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
        NSString* ver = [NSString stringWithCString:argv[i] encoding:NSUTF8StringEncoding];
        id tClass = objc_getClass("DTiPhoneSimulatorSystemRoot");
        NSArray *roots;
        if (tClass == nil) {
          nsprintf(@"<msg>DTiPhoneSimulatorSystemRoot class is nil.</msg>");
          [self doExit:EXIT_FAILURE];
          return;
        }
        roots = [tClass knownRoots];
        for (DTiPhoneSimulatorSystemRoot *root in roots) {
          NSString *v = [root sdkVersion];
          if ([v isEqualToString:ver]) {
            sdkRoot = root;
            break;
          }
        }
        if (sdkRoot == nil) {
          fprintf(stdout,"<msg>Unknown or unsupported SDK version: %s</msg>\n",argv[i]);
          [self showSDKs];
          exit(EXIT_FAILURE);
        }
      } else if (strcmp(argv[i], "--family") == 0) {
        i++;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --family</msg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
        family = [NSString stringWithUTF8String:argv[i]];
      } else if (strcmp(argv[i], "--uuid") == 0) {
        i++;
        uuid = [NSString stringWithUTF8String:argv[i]];
      } else if (strcmp(argv[i], "--devicetypeid") == 0) {
          i++;
          if (i == argc) {
              nsprintf(@"<msg>missing arg after --devicetypeid</msg>");
              [self doExit:EXIT_FAILURE];
              return;
          }
          if (strcmp(argv[i], "com.apple.CoreSimulator.SimDeviceType.iPhone-4s") == 0) {
              family = [NSString stringWithUTF8String:"iphone"];
              retinaDevice = YES;
          } else if (strcmp(argv[i], "com.apple.CoreSimulator.SimDeviceType.iPad-2") == 0) {
              family = [NSString stringWithUTF8String:"ipad"];
          } else if (strcmp(argv[i], "com.apple.CoreSimulator.SimDeviceType.iPhone-5s") == 0) {
              family = [NSString stringWithUTF8String:"iphone"];
              retinaDevice = YES;
              tallDevice = YES;
          } else if (strcmp(argv[i], "com.apple.CoreSimulator.SimDeviceType.iPad-Retina") == 0) {
              family = [NSString stringWithUTF8String:"ipad"];
              retinaDevice = YES;
          } else {
              fprintf(stdout,"<msg>Unknown or unsupported device type: %s</msg>\n",argv[i]);
              [self doExit:EXIT_FAILURE];
              return;
          }
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
        environment = [NSMutableDictionary dictionaryWithContentsOfFile:envFilePath];
        if (!environment) {
          fprintf(stdout, "<msg>Could not read environment from file: %s</msg>\n", argv[i]);
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
        nsprintf(@"<msg>stdoutPath: %@</msg>", stdoutPath);
      } else if (strcmp(argv[i], "--stderr") == 0) {
        i++;
        if (i == argc) {
            nsprintf(@"<msg>missing arg after --stderr</msg>");
            [self doExit:EXIT_FAILURE];
            return;
        }
        stderrPath = [[NSString stringWithUTF8String:argv[i]] expandPath];
        nsprintf(@"<msg>stderrPath: %@</msg>", stderrPath);
      } else if (strcmp(argv[i], "--retina") == 0) {
          retinaDevice = YES;
      } else if (strcmp(argv[i], "--tall") == 0) {
          tallDevice = YES;
      } else if (strcmp(argv[i], "--args") == 0) {
        i++;
        break;
      } else {
        printf("<msg>unrecognized argument:%s</msg>\n", argv[i]);
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
      id tClass = objc_getClass("DTiPhoneSimulatorSystemRoot");
      if (tClass == nil) {
        nsprintf(@"<msg>DTiPhoneSimulatorSystemRoot class is nil.</msg>");
        [self doExit:EXIT_FAILURE];
        return;
      }
      sdkRoot = [tClass defaultRoot];
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
      fprintf(stdout, "<msg>Unknown command</msg>\n");
      [self printUsage];
      [self doExit:EXIT_FAILURE];
    }
  }
}

@end
