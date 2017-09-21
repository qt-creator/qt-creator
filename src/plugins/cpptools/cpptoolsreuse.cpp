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

#include "cpptoolsreuse.h"

#include "cppcodemodelsettings.h"
#include "cpptoolsplugin.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>

#include <cplusplus/Overview.h>
#include <cplusplus/LookupContext.h>
#include <utils/algorithm.h>
#include <utils/textutils.h>
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
            ClassOrNamespace *clazz = context.lookupType(namedType->name(),
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
    Utils::Text::convertPosition(cursor.document(), cursor.position(), &line, &column);

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

int indexerFileSizeLimitInMb()
{
    const QSharedPointer<CppCodeModelSettings> settings = codeModelSettings();
    QTC_ASSERT(settings, return -1);

    if (settings->skipIndexingBigFiles())
        return settings->indexerFileSizeLimitInMb();

    return -1;
}

bool fileSizeExceedsLimit(const QFileInfo &fileInfo, int sizeLimitInMb)
{
    if (sizeLimitInMb <= 0)
        return false;

    const qint64 fileSizeInMB = fileInfo.size() / (1000 * 1000);
    if (fileSizeInMB > sizeLimitInMb) {
        const QString absoluteFilePath = fileInfo.absoluteFilePath();
        const QString msg = QCoreApplication::translate(
                    "CppIndexer",
                    "C++ Indexer: Skipping file \"%1\" because it is too big.")
                        .arg(absoluteFilePath);
        Core::MessageManager::write(msg, Core::MessageManager::Silent);
        qWarning().noquote() << msg;
        return true;
    }

    return false;
}

CompilerOptionsBuilder::PchUsage getPchUsage()
{
    const QSharedPointer<CppCodeModelSettings> cms = codeModelSettings();
    if (cms->pchUsage() == CppCodeModelSettings::PchUse_None)
        return CompilerOptionsBuilder::PchUsage::None;
    return CompilerOptionsBuilder::PchUsage::Use;
}

} // CppTools
