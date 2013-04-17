#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECTFILE_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECTFILE_H

#include <coreplugin/idocument.h>

#include "vcprojectmodel/vcprojectdocument_constants.h"

#include <QString>

namespace VcProjectManager {
namespace Internal {

class VcDocProjectNode;
class VcDocumentModel;

class VcProjectFile : public Core::IDocument
{
    Q_OBJECT

public:
    VcProjectFile(const QString &filePath, VcDocConstants::DocumentVersion docVersion);
    ~VcProjectFile();

    bool save(QString *errorString, const QString &fileName = QString(), bool autoSave = false);
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isSaveAsAllowed() const;

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    void rename(const QString &newName);

    QString filePath();
    QString path();

    VcDocProjectNode* createVcDocNode() const;
    void reloadVcDoc();
    VcDocumentModel* documentModel() const;
private:
    QString m_filePath;
    QString m_path;
    VcDocumentModel* m_documentModel;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECTFILE_H
