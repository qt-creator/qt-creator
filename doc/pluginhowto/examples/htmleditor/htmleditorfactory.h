#ifndef HTMLEDITORFACTORY_H
#define HTMLEDITORFACTORY_H

#include "coreplugin/editormanager/ieditorfactory.h"

class HTMLEditorPlugin;

struct HTMLEditorFactoryData;
class HTMLEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    HTMLEditorFactory(HTMLEditorPlugin* owner);
    ~HTMLEditorFactory();

    QStringList mimeTypes() const;
    QString kind() const;

    Core::IEditor* createEditor(QWidget* parent);
    Core::IFile* open(const QString &fileName);

private:
    HTMLEditorFactoryData* d;
};

#endif // HTMLEDITORFACTORY_H
