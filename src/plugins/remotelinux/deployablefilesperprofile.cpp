/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "deployablefilesperprofile.h"

#include "maemoglobal.h"

#include <coreplugin/icore.h>
#include <coreplugin/filemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4target.h>
#include <qtsupport/baseqtversion.h>

#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QBrush>
#include <QtGui/QImageReader>
#include <QtGui/QMainWindow>

using namespace Qt4ProjectManager;

namespace RemoteLinux {
using namespace Internal;

DeployableFilesPerProFile::DeployableFilesPerProFile(const Qt4BaseTarget *target,
    const Qt4ProFileNode *proFileNode, ProFileUpdateSetting updateSetting, QObject *parent)
    : QAbstractTableModel(parent),
      m_target(target),
      m_projectType(proFileNode->projectType()),
      m_proFilePath(proFileNode->path()),
      m_projectName(proFileNode->displayName()),
      m_targetInfo(proFileNode->targetInformation()),
      m_installsList(proFileNode->installsList()),
      m_projectVersion(proFileNode->projectVersion()),
      m_config(proFileNode->variableValue(ConfigVar)),
      m_modified(false),
      m_proFileUpdateSetting(updateSetting),
      m_hasTargetPath(false)
{
    buildModel();
}

DeployableFilesPerProFile::~DeployableFilesPerProFile() {}

bool DeployableFilesPerProFile::buildModel()
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
    } else if (m_projectType == ApplicationTemplate) {
        m_deployables.prepend(DeployableFile(localExecutableFilePath(),
            m_installsList.targetPath));
    } else if (m_projectType == LibraryTemplate) {
        foreach (const QString &filePath, localLibraryFilePaths()) {
            m_deployables.prepend(DeployableFile(filePath,
                m_installsList.targetPath));
        }
    }
    foreach (const InstallsItem &elem, m_installsList.items) {
        foreach (const QString &file, elem.files)
            m_deployables << DeployableFile(file, elem.path);
    }

    m_modified = true;
    return true;
}

DeployableFile DeployableFilesPerProFile::deployableAt(int row) const
{
    Q_ASSERT(row >= 0 && row < rowCount());
    return m_deployables.at(row);
}

int DeployableFilesPerProFile::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_deployables.count();
}

int DeployableFilesPerProFile::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant DeployableFilesPerProFile::data(const QModelIndex &index, int role) const
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

    const DeployableFile &d = deployableAt(index.row());
    if (index.column() == 0 && role == Qt::DisplayRole)
        return QDir::toNativeSeparators(d.localFilePath);
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return d.remoteDir;
    return QVariant();
}

Qt::ItemFlags DeployableFilesPerProFile::flags(const QModelIndex &index) const
{
    Qt::ItemFlags parentFlags = QAbstractTableModel::flags(index);
    if (isEditable(index))
        return parentFlags | Qt::ItemIsEditable;
    return parentFlags;
}

