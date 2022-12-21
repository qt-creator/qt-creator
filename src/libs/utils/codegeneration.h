// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QStringList>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace Utils {

// Convert a file name to a Cpp identifier (stripping invalid characters
// or replacing them by an underscore).
QTCREATOR_UTILS_EXPORT QString fileNameToCppIdentifier(const QString &s);

QTCREATOR_UTILS_EXPORT QString headerGuard(const QString &file);
QTCREATOR_UTILS_EXPORT QString headerGuard(const QString &file, const QStringList &namespaceList);

QTCREATOR_UTILS_EXPORT
void writeIncludeFileDirective(const QString &file,
                               bool globalInclude,
                               QTextStream &str);

QTCREATOR_UTILS_EXPORT void writeBeginQtVersionCheck(QTextStream &str);

QTCREATOR_UTILS_EXPORT void writeQtIncludeSection(const QStringList &qt4,
                                                  const QStringList &qt5,
                                                  bool addQtVersionCheck,
                                                  bool includeQtModule,
                                                  QTextStream &str);

// Write opening namespaces and return an indentation string to be used
// in the following code if there are any.
QTCREATOR_UTILS_EXPORT
QString writeOpeningNameSpaces(const QStringList &namespaces,
                               const QString &indent,
                               QTextStream &str);

// Close namespacesnamespaces
QTCREATOR_UTILS_EXPORT
void writeClosingNameSpaces(const QStringList &namespaces,
                            const QString &indent,
                            QTextStream &str);

} // namespace Utils
