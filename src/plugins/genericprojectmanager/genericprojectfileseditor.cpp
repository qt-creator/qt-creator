#include "genericprojectfileseditor.h"
#include "genericprojectmanager.h"
#include "genericprojectconstants.h"

#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;


////////////////////////////////////////////////////////////////////////////////////////
// ProjectFilesFactory
////////////////////////////////////////////////////////////////////////////////////////

ProjectFilesFactory::ProjectFilesFactory(Manager *manager,
                                         TextEditor::TextEditorActionHandler *handler)
    : Core::IEditorFactory(manager),
      m_manager(manager),
      m_actionHandler(handler)
{
    m_mimeTypes.append(QLatin1String(Constants::FILES_MIMETYPE));
    m_mimeTypes.append(QLatin1String(Constants::INCLUDES_MIMETYPE));
    m_mimeTypes.append(QLatin1String(Constants::CONFIG_MIMETYPE));
}

ProjectFilesFactory::~ProjectFilesFactory()
{
}

Manager *ProjectFilesFactory::manager() const
{
    return m_manager;
}

Core::IEditor *ProjectFilesFactory::createEditor(QWidget *parent)
{
    ProjectFilesEditor *ed = new ProjectFilesEditor(parent, this, m_actionHandler);
    TextEditor::TextEditorSettings::instance()->initializeEditor(ed);
    return ed->editableInterface();
}

QStringList ProjectFilesFactory::mimeTypes() const
{
    return m_mimeTypes;
}

QString ProjectFilesFactory::kind() const
{
    return QLatin1String(Constants::FILES_EDITOR);
}

Core::IFile *ProjectFilesFactory::open(const QString &fileName)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();

    if (Core::IEditor *editor = editorManager->openEditor(fileName, kind()))
        return editor->file();

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// ProjectFilesEditable
////////////////////////////////////////////////////////////////////////////////////////

ProjectFilesEditable::ProjectFilesEditable(ProjectFilesEditor *editor)
    : TextEditor::BaseTextEditorEditable(editor)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(Constants::C_FILESEDITOR);
}

ProjectFilesEditable::~ProjectFilesEditable()
{ }

QList<int> ProjectFilesEditable::context() const
{
    return m_context;
}

const char *ProjectFilesEditable::kind() const
{
    return Constants::FILES_EDITOR;
}

bool ProjectFilesEditable::duplicateSupported() const
{
    return true;
}

Core::IEditor *ProjectFilesEditable::duplicate(QWidget *parent)
{
    ProjectFilesEditor *parentEditor = qobject_cast<ProjectFilesEditor *>(editor());
    ProjectFilesEditor *editor = new ProjectFilesEditor(parent,
                                                        parentEditor->factory(),
                                                        parentEditor->actionHandler());
    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);
    return editor->editableInterface();
}

////////////////////////////////////////////////////////////////////////////////////////
// ProjectFilesEditor
////////////////////////////////////////////////////////////////////////////////////////

ProjectFilesEditor::ProjectFilesEditor(QWidget *parent, ProjectFilesFactory *factory,
                                       TextEditor::TextEditorActionHandler *handler)
    : TextEditor::BaseTextEditor(parent),
      m_factory(factory),
      m_actionHandler(handler)
{
    Manager *manager = factory->manager();
    ProjectFilesDocument *doc = new ProjectFilesDocument(manager);
    setBaseTextDocument(doc);

    handler->setupActions(this);
}

ProjectFilesEditor::~ProjectFilesEditor()
{ }

ProjectFilesFactory *ProjectFilesEditor::factory() const
{
    return m_factory;
}

TextEditor::TextEditorActionHandler *ProjectFilesEditor::actionHandler() const
{
    return m_actionHandler;
}

TextEditor::BaseTextEditorEditable *ProjectFilesEditor::createEditableInterface()
{
    return new ProjectFilesEditable(this);
}

////////////////////////////////////////////////////////////////////////////////////////
// ProjectFilesDocument
////////////////////////////////////////////////////////////////////////////////////////

ProjectFilesDocument::ProjectFilesDocument(Manager *manager)
    : m_manager(manager)
{
    setMimeType(QLatin1String(Constants::FILES_MIMETYPE));
}

ProjectFilesDocument::~ProjectFilesDocument()
{ }

bool ProjectFilesDocument::save(const QString &name)
{
    if (! BaseTextDocument::save(name))
        return false;

    m_manager->notifyChanged(name);
    return true;
}
