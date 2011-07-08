#ifndef CPPTOOLSREUSE_H
#define CPPTOOLSREUSE_H

#include "cpptools_global.h"

QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace CppTools {

void CPPTOOLS_EXPORT moveCursorToEndOfIdentifier(QTextCursor *tc);

} // CppTools

#endif // CPPTOOLSREUSE_H
