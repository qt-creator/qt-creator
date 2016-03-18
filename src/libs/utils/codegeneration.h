/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QTextStream;
class QStringList;
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
