#ifndef GENERICPROJECTFILESEDITOR_H
#define GENERICPROJECTFILESEDITOR_H

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextdocument.h>

#include <coreplugin/editormanager/ieditorfactory.h>

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
    virtual QString kind() const;
    virtual Core::IFile *open(const QString &fileName);

private:
    Manager *_manager;
    TextEditor::TextEditorActionHandler *_actionHandler;
    QStringList _mimeTypes;
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

private:
    QList<int> _context;
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
    ProjectFilesFactory *_factory;
    TextEditor::TextEditorActionHandler *_actionHandler;
};

class ProjectFilesDocument: public TextEditor::BaseTextDocument
{
    Q_OBJECT

public:
    ProjectFilesDocument(Manager *manager);
    virtual ~ProjectFilesDocument();

    virtual bool save(const QString &name);

private:
    Manager *_manager;
};

} // end of namespace Internal
} // end of namespace GenericProjectManager

#endif // GENERICPROJECTFILESEDITOR_H
