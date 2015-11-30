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

NSString *escapeString(NSString *origString)
{
    return [[[[origString stringByReplacingOccurrencesOfString: @"&" withString: @"&amp;"]
     stringByReplacingOccurrencesOfString: @"\"" withString: @"&quot;"]
     stringByReplacingOccurrencesOfString: @">" withString: @"&gt;"]
     stringByReplacingOccurrencesOfString: @"<" withString: @"&lt;"];
}

int msgvfprintf (FILE *stream, NSString *format, va_list args) {
    int retval;

    NSString *str = (NSString *) CFStringCreateWithFormatAndArguments(NULL, NULL, (CFStringRef) format, args);
    retval = fprintf(stream, "<msg>%s</msg>\n", [escapeString(str) UTF8String]);
    [str release];

    return retval;
}

int msgprintf(NSString *format, ...) {
    va_list ap;
    int retval;

    va_start(ap, format);
    {
        retval = msgvfprintf(stdout, format, ap);
    }
    va_end(ap);

    return retval;
}
