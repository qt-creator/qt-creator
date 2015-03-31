/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlproject.h"
#include "qmlprojectfile.h"
#include "fileformat/qmlprojectfileformat.h"
#include "fileformat/qmlprojectitem.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlprojectconstants.h"
#include "qmlprojectnodes.h"
#include "qmlprojectmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/documentmanager.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtsupportconstants.h>

#include <QDebug>

using namespace Core;
using namespace ProjectExplorer;

namespace QmlProjectManager {
namespace Internal {

} // namespace Internal

QmlProject::QmlProject(Internal::Manager *manager, const Utils::FileName &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_defaultImport(UnknownImport),
      m_activeTarget(0)
{
    setId("QmlProjectManager.QmlProject");
    setProjectContext(Context(QmlProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguages(Context(ProjectExplorer::Constants::LANG_QMLJS));

    QFileInfo fileInfo = m_fileName.toFileInfo();
    m_projectName = fileInfo.completeBaseName();

    m_file = new Internal::QmlProjectFile(this, fileName);
    m_rootNode = new Internal::QmlProjectNode(this, m_file);

    DocumentManager::addDocument(m_file, true);

    m_manager->registerProject(this);
}

QmlProject::~QmlProject()
{
    m_manager->unregisterProject(this);

    DocumentManager::removeDocument(m_file);

    delete m_projectItem.data();
    delete m_rootNode;
}

void QmlProject::addedTarget(Target *target)
{
    connect(target, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(addedRunConfiguration(ProjectExplorer::RunConfiguration*)));
    foreach (RunConfiguration *rc, target->runConfigurations())
        addedRunConfiguration(rc);
}

void QmlProject::onActiveTargetChanged(Target *target)
{
    if (m_activeTarget)
        disconnect(m_activeTarget, &Target::kitChanged, this, &QmlProject::onKitChanged);
    m_activeTarget = target;
    if (m_activeTarget)
        connect(target, SIGNAL(kitChanged()), this, SLOT(onKitChanged()));

    // make sure e.g. the default qml imports are adapted
    refresh(Configuration);
}

void QmlProject::onKitChanged()
{
    // make sure e.g. the default qml imports are adapted
    refresh(Configuration);
}

void QmlProject::addedRunConfiguration(RunConfiguration *rc)
{
    // The enabled state of qml runconfigurations can only be decided after
    // they have been added to a project
    QmlProjectRunConfiguration *qmlrc = qobject_cast<QmlProjectRunConfiguration *>(rc);
    if (qmlrc)
        qmlrc->updateEnabled();
}

QDir QmlProject::projectDir() const
{
    return projectFilePath().toFileInfo().dir();
}

Utils::FileName QmlProject::filesFileName() const
{ return m_fileName; }

static QmlProject::QmlImport detectImport(const QString &qml) {
    static QRegExp qtQuick1RegExp(QLatin1String("import\\s+QtQuick\\s+1"));
    static QRegExp qtQuick2RegExp(QLatin1String("import\\s+QtQuick\\s+2"));

    if (qml.contains(qtQuick1RegExp))
        return QmlProject::QtQuick1Import;
    else if (qml.contains(qtQuick2RegExp))
        return QmlProject::QtQuick2Import;
    else
        return QmlProject::UnknownImport;
}

void QmlProject::parseProject(RefreshOptions options)
{
    if (options & Files) {
        if (options & ProjectFile)
            delete m_projectItem.data();
        if (!m_projectItem) {
              QString errorMessage;
              m_projectItem = QmlProjectFileFormat::parseProjectFile(m_fileName, &errorMessage);
              if (m_projectItem) {
                  connect(m_projectItem.data(), SIGNAL(qmlFilesChanged(QSet<QString>,QSet<QString>)),
                          this, SLOT(refreshFiles(QSet<QString>,QSet<QString>)));

              } else {
                  MessageManager::write(tr("Error while loading project file %1.")
                                        .arg(m_fileName.toUserOutput()),
                                        MessageManager::NoModeSwitch);
                  MessageManager::write(errorMessage);
              }
        }
        if (m_projectItem) {
            m_projectItem.data()->setSourceDirectory(projectDir().path());
            if (modelManager())
                modelManager()->updateSourceFiles(m_projectItem.data()->files(), true);

            QString mainFilePath = m_projectItem.data()->mainFile();
            if (!mainFilePath.isEmpty()) {
                mainFilePath = projectDir().absoluteFilePath(mainFilePath);
                Utils::FileReader reader;
                QString errorMessage;
                if (!reader.fetch(mainFilePath, &errorMessage)) {
                    MessageManager::write(tr("Warning while loading project file %1.")
                                          .arg(m_fileName.toUserOutput()));
                    MessageManager::write(errorMessage);
                } else {
                    m_defaultImport = detectImport(QString::fromUtf8(reader.data()));
                }
            }
        }
        m_rootNode->refresh();
    }

    if (options & Configuration) {
        // update configuration
    }

    if (options & Files)
        emit fileListChanged();
}

void QmlProject::refresh(RefreshOptions options)
{
    parseProject(options);

    if (options & Files)
        m_rootNode->refresh();

    if (!modelManager())
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager()->defaultProjectInfoForProject(this);
    foreach (const QString &searchPath, customImportPaths())
        projectInfo.importPaths.maybeInsert(Utils::FileName::fromString(searchPath),
                                            QmlJS::Dialect::Qml);

    modelManager()->updateProjectInfo(projectInfo, this);
}

QStringList QmlProject::convertToAbsoluteFiles(const QStringList &paths) const
{
    const QDir projectDir(m_fileName.toFileInfo().dir());
    QStringList absolutePaths;
    foreach (const QString &file, paths) {
        QFileInfo fileInfo(projectDir, file);
        absolutePaths.append(fileInfo.absoluteFilePath());
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

QmlJS::ModelManagerInterface *QmlProject::modelManager() const
{
    return QmlJS::ModelManagerInterface::instance();
}

QStringList QmlProject::files() const
{
    QStringList files;
    if (m_projectItem)
        files = m_projectItem.data()->files();
    else
        files = m_files;
    return files;
}

QString QmlProject::mainFile() const
{
    if (m_projectItem)
        return m_projectItem.data()->mainFile();
    return QString();
}

bool QmlProject::validProjectFile() const
{
    return !m_projectItem.isNull();
}

QStringList QmlProject::customImportPaths() const
{
    QStringList importPaths;
    if (m_projectItem)
        importPaths = m_projectItem.data()->importPaths();

    return importPaths;
}

bool QmlProject::addFiles(const QStringList &filePaths)
{
    QStringList toAdd;
    foreach (const QString &filePath, filePaths) {
        if (!m_projectItem.data()->matchesFile(filePath))
            toAdd << filePaths;
    }
    return toAdd.isEmpty();
}

void QmlProject::refreshProjectFile()
{
    refresh(QmlProject::ProjectFile | Files);
}

QmlProject::QmlImport QmlProject::defaultImport() const
{
    return m_defaultImport;
}

void QmlProject::refreshFiles(const QSet<QString> &/*added*/, const QSet<QString> &removed)
{
    refresh(Files);
    if (!removed.isEmpty() && modelManager())
        modelManager()->removeFiles(removed.toList());
}

QString QmlProject::displayName() const
{
    return m_projectName;
}

IDocument *QmlProject::document() const
{
    return m_file;
}

IProjectManager *QmlProject::projectManager() const
{
    return m_manager;
}

bool QmlProject::supportsKit(Kit *k, QString *errorMessage) const
{
    Id deviceType = DeviceTypeKitInformation::deviceTypeId(k);
    if (deviceType != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        if (errorMessage)
            *errorMessage = tr("Device type is not desktop.");
        return false;
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version) {
        if (errorMessage)
            *errorMessage = tr("No Qt version set in kit.");
        return false;
    }

    if (version->qtVersion() < QtSupport::QtVersionNumber(4, 7, 0)) {
        if (errorMessage)
            *errorMessage = tr("Qt version is too old.");
        return false;
    }

    if (version->qtVersion() < QtSupport::QtVersionNumber(5, 0, 0)
            && defaultImport() == QtQuick2Import) {
        if (errorMessage)
            *errorMessage = tr("Qt version is too old.");
        return false;
    }
    return true;
}

ProjectNode *QmlProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList QmlProject::files(FilesMode) const
{
    return files();
}

bool QmlProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    // refresh first - project information is used e.g. to decide the default RC's
    refresh(Everything);

    if (!activeTarget()) {
        // find a kit that matches prerequisites (prefer default one)
        QList<Kit*> kits = KitManager::matchingKits(
            std::function<bool(const Kit *)>([this](const Kit *k) -> bool {
                if (!k->isValid())
                    return false;

                IDevice::ConstPtr dev = DeviceKitInformation::device(k);
                if (dev.isNull() || dev->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
                    return false;
                QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
                if (!version || version->type() != QLatin1String(QtSupport::Constants::DESKTOPQT))
                    return false;

                bool hasViewer = false; // Initialization needed for dumb compilers.
                QtSupport::QtVersionNumber minVersion;
                switch (m_defaultImport) {
                case QmlProject::UnknownImport:
                    minVersion = QtSupport::QtVersionNumber(4, 7, 0);
                    hasViewer = !version->qmlviewerCommand().isEmpty() || !version->qmlsceneCommand().isEmpty();
                    break;
                case QmlProject::QtQuick1Import:
                    minVersion = QtSupport::QtVersionNumber(4, 7, 1);
                    hasViewer = !version->qmlviewerCommand().isEmpty();
                    break;
                case QmlProject::QtQuick2Import:
                    minVersion = QtSupport::QtVersionNumber(5, 0, 0);
                    hasViewer = !version->qmlsceneCommand().isEmpty();
                    break;
                }

                return version->qtVersion() >= minVersion && hasViewer;
            }));

        if (!kits.isEmpty()) {
            Kit *kit = 0;
            if (kits.contains(KitManager::defaultKit()))
                kit = KitManager::defaultKit();
            else
                kit = kits.first();
            addTarget(createTarget(kit));
        }
    }

    // addedTarget calls updateEnabled on the runconfigurations
    // which needs to happen after refresh
    foreach (Target *t, targets())
        addedTarget(t);

    connect(this, &ProjectExplorer::Project::addedTarget, this, &QmlProject::addedTarget);

    connect(this, &ProjectExplorer::Project::activeTargetChanged,
            this, &QmlProject::onActiveTargetChanged);

    onActiveTargetChanged(activeTarget());

    return true;
}

} // namespace QmlProjectManager

