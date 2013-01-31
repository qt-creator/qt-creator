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

ProjectFilesFactory::ProjectFilesFactory(Manager *manager, TextEditorActionHandler *handler)
    : Core::IEditorFactory(manager),
      m_actionHandler(handler)
{
    m_mimeTypes.append(QLatin1String(Constants::FILES_MIMETYPE));
    m_mimeTypes.append(QLatin1String(Constants::INCLUDES_MIMETYPE));
    m_mimeTypes.append(QLatin1String(Constants::CONFIG_MIMETYPE));
}

Core::IEditor *ProjectFilesFactory::createEditor(QWidget *parent)
{
    ProjectFilesEditorWidget *ed = new ProjectFilesEditorWidget(parent, this, m_actionHandler);
    TextEditorSettings::instance()->initializeEditor(ed);
    return ed->editor();
}

QStringList ProjectFilesFactory::mimeTypes() const
{
    return m_mimeTypes;
}

Core::Id ProjectFilesFactory::id() const
{
    return Core::Id(Constants::FILES_EDITOR_ID);
}

QString ProjectFilesFactory::displayName() const
{
    return  QCoreApplication::translate("OpenWith::Editors", ".files Editor");
}

////////////////////////////////////////////////////////////////////////////////////////
//
// ProjectFilesEditable
//
////////////////////////////////////////////////////////////////////////////////////////

ProjectFilesEditor::ProjectFilesEditor(ProjectFilesEditorWidget *editor)
  : BaseTextEditor(editor)
{
   setContext(Core::Context(Constants::C_FILESEDITOR));
}

Core::Id ProjectFilesEditor::id() const
{
    return Core::Id(Constants::FILES_EDITOR_ID);
}

bool ProjectFilesEditor::duplicateSupported() const
{
    return true;
}

Core::IEditor *ProjectFilesEditor::duplicate(QWidget *parent)
{
    ProjectFilesEditorWidget *parentEditor = qobject_cast<ProjectFilesEditorWidget *>(editorWidget());
    ProjectFilesEditorWidget *editor = new ProjectFilesEditorWidget(parent,
                                                        parentEditor->factory(),
                                                        parentEditor->actionHandler());
    TextEditorSettings::instance()->initializeEditor(editor);
    return editor->editor();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// ProjectFilesEditor
//
////////////////////////////////////////////////////////////////////////////////////////

ProjectFilesEditorWidget::ProjectFilesEditorWidget(QWidget *parent, ProjectFilesFactory *factory,
                                       TextEditorActionHandler *handler)
    : BaseTextEditorWidget(parent),
      m_factory(factory),
      m_actionHandler(handler)
{
    QSharedPointer<BaseTextDocument> doc(new BaseTextDocument());
    setBaseTextDocument(doc);

    handler->setupActions(this);
}

ProjectFilesFactory *ProjectFilesEditorWidget::factory() const
{
    return m_factory;
}

TextEditorActionHandler *ProjectFilesEditorWidget::actionHandler() const
{
    return m_actionHandler;
}

BaseTextEditor *ProjectFilesEditorWidget::createEditor()
{
    return new ProjectFilesEditor(this);
}

} // namespace Internal
} // namespace GenericProjectManager
