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

#include "genericprojectfileseditor.h"
#include "genericprojectmanager.h"
#include "genericprojectconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

#include <QCoreApplication>
#include <QSharedPointer>

using namespace TextEditor;

namespace GenericProjectManager {
namespace Internal {

////////////////////////////////////////////////////////////////////////////////////////
//
// ProjectFilesFactory
//
////////////////////////////////////////////////////////////////////////////////////////

ProjectFilesFactory::ProjectFilesFactory(Manager *manager)
    : Core::IEditorFactory(manager)
{
    setId(Constants::FILES_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", ".files Editor"));
    addMimeType(Constants::FILES_MIMETYPE);
    addMimeType(Constants::INCLUDES_MIMETYPE);
    addMimeType(Constants::CONFIG_MIMETYPE);
    new TextEditor::TextEditorActionHandler(this, Constants::C_FILESEDITOR);

}

Core::IEditor *ProjectFilesFactory::createEditor()
{
    auto widget = new ProjectFilesEditorWidget;
    widget->setSimpleTextDocument(Constants::FILES_EDITOR_ID);
    TextEditorSettings::initializeEditor(widget);
    return widget->editor();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// ProjectFilesEditable
//
////////////////////////////////////////////////////////////////////////////////////////

ProjectFilesEditor::ProjectFilesEditor()
{
    setContext(Core::Context(Constants::C_FILESEDITOR));
    setDuplicateSupported(true);
}

Core::IEditor *ProjectFilesEditor::duplicate()
{
    auto widget = new ProjectFilesEditorWidget;
    widget->setTextDocument(editorWidget()->textDocumentPtr());
    TextEditorSettings::initializeEditor(widget);
    return widget->editor();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// ProjectFilesEditor
//
////////////////////////////////////////////////////////////////////////////////////////

ProjectFilesEditorWidget::ProjectFilesEditorWidget()
{
}

BaseTextEditor *ProjectFilesEditorWidget::createEditor()
{
    return new ProjectFilesEditor;
}

} // namespace Internal
} // namespace GenericProjectManager
