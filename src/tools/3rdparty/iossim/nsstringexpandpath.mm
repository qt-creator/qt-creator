/*
 * See the LICENSE file for the license on the source code in this file.
 */

#import "nsstringexpandpath.h"

@implementation NSString (ExpandPath)

- (NSString *)expandPath {
  if ([self isAbsolutePath]) {
    return [self stringByStandardizingPath];
  } else {
    NSString *cwd = [[NSFileManager defaultManager] currentDirectoryPath];
    return [[cwd stringByAppendingPathComponent:self] stringByStandardizingPath];
  }
}

@end
