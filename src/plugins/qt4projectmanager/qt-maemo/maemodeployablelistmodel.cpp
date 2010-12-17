/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "maemodeployablelistmodel.h"

#include "maemotoolchain.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4target.h>

#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QBrush>
#include <QtGui/QImageReader>

namespace Qt4ProjectManager {
namespace Internal {
namespace {
const QLatin1String RemoteIconPath("/usr/share/icons/hicolor/64x64/apps");
} // anonymous namespace

MaemoDeployableListModel::MaemoDeployableListModel(const Qt4ProFileNode *proFileNode,
    ProFileUpdateSetting updateSetting, QObject *parent)
    : QAbstractTableModel(parent),
      m_projectType(proFileNode->projectType()),
      m_proFilePath(proFileNode->path()),
      m_projectName(proFileNode->displayName()),
      m_targetInfo(proFileNode->targetInformation()),
      m_installsList(proFileNode->installsList()),
      m_config(proFileNode->variableValue(ConfigVar)),
      m_modified(false),
      m_proFileUpdateSetting(updateSetting),
      m_hasTargetPath(false)
{
    buildModel();
}

MaemoDeployableListModel::~MaemoDeployableListModel() {}

bool MaemoDeployableListModel::buildModel()
{
    m_deployables.clear();

    m_hasTargetPath = !m_installsList.targetPath.isEmpty();
    if (!m_hasTargetPath && m_proFileUpdateSetting == UpdateProFile) {
        const QString remoteDirSuffix
            = QLatin1String(m_projectType == LibraryTemplate
                ? "/lib" : "/bin");
        const QString remoteDir = QLatin1String("target.path = ")
            + installPrefix() + remoteDirSuffix;
        const QStringList deployInfo = QStringList() << remoteDir
            << QLatin1String("INSTALLS += target");
        return addLinesToProFile(deployInfo);
    } else {
        m_deployables.prepend(MaemoDeployable(localExecutableFilePath(),
            m_installsList.targetPath));
    }
    foreach (const InstallsItem &elem, m_installsList.items) {
        foreach (const QString &file, elem.files)
            m_deployables << MaemoDeployable(file, elem.path);
    }

    m_modified = true;
    return true;
}

MaemoDeployable MaemoDeployableListModel::deployableAt(int row) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    return m_deployables.at(row);
}

int MaemoDeployableListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_deployables.count();
}

int MaemoDeployableListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant MaemoDeployableListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    if (isEditable(index)) {
        if (role == Qt::DisplayRole)
            return tr("<no target path set>");
        if (role == Qt::ForegroundRole) {
            QBrush brush;
            brush.setColor("red");
            return brush;
        }
    }

    const MaemoDeployable &d = deployableAt(index.row());
    if (index.column() == 0 && role == Qt::DisplayRole)
        return QDir::toNativeSeparators(d.localFilePath);
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return d.remoteDir;
    return QVariant();
}

Qt::ItemFlags MaemoDeployableListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags parentFlags = QAbstractTableModel::flags(index);
    if (isEditable(index))
        return parentFlags | Qt::ItemIsEditable;
    return parentFlags;
}

bool MaemoDeployableListModel::setData(const QModelIndex &index,
                                   const QVariant &value, int role)
{
    if (!isEditable(index) || role != Qt::EditRole)
        return false;
    const QString &remoteDir = value.toString();
    if (!addLinesToProFile(QStringList()
             << QString::fromLocal8Bit("target.path = %1").arg(remoteDir)
             << QLatin1String("INSTALLS += target")))
        return false;
    m_deployables.first().remoteDir = remoteDir;
    emit dataChanged(index, index);
    return true;
}

QVariant MaemoDeployableListModel::headerData(int section,
             Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Local File Path") : tr("Remote Directory");
}

QString MaemoDeployableListModel::localExecutableFilePath() const
{
    if (!m_targetInfo.valid)
        return QString();

    const bool isLib = m_projectType == LibraryTemplate;
    bool isStatic = false; // Nonsense init for stupid compilers.
    QString fileName;
    if (isLib) {
        fileName += QLatin1String("lib");
        isStatic = m_config.contains(QLatin1String("static"))
            || m_config.contains(QLatin1String("staticlib"));
    }
    fileName += m_targetInfo.target;
    if (isLib)
        fileName += QLatin1String(isStatic ? ".a" : ".so");
    return QDir::cleanPath(m_targetInfo.workingDir + '/' + fileName);
}

QString MaemoDeployableListModel::remoteExecutableFilePath() const
{
    return m_hasTargetPath
        ? deployableAt(0).remoteDir + '/'
              + QFileInfo(localExecutableFilePath()).fileName()
        : QString();
}

QString MaemoDeployableListModel::projectDir() const
{
    return QFileInfo(m_proFilePath).dir().path();
}

void MaemoDeployableListModel::setProFileUpdateSetting(ProFileUpdateSetting updateSetting)
{
    m_proFileUpdateSetting = updateSetting;
    if (updateSetting == UpdateProFile)
        buildModel();
}

bool MaemoDeployableListModel::isEditable(const QModelIndex &index) const
{
    return index.row() == 0 && index.column() == 1
        && m_deployables.first().remoteDir.isEmpty();
}

QString MaemoDeployableListModel::localDesktopFilePath() const
{
    if (m_projectType == LibraryTemplate)
        return QString();
    foreach (const MaemoDeployable &d, m_deployables) {
        if (QFileInfo(d.localFilePath).fileName() == m_projectName + QLatin1String(".desktop"))
            return d.localFilePath;
    }
    return QString();
}

