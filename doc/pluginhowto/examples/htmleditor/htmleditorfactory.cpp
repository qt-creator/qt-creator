#include "coreplugin/editormanager/editormanager.h"
#include "htmleditorfactory.h"
#include "htmleditorwidget.h"
#include "htmleditorplugin.h"
#include "htmleditor.h"

namespace HTMLEditorConstants
{
    const char* const C_HTMLEDITOR_MIMETYPE = "text/html";
    const char* const C_HTMLEDITOR = "HTML Editor";
}

struct HTMLEditorFactoryData
{
    HTMLEditorFactoryData() : kind(HTMLEditorConstants::C_HTMLEDITOR)
    {
        mimeTypes << QString(HTMLEditorConstants::C_HTMLEDITOR_MIMETYPE);
    }

    QString kind;
    QStringList mimeTypes;
};


HTMLEditorFactory::HTMLEditorFactory(HTMLEditorPlugin* owner)
    :Core::IEditorFactory(owner)
{
    d = new HTMLEditorFactoryData;
}

HTMLEditorFactory::~HTMLEditorFactory()
{
    delete d;
}

QStringList HTMLEditorFactory::mimeTypes() const
{
    return d->mimeTypes;
}

QString HTMLEditorFactory::kind() const
{
    return d->kind;
}

Core::IFile* HTMLEditorFactory::open(const QString& fileName)
{
    Core::EditorManager* em = Core::EditorManager::instance();
    Core::IEditor* iface = em->openEditor(fileName, d->kind);

    return iface ? iface->file() : 0;
}

Core::IEditor* HTMLEditorFactory::createEditor(QWidget* parent)
{
    HTMLEditorWidget* editorWidget = new HTMLEditorWidget(parent);

    return new HTMLEditor(editorWidget);
}