bool DeployableFilesPerProFile::setData(const QModelIndex &index,
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

QVariant DeployableFilesPerProFile::headerData(int section,
             Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Local File Path") : tr("Remote Directory");
}

QString DeployableFilesPerProFile::localExecutableFilePath() const
{
    if (!m_targetInfo.valid || m_projectType != ApplicationTemplate)
        return QString();
    return QDir::cleanPath(m_targetInfo.workingDir + '/' + m_targetInfo.target);
}

QStringList DeployableFilesPerProFile::localLibraryFilePaths() const
{
    if (!m_targetInfo.valid || m_projectType != LibraryTemplate)
        return QStringList();
    QString basePath = m_targetInfo.workingDir + QLatin1String("/lib");
    const bool isStatic = m_config.contains(QLatin1String("static"))
            || m_config.contains(QLatin1String("staticlib"));
    basePath += m_targetInfo.target + QLatin1String(isStatic ? ".a" : ".so");
    basePath = QDir::cleanPath(basePath);
    const QChar dot(QLatin1Char('.'));
    const QString filePathMajor = basePath + dot
        + QString::number(m_projectVersion.major);
    const QString filePathMinor = filePathMajor + dot
         + QString::number(m_projectVersion.minor);
    const QString filePathPatch  = filePathMinor + dot
         + QString::number(m_projectVersion.patch);
    return QStringList() << filePathPatch << filePathMinor << filePathMajor
        << basePath;
}

QString DeployableFilesPerProFile::remoteExecutableFilePath() const
{
    return m_hasTargetPath && m_projectType == ApplicationTemplate
        ? deployableAt(0).remoteDir + '/'
              + QFileInfo(localExecutableFilePath()).fileName()
        : QString();
}

QString DeployableFilesPerProFile::projectDir() const
{
    return QFileInfo(m_proFilePath).dir().path();
}

void DeployableFilesPerProFile::setProFileUpdateSetting(ProFileUpdateSetting updateSetting)
{
    m_proFileUpdateSetting = updateSetting;
    if (updateSetting == UpdateProFile)
        buildModel();
}

bool DeployableFilesPerProFile::isEditable(const QModelIndex &index) const
{
    return m_projectType != AuxTemplate
        && index.row() == 0 && index.column() == 1
        && m_deployables.first().remoteDir.isEmpty();
}

QString DeployableFilesPerProFile::localDesktopFilePath() const
{
    if (m_projectType == LibraryTemplate)
        return QString();
    foreach (const DeployableFile &d, m_deployables) {
        if (QFileInfo(d.localFilePath).fileName() == m_projectName + QLatin1String(".desktop"))
            return d.localFilePath;
    }
    return QString();
}

bool DeployableFilesPerProFile::addDesktopFile()
{
    if (!canAddDesktopFile())
        return true;
    const QString desktopFilePath = QFileInfo(m_proFilePath).path()
        + QLatin1Char('/') + m_projectName + QLatin1String(".desktop");
    if (!QFile::exists(desktopFilePath)) {
        const QByteArray desktopTemplate("[Desktop Entry]\nEncoding=UTF-8\n"
            "Version=1.0\nType=Application\nTerminal=false\nName=%1\nExec=%2\n"
            "Icon=%1\nX-Window-Icon=\nX-HildonDesk-ShowInToolbar=true\n"
            "X-Osso-Type=application/x-executable\n");
        Utils::FileSaver saver(desktopFilePath);
        saver.write(QString::fromLatin1(desktopTemplate)
                    .arg(m_projectName, remoteExecutableFilePath()).toUtf8());
        if (!saver.finalize(Core::ICore::instance()->mainWindow()))
            return false;
    }

    const QtSupport::BaseQtVersion * const version = qtVersion();
    QTC_ASSERT(version && version->isValid(), return false);
    QString remoteDir = QLatin1String("/usr/share/applications");
    if (MaemoGlobal::osType(version->qmakeCommand()) == LinuxDeviceConfiguration::Maemo5OsType)
        remoteDir += QLatin1String("/hildon");
    const QLatin1String filesLine("desktopfile.files = $${TARGET}.desktop");
    const QString pathLine = QLatin1String("desktopfile.path = ") + remoteDir;
    const QLatin1String installsLine("INSTALLS += desktopfile");
    if (!addLinesToProFile(QStringList() << filesLine << pathLine
            << installsLine))
        return false;

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_deployables << DeployableFile(desktopFilePath, remoteDir);
    endInsertRows();
    return true;
}

bool DeployableFilesPerProFile::addIcon(const QString &fileName)
{
    if (!canAddIcon())
        return true;

    const QString filesLine = QLatin1String("icon.files = ") + fileName;
    const QString pathLine = QLatin1String("icon.path = ") + remoteIconDir();
    const QLatin1String installsLine("INSTALLS += icon");
    if (!addLinesToProFile(QStringList() << filesLine << pathLine
            << installsLine))
        return false;

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    const QString filePath = QFileInfo(m_proFilePath).path()
        + QLatin1Char('/') + fileName;
    m_deployables << DeployableFile(filePath, remoteIconDir());
    endInsertRows();
    return true;
}

QString DeployableFilesPerProFile::remoteIconFilePath() const
{
    if (m_projectType == LibraryTemplate)
        return QString();
    const QList<QByteArray> &imageTypes = QImageReader::supportedImageFormats();
    foreach (const DeployableFile &d, m_deployables) {
        const QByteArray extension
            = QFileInfo(d.localFilePath).suffix().toLocal8Bit();
        if (d.remoteDir.startsWith(remoteIconDir())
                && imageTypes.contains(extension))
            return d.remoteDir + QLatin1Char('/')
                + QFileInfo(d.localFilePath).fileName();
    }
    return QString();
}

bool DeployableFilesPerProFile::addLinesToProFile(const QStringList &lines)
{
    Core::FileChangeBlocker update(m_proFilePath);

    const QLatin1String separator("\n    ");
    const QString proFileString = QString(QLatin1Char('\n') + proFileScope()
        + QLatin1String(" {") + separator + lines.join(separator)
        + QLatin1String("\n}\n"));
    Utils::FileSaver saver(m_proFilePath, QIODevice::Append);
    saver.write(proFileString.toLocal8Bit());
    return saver.finalize(Core::ICore::instance()->mainWindow());
}

const QtSupport::BaseQtVersion *DeployableFilesPerProFile::qtVersion() const
{
    const Qt4BuildConfiguration *const bc = m_target->activeBuildConfiguration();
    QTC_ASSERT(bc, return 0);
    return bc->qtVersion();
}

QString DeployableFilesPerProFile::proFileScope() const
{
    const QtSupport::BaseQtVersion *const qv = qtVersion();
    QTC_ASSERT(qv && qv->isValid(), return QString());
    const QString osType = MaemoGlobal::osType(qv->qmakeCommand());
    if (osType == LinuxDeviceConfiguration::Maemo5OsType)
        return QLatin1String("maemo5");
    if (osType == LinuxDeviceConfiguration::HarmattanOsType)
        return QLatin1String("contains(MEEGO_EDITION,harmattan)");
    if (osType == LinuxDeviceConfiguration::MeeGoOsType)
        return QLatin1String("!isEmpty(MEEGO_VERSION_MAJOR):!contains(MEEGO_EDITION,harmattan)");
    return QLatin1String("unix:!symbian:!maemo5:isEmpty(MEEGO_VERSION_MAJOR)");
}

QString DeployableFilesPerProFile::installPrefix() const
{
    return QLatin1String("/opt/") + m_projectName;
}

QString DeployableFilesPerProFile::remoteIconDir() const
{
    const QtSupport::BaseQtVersion *const qv = qtVersion();
    QTC_ASSERT(qv && qv->isValid(), return QString());
    return QString::fromLocal8Bit("/usr/share/icons/hicolor/%1x%1/apps")
            .arg(MaemoGlobal::applicationIconSize(MaemoGlobal::osType(qv->qmakeCommand())));
}

} // namespace RemoteLinux
