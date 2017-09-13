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

#include "cpptools_global.h"

#include <texteditor/texteditor.h>

#include <cpptools/compileroptionsbuilder.h>

#include <cplusplus/CppDocument.h>

QT_BEGIN_NAMESPACE
class QChar;
class QFileInfo;
class QStringRef;
class QTextCursor;
QT_END_NAMESPACE

namespace CPlusPlus {
class Macro;
class Symbol;
class LookupContext;
} // namespace CPlusPlus

namespace CppTools {

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

enum class CacheUsage { ReadWrite, ReadOnly };

QString CPPTOOLS_EXPORT correspondingHeaderOrSource(const QString &fileName, bool *wasHeader = 0,
                                                    CacheUsage cacheUsage = CacheUsage::ReadWrite);
void CPPTOOLS_EXPORT switchHeaderSource();

class CppCodeModelSettings;
QSharedPointer<CppCodeModelSettings> CPPTOOLS_EXPORT codeModelSettings();

CompilerOptionsBuilder::PchUsage CPPTOOLS_EXPORT getPchUsage();

int indexerFileSizeLimitInMb();
bool fileSizeExceedsLimit(const QFileInfo &fileInfo, int sizeLimitInMb);

} // CppTools
