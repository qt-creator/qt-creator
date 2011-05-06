/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CODEGENERATION_H
#define CODEGENERATION_H

#include "utils_global.h"
#include <QtCore/QString>

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

#endif // CODEGENERATION_H
