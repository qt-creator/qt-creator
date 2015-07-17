/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPTOOLSREUSE_H
#define CPPTOOLSREUSE_H

#include "cpptools_global.h"

#include <texteditor/texteditor.h>

#include <cplusplus/CppDocument.h>

QT_BEGIN_NAMESPACE
class QChar;
class QFileInfo;
class QStringRef;
class QTextCursor;
QT_END_NAMESPACE

namespace Utils {
class FileNameList;
} // namespace Utils

namespace CPlusPlus {
class Macro;
class Symbol;
class LookupContext;
} // namespace CPlusPlus

namespace CppTools {

Utils::FileNameList CPPTOOLS_EXPORT modifiedFiles();

void CPPTOOLS_EXPORT moveCursorToEndOfIdentifier(QTextCursor *tc);
void CPPTOOLS_EXPORT moveCursorToStartOfIdentifier(QTextCursor *tc);

bool CPPTOOLS_EXPORT isQtKeyword(const QStringRef &text);

bool CPPTOOLS_EXPORT isValidAsciiIdentifierChar(const QChar &ch);
bool CPPTOOLS_EXPORT isValidFirstIdentifierChar(const QChar &ch);
bool CPPTOOLS_EXPORT isValidIdentifierChar(const QChar &ch);
bool CPPTOOLS_EXPORT isValidIdentifier(const QString &s);

TextEditor::TextEditorWidget::Link CPPTOOLS_EXPORT linkToSymbol(CPlusPlus::Symbol *symbol);

QString CPPTOOLS_EXPORT identifierUnderCursor(QTextCursor *cursor);

bool CPPTOOLS_EXPORT isOwnershipRAIIType(CPlusPlus::Symbol *symbol,
                                         const CPlusPlus::LookupContext &context);

const CPlusPlus::Macro CPPTOOLS_EXPORT *findCanonicalMacro(const QTextCursor &cursor,
                                                           CPlusPlus::Document::Ptr document);

QString CPPTOOLS_EXPORT correspondingHeaderOrSource(const QString &fileName, bool *wasHeader = 0);
void CPPTOOLS_EXPORT switchHeaderSource();

class CppCodeModelSettings;
QSharedPointer<CppCodeModelSettings> CPPTOOLS_EXPORT codeModelSettings();

int fileSizeLimit();
bool skipFileDueToSizeLimit(const QFileInfo &fileInfo, int limitInMB = fileSizeLimit());

} // CppTools

#endif // CPPTOOLSREUSE_H
