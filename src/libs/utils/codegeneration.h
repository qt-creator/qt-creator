/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
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

namespace Core {
namespace Utils {

QWORKBENCH_UTILS_EXPORT QString headerGuard(const QString &file);

QWORKBENCH_UTILS_EXPORT
void writeIncludeFileDirective(const QString &file,
                               bool globalInclude,
                               QTextStream &str);

// Write opening namespaces and return an indentation string to be used
// in the following code if there are any.
QWORKBENCH_UTILS_EXPORT
QString writeOpeningNameSpaces(const QStringList &namespaces,
                               const QString &indent,
                               QTextStream &str);

// Close namespacesnamespaces
QWORKBENCH_UTILS_EXPORT
void writeClosingNameSpaces(const QStringList &namespaces,
                            const QString &indent,
                            QTextStream &str);

} // namespace Utils
} // namespace Core

#endif // CODEGENERATION_H
