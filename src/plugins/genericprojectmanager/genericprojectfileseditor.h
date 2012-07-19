/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef GENERICPROJECTFILESEDITOR_H
#define GENERICPROJECTFILESEDITOR_H

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextdocument.h>

#include <coreplugin/editormanager/ieditorfactory.h>

namespace TextEditor {
class TextEditorActionHandler;
}

namespace GenericProjectManager {
namespace Internal {

class Manager;
class ProjectFilesEditor;
class ProjectFilesEditorWidget;
class ProjectFilesFactory;

class ProjectFilesFactory: public Core::IEditorFactory
{
    Q_OBJECT

public:
    ProjectFilesFactory(Manager *manager, TextEditor::TextEditorActionHandler *handler);

    virtual Core::IEditor *createEditor(QWidget *parent);

    virtual QStringList mimeTypes() const;
    virtual Core::Id id() const;
    virtual QString displayName() const;

private:
    TextEditor::TextEditorActionHandler *m_actionHandler;
    QStringList m_mimeTypes;
};

class ProjectFilesEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    ProjectFilesEditor(ProjectFilesEditorWidget *editorWidget);

    virtual Core::Id id() const;
    virtual bool duplicateSupported() const;
    virtual Core::IEditor *duplicate(QWidget *parent);
    virtual bool isTemporary() const { return false; }
};

class ProjectFilesEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT

public:
    ProjectFilesEditorWidget(QWidget *parent, ProjectFilesFactory *factory,
                       TextEditor::TextEditorActionHandler *handler);
    virtual ~ProjectFilesEditorWidget();

    ProjectFilesFactory *factory() const;
    TextEditor::TextEditorActionHandler *actionHandler() const;

    virtual TextEditor::BaseTextEditor *createEditor();

private:
    ProjectFilesFactory *m_factory;
    TextEditor::TextEditorActionHandler *m_actionHandler;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICPROJECTFILESEDITOR_H
