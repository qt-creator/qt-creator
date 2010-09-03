/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "showineditortaskhandler.h"

#include "projectexplorerconstants.h"
#include "task.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/basetexteditor.h>

#include <QtGui/QAction>
#include <QtCore/QFileInfo>

using namespace ProjectExplorer::Internal;

ShowInEditorTaskHandler::ShowInEditorTaskHandler() :
    ITaskHandler(QLatin1String(Constants::SHOW_TASK_IN_EDITOR))
{ }

bool ShowInEditorTaskHandler::canHandle(const ProjectExplorer::Task &task)
{
    if (task.file.isEmpty())
        return false;
    QFileInfo fi(task.file);
    return fi.exists() && fi.isFile() && fi.isReadable();
}

void ShowInEditorTaskHandler::handle(const ProjectExplorer::Task &task)
{
    QFileInfo fi(task.file);
    TextEditor::BaseTextEditor::openEditorAt(fi.canonicalFilePath(), task.line);
}

QAction *ShowInEditorTaskHandler::createAction(QObject *parent)
{
    QAction *showAction = new QAction(tr("&Show in editor"), parent);
    showAction->setToolTip(tr("Show task location in an editor"));
    return showAction;
}
