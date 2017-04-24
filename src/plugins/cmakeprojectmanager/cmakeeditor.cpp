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

#include "cmakefilecompletionassist.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"
#include "cmakeindenter.h"
#include "cmakeautocompleter.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <texteditor/highlighterutils.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QTextBlock>

using namespace Core;
using namespace ProjectExplorer;
using namespace TextEditor;

namespace CMakeProjectManager {
namespace Internal {

//
// CMakeEditor
//

QString CMakeEditor::contextHelpId() const
{
    int pos = position();

    QChar chr;
    do {
        --pos;
        if (pos < 0)
            break;
        chr = characterAt(pos);
        if (chr == QLatin1Char('('))
            return QString();
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
    if (chr != QLatin1Char('('))
        return QString();

    QString command = textAt(begin, end - begin).toLower();
    return QLatin1String("command/") + command;
}

//
// CMakeEditorWidget
//

class CMakeEditorWidget : public TextEditorWidget
{
public:
    ~CMakeEditorWidget() final = default;

private:
    bool save(const QString &fileName = QString());
    Link findLinkAt(const QTextCursor &cursor, bool resolveTarget = true, bool inNextSplit = false) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
};

void CMakeEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    showDefaultContextMenu(e, Constants::M_CONTEXT);
}

static bool isValidFileNameChar(const QChar &c)
{
    return c.isLetterOrNumber()
            || c == QLatin1Char('.')
            || c == QLatin1Char('_')
            || c == QLatin1Char('-')
            || c == QLatin1Char('/')
            || c == QLatin1Char('\\');
}

CMakeEditorWidget::Link CMakeEditorWidget::findLinkAt(const QTextCursor &cursor,
                                                      bool/* resolveTarget*/, bool /*inNextSplit*/)
{
    Link link;

    int lineNumber = 0, positionInBlock = 0;
    convertPosition(cursor.position(), &lineNumber, &positionInBlock);

    const QString block = cursor.block().text();

    // check if the current position is commented out
    const int hashPos = block.indexOf(QLatin1Char('#'));
    if (hashPos >= 0 && hashPos < positionInBlock)
        return link;

    // find the beginning of a filename
    QString buffer;
    int beginPos = positionInBlock - 1;
    while (beginPos >= 0) {
        QChar c = block.at(beginPos);
        if (isValidFileNameChar(c)) {
            buffer.prepend(c);
            beginPos--;
        } else {
            break;
        }
    }

    // find the end of a filename
    int endPos = positionInBlock;
    while (endPos < block.count()) {
        QChar c = block.at(endPos);
        if (isValidFileNameChar(c)) {
            buffer.append(c);
            endPos++;
        } else {
            break;
        }
    }

    if (buffer.isEmpty())
        return link;

    // TODO: Resolve variables

    QDir dir(textDocument()->filePath().toFileInfo().absolutePath());
    QString fileName = dir.filePath(buffer);
    QFileInfo fi(fileName);
    if (fi.exists()) {
        if (fi.isDir()) {
            QDir subDir(fi.absoluteFilePath());
            QString subProject = subDir.filePath(QLatin1String("CMakeLists.txt"));
            if (QFileInfo::exists(subProject))
                fileName = subProject;
            else
                return link;
        }
        link.targetFileName = fileName;
        link.linkTextStart = cursor.position() - positionInBlock + beginPos + 1;
        link.linkTextEnd = cursor.position() - positionInBlock + endPos;
    }
    return link;
}

static TextDocument *createCMakeDocument()
{
    auto doc = new TextDocument;
    doc->setId(Constants::CMAKE_EDITOR_ID);
    doc->setMimeType(QLatin1String(Constants::CMAKEMIMETYPE));
    return doc;
}

//
// CMakeEditorFactory
//

CMakeEditorFactory::CMakeEditorFactory()
{
    setId(Constants::CMAKE_EDITOR_ID);
    setDisplayName(tr(Constants::CMAKE_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::CMAKEMIMETYPE);
    addMimeType(Constants::CMAKEPROJECTMIMETYPE);

    setEditorCreator([]() { return new CMakeEditor; });
    setEditorWidgetCreator([]() { return new CMakeEditorWidget; });
    setDocumentCreator(createCMakeDocument);
    setIndenterCreator([]() { return new CMakeIndenter; });
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