bool MaemoDeployableListModel::addDesktopFile(QString &error)
{
    if (!canAddDesktopFile())
        return true;
    const QString desktopFilePath = QFileInfo(m_proFilePath).path()
        + QLatin1Char('/') + m_projectName + QLatin1String(".desktop");
    QFile desktopFile(desktopFilePath);
    const bool existsAlready = desktopFile.exists();
    if (!desktopFile.open(QIODevice::ReadWrite)) {
        error = tr("Failed to open '%1': %2")
            .arg(desktopFilePath, desktopFile.errorString());
        return false;
    }

    const QByteArray desktopTemplate("[Desktop Entry]\nEncoding=UTF-8\n"
        "Version=1.0\nType=Application\nTerminal=false\nName=%1\nExec=%2\n"
        "Icon=%1\nX-Window-Icon=\nX-HildonDesk-ShowInToolbar=true\n"
        "X-Osso-Type=application/x-executable\n");
    const QString contents = existsAlready
        ? QString::fromUtf8(desktopFile.readAll())
        : QString::fromLocal8Bit(desktopTemplate)
              .arg(m_projectName, remoteExecutableFilePath());
    desktopFile.resize(0);
    const QByteArray &contentsAsByteArray = contents.toUtf8();
    if (desktopFile.write(contentsAsByteArray) != contentsAsByteArray.count()
            || !desktopFile.flush()) {
        error = tr("Could not write '%1': %2")
            .arg(desktopFilePath, desktopFile.errorString());
            return false;
    }

    const MaemoToolChain *const tc = maemoToolchain();
    QTC_ASSERT(tc, return false);
    QString remoteDir = QLatin1String("/usr/share/applications");
    if (tc->version() == MaemoToolChain::Maemo5)
        remoteDir += QLatin1String("/hildon");
    const QLatin1String filesLine("desktopfile.files = $${TARGET}.desktop");
    const QString pathLine = QLatin1String("desktopfile.path = ") + remoteDir;
    const QLatin1String installsLine("INSTALLS += desktopfile");
    if (!addLinesToProFile(QStringList() << filesLine << pathLine
            << installsLine)) {
        error = tr("Error writing project file.");
        return false;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_deployables << MaemoDeployable(desktopFilePath, remoteDir);
    endInsertRows();
    return true;
}

bool MaemoDeployableListModel::addIcon(const QString &fileName, QString &error)
{
    if (!canAddIcon())
        return true;

    const QString filesLine = QLatin1String("icon.files = ") + fileName;
    const QString pathLine = QLatin1String("icon.path = ") + RemoteIconPath;
    const QLatin1String installsLine("INSTALLS += icon");
    if (!addLinesToProFile(QStringList() << filesLine << pathLine
            << installsLine)) {
        error = tr("Error writing project file.");
        return false;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    const QString filePath = QFileInfo(m_proFilePath).path()
        + QLatin1Char('/') + fileName;
    m_deployables << MaemoDeployable(filePath, RemoteIconPath);
    endInsertRows();
    return true;
}

QString MaemoDeployableListModel::remoteIconFilePath() const
{
    if (m_projectType == LibraryTemplate)
        return QString();
    const QList<QByteArray> &imageTypes = QImageReader::supportedImageFormats();
    foreach (const MaemoDeployable &d, m_deployables) {
        const QByteArray extension
            = QFileInfo(d.localFilePath).suffix().toLocal8Bit();
        if (d.remoteDir.startsWith(RemoteIconPath)
                && imageTypes.contains(extension))
            return d.remoteDir + QLatin1Char('/')
                + QFileInfo(d.localFilePath).fileName();
    }
    return QString();
}

bool MaemoDeployableListModel::addLinesToProFile(const QStringList &lines)
{
    QFile projectFile(m_proFilePath);
    if (!projectFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning("Error opening .pro file for writing.");
        return false;
    }
    const QLatin1String separator("\n    ");
    const QString proFileString = QString(QLatin1Char('\n') + proFileScope()
        + QLatin1String(" {") + separator + lines.join(separator)
        + QLatin1String("\n}\n"));
    const QByteArray &proFileByteArray = proFileString.toLocal8Bit();
    if (projectFile.write(proFileByteArray) != proFileByteArray.count()
            || !projectFile.flush()) {
        qWarning("Error updating .pro file.");
        return false;
    }
    return true;
}

const MaemoToolChain *MaemoDeployableListModel::maemoToolchain() const
{
    const ProjectExplorer::Project *const activeProject
        = ProjectExplorer::ProjectExplorerPlugin::instance()->session()->startupProject();
    QTC_ASSERT(activeProject, return 0);
    const Qt4Target *const activeTarget
        = qobject_cast<Qt4Target *>(activeProject->activeTarget());
    QTC_ASSERT(activeTarget, return 0);
    const Qt4BuildConfiguration *const bc
        = activeTarget->activeBuildConfiguration();
    QTC_ASSERT(bc, return 0);
    const MaemoToolChain *const tc
        = dynamic_cast<MaemoToolChain *>(bc->toolChain());
    QTC_ASSERT(tc, return 0);
    return tc;
}

QString MaemoDeployableListModel::proFileScope() const
{
    const MaemoToolChain *const tc = maemoToolchain();
    QTC_ASSERT(tc, return QString());
    return QLatin1String(tc->version() == MaemoToolChain::Maemo5
        ? "maemo5" : "unix:!symbian:!maemo5");
}

QString MaemoDeployableListModel::installPrefix() const
{
    const MaemoToolChain *const tc = maemoToolchain();
    QTC_ASSERT(tc, return QString());
    return QLatin1String(tc->version() == MaemoToolChain::Maemo5
        ? "/opt/usr" : "/usr");
}

} // namespace Qt4ProjectManager
} // namespace Internal
