#include "vcprojectfile.h"

#include "vcprojectmanagerconstants.h"

#include <QFileInfo>

namespace VcProjectManager {
namespace Internal {

VcProjectFile::VcProjectFile(const QString &filePath)
    : m_filePath(filePath)
    , m_path(QFileInfo(filePath).path())
{
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

} // namespace Internal
} // namespace VcProjectManager
