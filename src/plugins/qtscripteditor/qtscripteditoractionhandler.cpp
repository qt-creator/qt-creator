/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "qtscripteditoractionhandler.h"
#include "qtscripteditorconstants.h"
#include "qtscripteditor.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/scriptmanager/scriptmanager.h>

#include <QtCore/QDebug>
#include <QtGui/QAction>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

static QAction *actionFromId(const QString &id)
{
    Core::Command *cmd = Core::ICore::instance()->actionManager()->command(id);
    if (!cmd)
        return 0;
    return cmd->action();
}

namespace QtScriptEditor {
namespace Internal {

QtScriptEditorActionHandler::QtScriptEditorActionHandler()
  : TextEditor::TextEditorActionHandler(QLatin1String(QtScriptEditor::Constants::C_QTSCRIPTEDITOR),
                                        Format),
    m_runAction(0)
{
}

void QtScriptEditorActionHandler::createActions()
{
    TextEditor::TextEditorActionHandler::createActions();
    m_runAction = actionFromId(QLatin1String(QtScriptEditor::Constants::RUN));
    connect(m_runAction, SIGNAL(triggered()), this, SLOT(run()));
}


void QtScriptEditorActionHandler::run()
{
    typedef Core::ScriptManager::Stack Stack;
    if (!currentEditor())
        return;

    const QString script = currentEditor()->toPlainText();

    // run
    Stack errorStack;
    QString errorMessage;
    if (Core::ICore::instance()->scriptManager()->runScript(script, &errorMessage, &errorStack))
        return;

    // try to find a suitable error line in the stack
    // ignoring 0 and other files (todo: open other files?)
    int errorLineNumber = 0;
    if (const int numFrames = errorStack.size()) {
        for (int f = 0; f < numFrames; f++) {
            if (errorStack[f].lineNumber && errorStack[f].fileName.isEmpty()) {
                errorLineNumber = errorStack[f].lineNumber;
                break;
            }
        }
    }
    if (errorLineNumber)
        currentEditor()->gotoLine(errorLineNumber);
    QMessageBox::critical(Core::ICore::instance()->mainWindow(), tr("Qt Script Error"), errorMessage);
}

} // namespace Internal
} // namespace QtScriptEditor
