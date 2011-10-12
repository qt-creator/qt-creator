#ifndef CPPTOOLSREUSE_H
#define CPPTOOLSREUSE_H

#include "cpptools_global.h"

QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace CppTools {

void CPPTOOLS_EXPORT moveCursorToEndOfIdentifier(QTextCursor *tc);
bool CPPTOOLS_EXPORT isHexadecimal(char c);

} // CppTools

#endif // CPPTOOLSREUSE_H
