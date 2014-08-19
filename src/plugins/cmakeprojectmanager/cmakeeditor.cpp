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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmakeeditor.h"

#include "cmakefilecompletionassist.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>
#include <coreplugin/mimedatabase.h>

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
using namespace TextEditor;
using namespace CMakeProjectManager::Constants;

namespace CMakeProjectManager {
namespace Internal {

//
// CMakeEditor
//

CMakeEditor::CMakeEditor()
{
    addContext(Constants::C_CMAKEEDITOR);
    setDuplicateSupported(true);
    setCommentStyle(Utils::CommentDefinition::HashStyle);
    setCompletionAssistProvider(ExtensionSystem::PluginManager::getObject<CMakeFileCompletionAssistProvider>());
    setEditorCreator([]() { return new CMakeEditor; });
    setWidgetCreator([]() { return new CMakeEditorWidget; });

    setDocumentCreator([this]() -> BaseTextDocument * {
        auto doc = new CMakeDocument;
        connect(doc, &IDocument::changed, this, &CMakeEditor::markAsChanged);
        return doc;
    });
}

void CMakeEditor::markAsChanged()
{
    if (!document()->isModified())
        return;
    InfoBar *infoBar = document()->infoBar();
    Id infoRunCmake("CMakeEditor.RunCMake");
    if (!infoBar->canInfoBeAdded(infoRunCmake))
        return;
    InfoBarEntry info(infoRunCmake,
                      tr("Changes to cmake files are shown in the project tree after building."),
                      InfoBarEntry::GlobalSuppressionEnabled);
    info.setCustomButtonInfo(tr("Build now"), this, SLOT(build()));
    infoBar->addInfo(info);
}

void CMakeEditor::build()
{
    foreach (ProjectExplorer::Project *p, ProjectExplorer::SessionManager::projects()) {
        CMakeProject *cmakeProject = qobject_cast<CMakeProject *>(p);
        if (cmakeProject) {
            if (cmakeProject->isProjectFile(document()->filePath())) {
                ProjectExplorer::ProjectExplorerPlugin::instance()->buildProject(cmakeProject);
                break;
            }
        }
    }
}

QString CMakeEditor::contextHelpId() const
{
    int pos = position();
    BaseTextDocument *doc = const_cast<CMakeEditor*>(this)->textDocument();

    QChar chr;
    do {
        --pos;
        if (pos < 0)
            break;
        chr = doc->characterAt(pos);
        if (chr == QLatin1Char('('))
            return QString();
    } while (chr.unicode() != QChar::ParagraphSeparator);

    ++pos;
    chr = doc->characterAt(pos);
    while (chr.isSpace()) {
        ++pos;
        chr = doc->characterAt(pos);
    }
    int begin = pos;

    do {
        ++pos;
        chr = doc->characterAt(pos);
    } while (chr.isLetterOrNumber() || chr == QLatin1Char('_'));
    int end = pos;

    while (chr.isSpace()) {
        ++pos;
        chr = doc->characterAt(pos);
    }

    // Not a command
    if (chr != QLatin1Char('('))
        return QString();

    QString command = doc->textAt(begin, end - begin).toLower();
    return QLatin1String("command/") + command;
}

//
// CMakeEditorWidget
//

CMakeEditorWidget::CMakeEditorWidget()
{
    setCodeFoldingSupported(true);
}

BaseTextEditor *CMakeEditorWidget::createEditor()
{
    QTC_ASSERT("should not happen anymore" && false, return 0);
}

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

    QDir dir(QFileInfo(textDocument()->filePath()).absolutePath());
    QString fileName = dir.filePath(buffer);
    QFileInfo fi(fileName);
    if (fi.exists()) {
        if (fi.isDir()) {
            QDir subDir(fi.absoluteFilePath());
            QString subProject = subDir.filePath(QLatin1String("CMakeLists.txt"));
            if (QFileInfo(subProject).exists())
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

//
// CMakeDocument
//

CMakeDocument::CMakeDocument()
{
    setId(Constants::CMAKE_EDITOR_ID);
    setMimeType(QLatin1String(Constants::CMAKEMIMETYPE));

    MimeType mimeType = MimeDatabase::findByType(QLatin1String(Constants::CMAKEMIMETYPE));
    setSyntaxHighlighter(TextEditor::createGenericSyntaxHighlighter(mimeType));
}

QString CMakeDocument::defaultPath() const
{
    QFileInfo fi(filePath());
    return fi.absolutePath();
}

QString CMakeDocument::suggestedFileName() const
{
    QFileInfo fi(filePath());
    return fi.fileName();
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

    new TextEditorActionHandler(this, Constants::C_CMAKEEDITOR,
            TextEditorActionHandler::UnCommentSelection
            | TextEditorActionHandler::JumpToFileUnderCursor);

    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
    contextMenu->addAction(ActionManager::command(TextEditor::Constants::JUMP_TO_FILE_UNDER_CURSOR));
    contextMenu->addSeparator(Context(C_CMAKEEDITOR));
    contextMenu->addAction(ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION));
}

IEditor *CMakeEditorFactory::createEditor()
{
    return new CMakeEditor;
}

} // namespace Internal
} // namespace CMakeProjectManager
