/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "compileoutputwindow.h"
#include "buildmanager.h"
#include "showoutputtaskhandler.h"
#include "task.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "taskhub.h"

#include <coreplugin/outputwindow.h>
#include <find/basetextfind.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>

#include <QIcon>
#include <QTextCharFormat>
#include <QTextBlock>
#include <QTextCursor>
#include <QPlainTextEdit>
#include <QToolButton>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
const int MAX_LINECOUNT = 100000;
}

namespace ProjectExplorer {
namespace Internal {

class CompileOutputTextEdit : public Core::OutputWindow
{
    Q_OBJECT
public:
    CompileOutputTextEdit(const Core::Context &context) : Core::OutputWindow(context)
    {
        fontSettingsChanged();
        connect(TextEditor::TextEditorSettings::instance(), SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
                this, SLOT(fontSettingsChanged()));
    }

    void addTask(const Task &task, int blocknumber)
    {
        m_taskids.insert(blocknumber, task.taskId);
    }

    void clearTasks()
    {
        m_taskids.clear();
    }
private slots:
    void fontSettingsChanged()
    {
        setFont(TextEditor::TextEditorSettings::instance()->fontSettings().font());
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent *ev)
    {
        int line = cursorForPosition(ev->pos()).block().blockNumber();
        if (unsigned taskid = m_taskids.value(line, 0)) {
            TaskHub *hub = ExtensionSystem::PluginManager::getObject<TaskHub>();
            hub->showTaskInEditor(taskid);
        } else {
            QPlainTextEdit::mouseDoubleClickEvent(ev);
        }
    }

private:
    QHash<int, unsigned int> m_taskids;   //Map blocknumber to taskId
};

} // namespace Internal
} // namespace ProjectExplorer

CompileOutputWindow::CompileOutputWindow(BuildManager * /*bm*/, QAction *cancelBuildAction) :
    m_cancelBuildButton(new QToolButton)
{
    Core::Context context(Constants::C_COMPILE_OUTPUT);
    m_outputWindow = new CompileOutputTextEdit(context);
    m_outputWindow->setWindowTitle(tr("Compile Output"));
    m_outputWindow->setWindowIcon(QIcon(QLatin1String(Constants::ICON_WINDOW)));
    m_outputWindow->setReadOnly(true);
    m_outputWindow->setUndoRedoEnabled(false);
    m_outputWindow->setMaxLineCount(MAX_LINECOUNT);

    // Let selected text be colored as if the text edit was editable,
    // otherwise the highlight for searching is too light
    QPalette p = m_outputWindow->palette();
    QColor activeHighlight = p.color(QPalette::Active, QPalette::Highlight);
    p.setColor(QPalette::Highlight, activeHighlight);
    QColor activeHighlightedText = p.color(QPalette::Active, QPalette::HighlightedText);
    p.setColor(QPalette::HighlightedText, activeHighlightedText);
    m_outputWindow->setPalette(p);

    m_cancelBuildButton->setDefaultAction(cancelBuildAction);

    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_outputWindow);
    agg->add(new Find::BaseTextFind(m_outputWindow));

    qRegisterMetaType<QTextCharFormat>("QTextCharFormat");

    m_handler = new ShowOutputTaskHandler(this);
    ExtensionSystem::PluginManager::addObject(m_handler);
    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateWordWrapMode()));
    updateWordWrapMode();
}

CompileOutputWindow::~CompileOutputWindow()
{
    ExtensionSystem::PluginManager::removeObject(m_handler);
    delete m_handler;
    delete m_cancelBuildButton;
}

void CompileOutputWindow::updateWordWrapMode()
{
    m_outputWindow->setWordWrapEnabled(ProjectExplorerPlugin::instance()->projectExplorerSettings().wrapAppOutput);
}

bool CompileOutputWindow::hasFocus() const
{
    return m_outputWindow->hasFocus();
}

bool CompileOutputWindow::canFocus() const
{
    return true;
}

void CompileOutputWindow::setFocus()
{
    m_outputWindow->setFocus();
}

QWidget *CompileOutputWindow::outputWidget(QWidget *)
{
    return m_outputWindow;
}

QList<QWidget *> CompileOutputWindow::toolBarWidgets() const
{
     return QList<QWidget *>() << m_cancelBuildButton;
}

static QColor mix_colors(QColor a, QColor b)
{
    return QColor((a.red() + 2 * b.red()) / 3, (a.green() + 2 * b.green()) / 3,
                  (a.blue() + 2* b.blue()) / 3, (a.alpha() + 2 * b.alpha()) / 3);
}

void CompileOutputWindow::appendText(const QString &text, ProjectExplorer::BuildStep::OutputFormat format)
{
    QPalette p = m_outputWindow->palette();
    QTextCharFormat textFormat;
    switch (format) {
    case BuildStep::NormalOutput:
        textFormat.setForeground(p.color(QPalette::Text));
        textFormat.setFontWeight(QFont::Normal);
        break;
    case BuildStep::ErrorOutput:
        textFormat.setForeground(mix_colors(p.color(QPalette::Text), QColor(Qt::red)));
        textFormat.setFontWeight(QFont::Normal);
        break;
    case BuildStep::MessageOutput:
        textFormat.setForeground(mix_colors(p.color(QPalette::Text), QColor(Qt::blue)));
        break;
    case BuildStep::ErrorMessageOutput:
        textFormat.setForeground(mix_colors(p.color(QPalette::Text), QColor(Qt::red)));
        textFormat.setFontWeight(QFont::Bold);
        break;

    }

    m_outputWindow->appendText(text, textFormat);
}

void CompileOutputWindow::clearContents()
{
    m_outputWindow->clear();
    m_outputWindow->clearTasks();
    m_taskPositions.clear();
}

void CompileOutputWindow::visibilityChanged(bool)
{

}

int CompileOutputWindow::priorityInStatusBar() const
{
    return 50;
}

bool CompileOutputWindow::canNext() const
{
    return false;
}

bool CompileOutputWindow::canPrevious() const
{
    return false;
}

void CompileOutputWindow::goToNext()
{

}

void CompileOutputWindow::goToPrev()
{

}

bool CompileOutputWindow::canNavigate() const
{
    return false;
}

void CompileOutputWindow::registerPositionOf(const Task &task)
{
    int blocknumber = m_outputWindow->blockCount();
    if (blocknumber > MAX_LINECOUNT)
        return;

    m_taskPositions.insert(task.taskId, blocknumber);
    m_outputWindow->addTask(task, blocknumber);
}

bool CompileOutputWindow::knowsPositionOf(const Task &task)
{
    return (m_taskPositions.contains(task.taskId));
}

void CompileOutputWindow::showPositionOf(const Task &task)
{
    int position = m_taskPositions.value(task.taskId);
    QTextCursor newCursor(m_outputWindow->document()->findBlockByNumber(position));
    newCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    m_outputWindow->setTextCursor(newCursor);
}

#include "compileoutputwindow.moc"
