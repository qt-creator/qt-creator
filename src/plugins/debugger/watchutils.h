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

#ifndef WATCHUTILS_H
#define WATCHUTILS_H

#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QString;
class QByteArray;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

QString dotEscape(QString str);
QString currentTime();
bool isSkippableFunction(const QString &funcName, const QString &fileName);
bool isLeavableFunction(const QString &funcName, const QString &fileName);

inline bool isNameChar(char c)
{
    // could be 'stopped' or 'shlibs-added'
    return (c >= 'a' && c <= 'z') || c == '-';
}

bool hasLetterOrNumber(const QString &exp);
bool hasSideEffects(const QString &exp);
bool isKeyWord(const QString &exp);
bool isPointerType(const QString &type);
bool isAccessSpecifier(const QString &str);
bool startsWithDigit(const QString &str);
QString stripPointerType(QString type);
QString gdbQuoteTypes(const QString &type);
bool extractTemplate(const QString &type, QString *tmplate, QString *inner);
QString extractTypeFromPTypeOutput(const QString &str);
bool isIntOrFloatType(const QString &type);
QString sizeofTypeExpression(const QString &type);

// Parse 'query' (1) protocol response of the custom dumpers
bool parseQueryDumperOutput(const QByteArray &a, QStringList *types, QString *qtVersion, QString *qtNamespace);

} // namespace Internal
} // namespace Debugger

#endif // WATCHUTILS_H
