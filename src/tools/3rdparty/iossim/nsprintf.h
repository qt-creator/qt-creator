/*
 * See the IOSSIM_LICENSE file in this directory for the license on the source code in this file.
 */
int nsvfprintf (FILE *stream, NSString *format, va_list args);
int nsfprintf (FILE *stream, NSString *format, ...);
int nsprintf (NSString *format, ...);
int msgvfprintf (FILE *stream, NSString *format, va_list args);
int msgprintf (NSString *format, ...);
NSString *escapeString(NSString *origString);
