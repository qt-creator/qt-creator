/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLPROJECTFILESEDITOR_H
#define QMLPROJECTFILESEDITOR_H

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextdocument.h>

#include <coreplugin/editormanager/ieditorfactory.h>

namespace QmlProjectManager {
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
    virtual QString kind() const;
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

    virtual QList<int> context() const;
    virtual const char *kind() const;

    virtual bool duplicateSupported() const;
    virtual Core::IEditor *duplicate(QWidget *parent);
    virtual bool isTemporary() const { return false; }

private:
    QList<int> m_context;
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
} // end of namespace QmlProjectManager

#endif // QMLPROJECTFILESEDITOR_H
