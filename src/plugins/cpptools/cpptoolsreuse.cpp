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

#include "cpptoolsreuse.h"

#include "cpptoolsplugin.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <texteditor/convenience.h>

#include <cplusplus/Overview.h>
#include <cplusplus/LookupContext.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QSet>
#include <QStringRef>
#include <QTextCursor>
#include <QTextDocument>

using namespace CPlusPlus;

namespace CppTools {

static void moveCursorToStartOrEndOfIdentifier(QTextCursor *tc,
                                               QTextCursor::MoveOperation op,
                                               int posDiff = 0)
{
    QTextDocument *doc = tc->document();
    if (!doc)
        return;

    QChar ch = doc->characterAt(tc->position() - posDiff);
    while (isValidIdentifierChar(ch)) {
        tc->movePosition(op);
        ch = doc->characterAt(tc->position() - posDiff);
    }
}

void moveCursorToEndOfIdentifier(QTextCursor *tc)
{
    moveCursorToStartOrEndOfIdentifier(tc, QTextCursor::NextCharacter);
}

void moveCursorToStartOfIdentifier(QTextCursor *tc)
{
    moveCursorToStartOrEndOfIdentifier(tc, QTextCursor::PreviousCharacter, 1);
}

static bool isOwnershipRAIIName(const QString &name)
{
    static QSet<QString> knownNames;
    if (knownNames.isEmpty()) {
        // Qt
        knownNames.insert(QLatin1String("QScopedPointer"));
        knownNames.insert(QLatin1String("QScopedArrayPointer"));
        knownNames.insert(QLatin1String("QMutexLocker"));
        knownNames.insert(QLatin1String("QReadLocker"));
        knownNames.insert(QLatin1String("QWriteLocker"));
        // Standard C++
        knownNames.insert(QLatin1String("auto_ptr"));
        knownNames.insert(QLatin1String("unique_ptr"));
        // Boost
        knownNames.insert(QLatin1String("scoped_ptr"));
        knownNames.insert(QLatin1String("scoped_array"));
    }

    return knownNames.contains(name);
}

bool isOwnershipRAIIType(Symbol *symbol, const LookupContext &context)
{
    if (!symbol)
        return false;

    // This is not a "real" comparison of types. What we do is to resolve the symbol
    // in question and then try to match its name with already known ones.
    if (symbol->isDeclaration()) {
        Declaration *declaration = symbol->asDeclaration();
        const NamedType *namedType = declaration->type()->asNamedType();
        if (namedType) {
            LookupScope *clazz = context.lookupType(namedType->name(),
                                                         declaration->enclosingScope());
            if (clazz && !clazz->symbols().isEmpty()) {
                Overview overview;
                Symbol *symbol = clazz->symbols().at(0);
                return isOwnershipRAIIName(overview.prettyName(symbol->name()));
            }
        }
    }

    return false;
}

bool isValidAsciiIdentifierChar(const QChar &ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

bool isValidFirstIdentifierChar(const QChar &ch)
{
    return ch.isLetter() || ch == QLatin1Char('_') || ch.isHighSurrogate() || ch.isLowSurrogate();
}

bool isValidIdentifierChar(const QChar &ch)
{
    return isValidFirstIdentifierChar(ch) || ch.isNumber();
}

bool isValidIdentifier(const QString &s)
{
    const int length = s.length();
    for (int i = 0; i < length; ++i) {
        const QChar &c = s.at(i);
        if (i == 0) {
            if (!isValidFirstIdentifierChar(c))
                return false;
        } else {
            if (!isValidIdentifierChar(c))
                return false;
        }
    }
    return true;
}

bool isQtKeyword(const QStringRef &text)
{
    switch (text.length()) {
    case 4:
        switch (text.at(0).toLatin1()) {
        case 'e':
            if (text == QLatin1String("emit"))
                return true;
            break;
        case 'S':
            if (text == QLatin1String("SLOT"))
                return true;
            break;
        }
        break;

    case 5:
        if (text.at(0) == QLatin1Char('s') && text == QLatin1String("slots"))
            return true;
        break;

    case 6:
        if (text.at(0) == QLatin1Char('S') && text == QLatin1String("SIGNAL"))
            return true;
        break;

    case 7:
        switch (text.at(0).toLatin1()) {
        case 's':
            if (text == QLatin1String("signals"))
                return true;
            break;
        case 'f':
            if (text == QLatin1String("foreach") || text ==  QLatin1String("forever"))
                return true;
            break;
        }
        break;

    default:
        break;
    }
    return false;
}

void switchHeaderSource()
{
    const Core::IDocument *currentDocument = Core::EditorManager::currentDocument();
    QTC_ASSERT(currentDocument, return);
    const QString otherFile = correspondingHeaderOrSource(currentDocument->filePath().toString());
    if (!otherFile.isEmpty())
        Core::EditorManager::openEditor(otherFile);
}

QString identifierUnderCursor(QTextCursor *cursor)
{
    cursor->movePosition(QTextCursor::StartOfWord);
    cursor->movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    return cursor->selectedText();
}

const Macro *findCanonicalMacro(const QTextCursor &cursor, Document::Ptr document)
{
    QTC_ASSERT(document, return 0);

    int line, column;
    TextEditor::Convenience::convertPosition(cursor.document(), cursor.position(), &line, &column);

    if (const Macro *macro = document->findMacroDefinitionAt(line)) {
        QTextCursor macroCursor = cursor;
        const QByteArray name = CppTools::identifierUnderCursor(&macroCursor).toUtf8();
        if (macro->name() == name)
            return macro;
    } else if (const Document::MacroUse *use = document->findMacroUseAt(cursor.position())) {
        return &use->macro();
    }

    return 0;
}

TextEditor::TextEditorWidget::Link linkToSymbol(Symbol *symbol)
{
    typedef TextEditor::TextEditorWidget::Link Link;

    if (!symbol)
        return Link();

    const QString filename = QString::fromUtf8(symbol->fileName(),
                                               symbol->fileNameLength());

    unsigned line = symbol->line();
    unsigned column = symbol->column();

    if (column)
        --column;

    if (symbol->isGenerated())
        column = 0;

    return Link(filename, line, column);
}

QSharedPointer<CppCodeModelSettings> codeModelSettings()
{
    return CppTools::Internal::CppToolsPlugin::instance()->codeModelSettings();
}

int fileSizeLimit()
{
    static const QByteArray fileSizeLimitAsByteArray = qgetenv("QTC_CPP_FILE_SIZE_LIMIT_MB");
    static int fileSizeLimitAsInt = -1;

    if (fileSizeLimitAsInt == -1) {
        bool ok;
        const int limit = fileSizeLimitAsByteArray.toInt(&ok);
        fileSizeLimitAsInt = ok && limit >= 0 ? limit : 0;
    }

    return fileSizeLimitAsInt;
}

bool skipFileDueToSizeLimit(const QFileInfo &fileInfo, int limitInMB)
{
    if (limitInMB == 0) // unlimited
        return false;

    const int fileSizeInMB = fileInfo.size() * 1000 * 1000;
    if (fileSizeInMB > limitInMB) {
        qWarning() << "Files to process limited by QTC_CPP_FILE_SIZE_LIMIT_MB, skipping"
                   << fileInfo.absoluteFilePath();
        return true;
    }

    return false;
}

Utils::FileNameList modifiedFiles()
{
    Utils::FileNameList files;
    foreach (Core::IDocument *doc, Core::DocumentManager::modifiedDocuments())
        files.append(doc->filePath());
    files.removeDuplicates();
    return files;
}

} // CppTools
