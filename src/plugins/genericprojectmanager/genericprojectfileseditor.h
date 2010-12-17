/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
class ProjectFilesEditable;
class ProjectFilesEditor;
class ProjectFilesDocument;
class ProjectFilesFactory;

class ProjectFilesFactory: public Core::IEditorFactory
{
    Q_OBJECT

public:
    ProjectFilesFactory(Manager *manager, TextEditor::TextEditorActionHandler *handler);
    virtual ~ProjectFilesFactory();

    Manager *manager() const;

    virtual Core::IEditor *createEditor(QWidget *parent);

    virtual QStringList mimeTypes() const;
    virtual QString id() const;
    virtual QString displayName() const;
    virtual Core::IFile *open(const QString &fileName);

private:
    Manager *m_manager;
    TextEditor::TextEditorActionHandler *m_actionHandler;
    QStringList m_mimeTypes;
};

class ProjectFilesEditable: public TextEditor::BaseTextEditorEditable
{
    Q_OBJECT

public:
    ProjectFilesEditable(ProjectFilesEditor *editor);
    virtual ~ProjectFilesEditable();

    virtual Core::Context context() const;
    virtual QString id() const;

    virtual bool duplicateSupported() const;
    virtual Core::IEditor *duplicate(QWidget *parent);

    virtual bool isTemporary() const { return false; }

private:
    Core::Context m_context;
};

class ProjectFilesEditor: public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    ProjectFilesEditor(QWidget *parent, ProjectFilesFactory *factory,
                       TextEditor::TextEditorActionHandler *handler);
    virtual ~ProjectFilesEditor();

    ProjectFilesFactory *factory() const;
    TextEditor::TextEditorActionHandler *actionHandler() const;

    virtual TextEditor::BaseTextEditorEditable *createEditableInterface();

private:
    ProjectFilesFactory *m_factory;
    TextEditor::TextEditorActionHandler *m_actionHandler;
};

class ProjectFilesDocument: public TextEditor::BaseTextDocument
{
    Q_OBJECT

public:
    ProjectFilesDocument(Manager *manager);
    virtual ~ProjectFilesDocument();

    virtual bool save(const QString &name);

private:
    Manager *m_manager;
};

} // end of namespace Internal
} // end of namespace GenericProjectManager

#endif // GENERICPROJECTFILESEDITOR_H
