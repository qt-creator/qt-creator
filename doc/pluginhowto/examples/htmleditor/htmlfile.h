#ifndef HTMLFILE_H
#define HTMLFILE_H
#include "coreplugin/ifile.h"
#include "htmleditorwidget.h"
#include "htmleditor.h"

struct HTMLFileData;
class HTMLFile : public Core::IFile
{
    Q_OBJECT

public:
    HTMLFile(HTMLEditor* editor, HTMLEditorWidget* editorWidget);
    ~HTMLFile();

    void setModified(bool val=true);
    bool isModified() const ;

    bool save(const QString &filename);
    bool open(const QString &filename);
    void setFilename(const QString &filename);
    QString mimiType(void) const ;
    QString fileName() const;
    QString defaultPath() const ;
    QString mimeType() const;
    QString suggestedFileName() const;
    QString fileFilter() const;
    QString fileExtension() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;
    void modified(ReloadBehavior* behavior);

protected slots:
    void modified() { setModified(true); }
private:
    HTMLFileData* d;
};
#endif // HTMLFILE_H
