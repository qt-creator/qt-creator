#include "vcprojectfile.h"

#include "vcprojectmanagerconstants.h"
#include "vcprojectmodel/vcdocumentmodel.h"
#include "vcprojectmodel/vcprojectdocument.h"
#include "vcprojectmodel/vcdocprojectnodes.h"

#include <QFileInfo>

namespace VcProjectManager {
namespace Internal {

VcProjectFile::VcProjectFile(const QString &filePath, VcDocConstants::DocumentVersion docVersion)
    : m_filePath(filePath)
    , m_path(QFileInfo(filePath).path())
{
    m_documentModel = new VcProjectManager::Internal::VcDocumentModel(filePath, docVersion);
}

VcProjectFile::~VcProjectFile()
{
    delete m_documentModel;
}

bool VcProjectFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(fileName)
    Q_UNUSED(autoSave)
    // TODO: obvious
    return false;
}

QString VcProjectFile::fileName() const
{
    return QFileInfo(m_filePath).fileName();
}

QString VcProjectFile::defaultPath() const
{
    // TODO: what's this for?
    return QString();
}

QString VcProjectFile::suggestedFileName() const
{
    // TODO: what's this for?
    return QString();
}

QString VcProjectFile::mimeType() const
{
    return QLatin1String(Constants::VCPROJ_MIMETYPE);
}

bool VcProjectFile::isModified() const
{
    // TODO: obvious
    return false;
}

bool VcProjectFile::isSaveAsAllowed() const
{
    return false;
}

bool VcProjectFile::reload(QString *errorString, Core::IDocument::ReloadFlag flag, Core::IDocument::ChangeType type)
{
    Q_UNUSED(errorString);
    Q_UNUSED(flag);
    Q_UNUSED(type);

    // TODO: what's this for?
    return false;
}

void VcProjectFile::rename(const QString &newName)
{
    Q_UNUSED(newName);

    // TODO: obvious
}

QString VcProjectFile::filePath()
{
    return m_filePath;
}

QString VcProjectFile::path()
{
    return m_path;
}

VcDocProjectNode *VcProjectFile::createVcDocNode() const
{
    if (m_documentModel)
        return new VcDocProjectNode(m_documentModel->vcProjectDocument());
    return 0;
}

void VcProjectFile::reloadVcDoc()
{
    VcDocConstants::DocumentVersion docVersion = m_documentModel->vcProjectDocument()->documentVersion();
    delete m_documentModel;
    m_documentModel = 0;
    m_documentModel = new VcDocumentModel(m_filePath, docVersion);
}

VcDocumentModel *VcProjectFile::documentModel() const
{
    return m_documentModel;
}

} // namespace Internal
} // namespace VcProjectManager
