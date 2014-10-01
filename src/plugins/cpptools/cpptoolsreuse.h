/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPTOOLSREUSE_H
#define CPPTOOLSREUSE_H

#include "cpptools_global.h"

QT_FORWARD_DECLARE_CLASS(QChar)
QT_FORWARD_DECLARE_CLASS(QTextCursor)
QT_FORWARD_DECLARE_CLASS(QStringRef)

namespace CPlusPlus {
class Symbol;
class LookupContext;
} // namespace CPlusPlus

namespace CppTools {

void CPPTOOLS_EXPORT moveCursorToEndOfIdentifier(QTextCursor *tc);
void CPPTOOLS_EXPORT moveCursorToStartOfIdentifier(QTextCursor *tc);

bool CPPTOOLS_EXPORT isOwnershipRAIIType(CPlusPlus::Symbol *symbol,
                                         const CPlusPlus::LookupContext &context);

bool CPPTOOLS_EXPORT isValidAsciiIdentifierChar(const QChar &ch);
bool CPPTOOLS_EXPORT isValidFirstIdentifierChar(const QChar &ch);
bool CPPTOOLS_EXPORT isValidIdentifierChar(const QChar &ch);
bool CPPTOOLS_EXPORT isValidIdentifier(const QString &s);

bool CPPTOOLS_EXPORT isQtKeyword(const QStringRef &text);

QString CPPTOOLS_EXPORT correspondingHeaderOrSource(const QString &fileName, bool *wasHeader = 0);
void CPPTOOLS_EXPORT switchHeaderSource();

} // CppTools

#endif // CPPTOOLSREUSE_H
