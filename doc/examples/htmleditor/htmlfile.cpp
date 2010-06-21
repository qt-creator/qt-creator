#include "htmlfile.h"
#include<QFile>
#include<QFileInfo>

namespace HTMLEditorConstants
{
    const char* const C_HTMLEDITOR_MIMETYPE = "text/html";
    const char* const C_HTMLEDITOR = "HTML Editor";
}
struct HTMLFileData
{
    HTMLFileData(): mimeType(HTMLEditorConstants::C_HTMLEDITOR),
    editorWidget(0), editor(0), modified(false) { }
    const QString mimeType;
    HTMLEditorWidget* editorWidget;
    HTMLEditor* editor;
    QString fileName;
    bool modified;
};

HTMLFile::HTMLFile(HTMLEditor* editor, HTMLEditorWidget* editorWidget)
    : Core::IFile(editor)
{
    d = new HTMLFileData;
    d->editor = editor;
    d->editorWidget = editorWidget;
}


HTMLFile::~HTMLFile()
{
    delete d;
}

void HTMLFile::setModified(bool val)
{
    if(d->modified == val)
        return;

    d->modified = val;
    emit changed();
}

bool HTMLFile::isModified() const
{
    return d->modified;
}

QString HTMLFile::mimeType() const
{
    return d->mimeType;
}

bool HTMLFile::save(const QString &fileName)
{
    QFile file(fileName);

    if(file.open(QFile::WriteOnly))
    {
        d->fileName = fileName;

        QByteArray content = d->editorWidget->content();
        file.write(content);

        setModified(false);

        return true;
    }
    return false;
}

bool HTMLFile::open(const QString &fileName)
{
    QFile file(fileName);

    if(file.open(QFile::ReadOnly))
    {
        d->fileName = fileName;

        QString path = QFileInfo(fileName).absolutePath();
        d->editorWidget->setContent(file.readAll(), path);
        d->editor->setDisplayName(d->editorWidget->title());

        return true;
    }
    return false;
}

void HTMLFile::setFilename(const QString& filename)
{
    d->fileName = filename;
}

QString HTMLFile::fileName() const
{
    return d->fileName;
}
QString HTMLFile::defaultPath() const
{
    return QString();
}

QString HTMLFile::suggestedFileName() const
{
    return QString();
}

QString HTMLFile::fileFilter() const
{
    return QString();
}

QString HTMLFile::fileExtension() const
{
    return QString();
}

bool HTMLFile::isReadOnly() const
{
    return false;
}
bool HTMLFile::isSaveAsAllowed() const
{
    return true;
}

void HTMLFile::modified(ReloadBehavior* behavior)
{
    Q_UNUSED(behavior);
}
