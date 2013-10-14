/*
 * NSLog() clone, but writes to arbitrary output stream
 *
 * See the IOSSIM_LICENSE file in this directory for the license on the source code in this file.
 */

#import <Foundation/Foundation.h>
#import <stdio.h>

int nsvfprintf (FILE *stream, NSString *format, va_list args) {
    int retval;

    NSString *str = (NSString *) CFStringCreateWithFormatAndArguments(NULL, NULL, (CFStringRef) format, args);
    retval = fprintf(stream, "%s\n", [str UTF8String]);
    [str release];

    return retval;
}

int nsfprintf (FILE *stream, NSString *format, ...) {
    va_list ap;
    int retval;

    va_start(ap, format);
    {
        retval = nsvfprintf(stream, format, ap);
    }
    va_end(ap);

    return retval;
}

int nsprintf (NSString *format, ...) {
    va_list ap;
    int retval;

    va_start(ap, format);
    {
        retval = nsvfprintf(stdout, format, ap);
    }
    va_end(ap);

    return retval;
}
