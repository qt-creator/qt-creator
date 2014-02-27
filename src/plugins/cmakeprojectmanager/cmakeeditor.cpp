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
#include "cmakehighlighter.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"

#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <QFileInfo>
#include <QSharedPointer>
#include <QTextBlock>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

//
// ProFileEditorEditable
//

CMakeEditor::CMakeEditor(CMakeEditorWidget *editor)
  : BaseTextEditor(editor)
{
    setId(CMakeProjectManager::Constants::CMAKE_EDITOR_ID);
    setContext(Core::Context(CMakeProjectManager::Constants::C_CMAKEEDITOR,
              TextEditor::Constants::C_TEXTEDITOR));
    connect(document(), SIGNAL(changed()), this, SLOT(markAsChanged()));
}

Core::IEditor *CMakeEditor::duplicate()
{
    CMakeEditorWidget *ret = new CMakeEditorWidget(
                qobject_cast<CMakeEditorWidget *>(editorWidget()));
    TextEditor::TextEditorSettings::initializeEditor(ret);
    return ret->editor();
}

TextEditor::CompletionAssistProvider *CMakeEditor::completionAssistProvider()
{
    return ExtensionSystem::PluginManager::getObject<CMakeFileCompletionAssistProvider>();
}

void CMakeEditor::markAsChanged()
{
    if (!document()->isModified())
        return;
    Core::InfoBar *infoBar = document()->infoBar();
    Core::Id infoRunCmake("CMakeEditor.RunCMake");
    if (!infoBar->canInfoBeAdded(infoRunCmake))
        return;
    Core::InfoBarEntry info(infoRunCmake,
                            tr("Changes to cmake files are shown in the project tree after building."),
                            Core::InfoBarEntry::GlobalSuppressionEnabled);
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

//
// CMakeEditor
//

CMakeEditorWidget::CMakeEditorWidget(QWidget *parent)
    : BaseTextEditorWidget(new CMakeDocument(), parent)
{
    ctor();
}

CMakeEditorWidget::CMakeEditorWidget(CMakeEditorWidget *other)
    : BaseTextEditorWidget(other)
{
    ctor();
}

void CMakeEditorWidget::ctor()
{
    m_commentDefinition.clearCommentStyles();
    m_commentDefinition.singleLine = QLatin1Char('#');
}

TextEditor::BaseTextEditor *CMakeEditorWidget::createEditor()
{
    return new CMakeEditor(this);
}

void CMakeEditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this, m_commentDefinition);
}

void CMakeEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    showDefaultContextMenu(e, Constants::M_CONTEXT);
}

static bool isValidFileNameChar(const QChar &c)
{
    if (c.isLetterOrNumber()
            || c == QLatin1Char('.')
            || c == QLatin1Char('_')
            || c == QLatin1Char('-')
            || c == QLatin1Char('/')
            || c == QLatin1Char('\\'))
        return true;
    return false;
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

    QDir dir(QFileInfo(baseTextDocument()->filePath()).absolutePath());
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
    : TextEditor::BaseTextDocument()
{
    setMimeType(QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE));
    setSyntaxHighlighter(new CMakeHighlighter);
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
