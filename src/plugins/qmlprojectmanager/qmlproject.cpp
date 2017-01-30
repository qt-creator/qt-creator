/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

QmlProject::QmlProject(Internal::Manager *manager, const Utils::FileName &fileName) :
    m_defaultImport(UnknownImport)
{
    setId("QmlProjectManager.QmlProject");
    setProjectManager(manager);
    setDocument(new Internal::QmlProjectFile(this, fileName));
    DocumentManager::addDocument(document(), true);
    setRootProjectNode(new Internal::QmlProjectNode(this));

    setProjectContext(Context(QmlProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguages(Context(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID));

    m_projectName = projectFilePath().toFileInfo().completeBaseName();

    projectManager()->registerProject(this);
}

QmlProject::~QmlProject()
{
    projectManager()->unregisterProject(this);

    delete m_projectItem.data();
}

void QmlProject::addedTarget(Target *target)
{
    connect(target, &Target::addedRunConfiguration,
            this, &QmlProject::addedRunConfiguration);
    foreach (RunConfiguration *rc, target->runConfigurations())
        addedRunConfiguration(rc);
}

void QmlProject::onActiveTargetChanged(Target *target)
{
    if (m_activeTarget)
        disconnect(m_activeTarget, &Target::kitChanged, this, &QmlProject::onKitChanged);
    m_activeTarget = target;
    if (m_activeTarget)
        connect(target, &Target::kitChanged, this, &QmlProject::onKitChanged);

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
{ return projectFilePath(); }

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
              m_projectItem = QmlProjectFileFormat::parseProjectFile(projectFilePath(), &errorMessage);
              if (m_projectItem) {
                  connect(m_projectItem.data(), &QmlProjectItem::qmlFilesChanged,
                          this, &QmlProject::refreshFiles);

              } else {
                  MessageManager::write(tr("Error while loading project file %1.")
                                        .arg(projectFilePath().toUserOutput()),
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
                                          .arg(projectFilePath().toUserOutput()));
                    MessageManager::write(errorMessage);
                } else {
                    m_defaultImport = detectImport(QString::fromUtf8(reader.data()));
                }
            }
        }
        rootProjectNode()->refresh();
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
        rootProjectNode()->refresh();

    if (!modelManager())
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager()->defaultProjectInfoForProject(this);
    foreach (const QString &searchPath, customImportPaths())
        projectInfo.importPaths.maybeInsert(Utils::FileName::fromString(searchPath),
                                            QmlJS::Dialect::Qml);

    modelManager()->updateProjectInfo(projectInfo, this);

    emit parsingFinished();
}

QStringList QmlProject::convertToAbsoluteFiles(const QStringList &paths) const
{
    const QDir projectDir(projectDirectory().toString());
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

Internal::Manager *QmlProject::projectManager() const
{
    return static_cast<Internal::Manager *>(Project::projectManager());
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

Internal::QmlProjectNode *QmlProject::rootProjectNode() const
{
    return static_cast<Internal::QmlProjectNode *>(Project::rootProjectNode());
}

QStringList QmlProject::files(FilesMode) const
{
    return files();
}

Project::RestoreResult QmlProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    // refresh first - project information is used e.g. to decide the default RC's
    refresh(Everything);

    if (!activeTarget()) {
        // find a kit that matches prerequisites (prefer default one)
        QList<Kit*> kits = KitManager::kits(
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

    return RestoreResult::Ok;
}

} // namespace QmlProjectManager

