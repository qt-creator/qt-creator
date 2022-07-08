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

#include "cmakeeditor.h"

#include "cmakeautocompleter.h"
#include "cmakefilecompletionassist.h"
#include "cmakeindenter.h"
#include "cmakeprojectconstants.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>

#include <QDir>
#include <QTextDocument>

#include <functional>

using namespace Core;
using namespace TextEditor;

namespace CMakeProjectManager {
namespace Internal {

//
// CMakeEditor
//

void CMakeEditor::contextHelp(const HelpCallback &callback) const
{
    int pos = position();

    QChar chr;
    do {
        --pos;
        if (pos < 0)
            break;
        chr = characterAt(pos);
        if (chr == QLatin1Char('(')) {
            BaseTextEditor::contextHelp(callback);
            return;
        }
    } while (chr.unicode() != QChar::ParagraphSeparator);

    ++pos;
    chr = characterAt(pos);
    while (chr.isSpace()) {
        ++pos;
        chr = characterAt(pos);
    }
    int begin = pos;

    do {
        ++pos;
        chr = characterAt(pos);
    } while (chr.isLetterOrNumber() || chr == QLatin1Char('_'));
    int end = pos;

    while (chr.isSpace()) {
        ++pos;
        chr = characterAt(pos);
    }

    // Not a command
    if (chr != QLatin1Char('(')) {
        BaseTextEditor::contextHelp(callback);
        return;
    }

    const QString id = "command/" + textAt(begin, end - begin).toLower();
    callback(
        {{id, Utils::Text::wordUnderCursor(editorWidget()->textCursor())}, {}, HelpItem::Unknown});
}

//
// CMakeEditorWidget
//

class CMakeEditorWidget final : public TextEditorWidget
{
public:
    ~CMakeEditorWidget() final = default;

private:
    bool save(const QString &fileName = QString());
    void findLinkAt(const QTextCursor &cursor,
                    const Utils::LinkHandler &processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
};

void CMakeEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    showDefaultContextMenu(e, Constants::M_CONTEXT);
}

static bool mustBeQuotedInFileName(const QChar &c)
{
    return c.isSpace() || c == '"' || c == '(' || c == ')';
}

static bool isValidFileNameChar(const QString &block, int pos)
{
    const QChar c = block.at(pos);
    return !mustBeQuotedInFileName(c) || (pos > 0 && block.at(pos - 1) == '\\');
}

static QString unescape(const QString &s)
{
    QString result;
    int i = 0;
    const int size = s.size();
    while (i < size) {
        const QChar c = s.at(i);
        if (c == '\\' && i < size - 1) {
            const QChar nc = s.at(i + 1);
            if (mustBeQuotedInFileName(nc)) {
                result += nc;
                i += 2;
                continue;
            }
        }
        result += c;
        ++i;
    }
    return result;
}

void CMakeEditorWidget::findLinkAt(const QTextCursor &cursor,
                                   const Utils::LinkHandler &processLinkCallback,
                                   bool/* resolveTarget*/,
                                   bool /*inNextSplit*/)
{
    Utils::Link link;

    int line = 0;
    int column = 0;
    convertPosition(cursor.position(), &line, &column);
    const int positionInBlock = column - 1;

    const QString block = cursor.block().text();

    // check if the current position is commented out
    const int hashPos = block.indexOf(QLatin1Char('#'));
    if (hashPos >= 0 && hashPos < positionInBlock)
        return processLinkCallback(link);

    // find the beginning of a filename
    QString buffer;
    int beginPos = positionInBlock - 1;
    while (beginPos >= 0) {
        if (isValidFileNameChar(block, beginPos)) {
            buffer.prepend(block.at(beginPos));
            beginPos--;
        } else {
            break;
        }
    }

    // find the end of a filename
    int endPos = positionInBlock;
    while (endPos < block.count()) {
        if (isValidFileNameChar(block, endPos)) {
            buffer.append(block.at(endPos));
            endPos++;
        } else {
            break;
        }
    }

    if (buffer.isEmpty())
        return processLinkCallback(link);

    QDir dir(textDocument()->filePath().toFileInfo().absolutePath());
    buffer.replace("${CMAKE_CURRENT_SOURCE_DIR}", dir.path());
    buffer.replace("${CMAKE_CURRENT_LIST_DIR}", dir.path());
    // TODO: Resolve more variables

    QString fileName = dir.filePath(unescape(buffer));
    QFileInfo fi(fileName);
    if (fi.exists()) {
        if (fi.isDir()) {
            QDir subDir(fi.absoluteFilePath());
            QString subProject = subDir.filePath(QLatin1String("CMakeLists.txt"));
            if (QFileInfo::exists(subProject))
                fileName = subProject;
            else
                return processLinkCallback(link);
        }
        link.targetFilePath = Utils::FilePath::fromString(fileName);
        link.linkTextStart = cursor.position() - positionInBlock + beginPos + 1;
        link.linkTextEnd = cursor.position() - positionInBlock + endPos;
    }
    processLinkCallback(link);
}

static TextDocument *createCMakeDocument()
{
    auto doc = new TextDocument;
    doc->setId(Constants::CMAKE_EDITOR_ID);
    doc->setMimeType(QLatin1String(Constants::CMAKE_MIMETYPE));
    return doc;
}

//
// CMakeEditorFactory
//

CMakeEditorFactory::CMakeEditorFactory()
{
    setId(Constants::CMAKE_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", "CMake Editor"));
    addMimeType(Constants::CMAKE_MIMETYPE);
    addMimeType(Constants::CMAKE_PROJECT_MIMETYPE);

    setEditorCreator([]() { return new CMakeEditor; });
    setEditorWidgetCreator([]() { return new CMakeEditorWidget; });
    setDocumentCreator(createCMakeDocument);
    setIndenterCreator([](QTextDocument *doc) { return new CMakeIndenter(doc); });
    setUseGenericHighlighter(true);
    setCommentDefinition(Utils::CommentDefinition::HashStyle);
    setCodeFoldingSupported(true);

    setCompletionAssistProvider(new CMakeFileCompletionAssistProvider);
    setAutoCompleterCreator([]() { return new CMakeAutoCompleter; });

    setEditorActionHandlers(TextEditorActionHandler::UnCommentSelection
            | TextEditorActionHandler::JumpToFileUnderCursor
            | TextEditorActionHandler::Format);

    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
    contextMenu->addAction(ActionManager::command(TextEditor::Constants::JUMP_TO_FILE_UNDER_CURSOR));
    contextMenu->addSeparator(Context(Constants::CMAKE_EDITOR_ID));
    contextMenu->addAction(ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION));
}

} // namespace Internal
} // namespace CMakeProjectManager
