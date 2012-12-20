#include <My/MyHeader.h>

#ifndef MyHeader_h
bool failure_MyHeader_not_included;
#endif

#ifndef Nested_h
bool failure_Nested_header_not_included;
#endif

#ifdef IncorrectVersion_h
bool failure_Incorrect_version_of_nested_header_included;
#endif

#ifdef CorrectVersion_h
bool success_is_the_only_option;
#endif


