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

#include "qmlprojectrunconfiguration.h"
#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectrunconfigurationwidget.h"
#include "qmlprojectenvironmentaspect.h"
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/desktopqtversion.h>

#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>
#include <qmljstools/qmljstoolsconstants.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtSupport;

namespace QmlProjectManager {

const char M_CURRENT_FILE[] = "CurrentFile";

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    addExtraAspect(new QmlProjectEnvironmentAspect(this));
    setOutputFormatter<QtSupport::QtOutputFormatter>();

    // reset default settings in constructor
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &QmlProjectRunConfiguration::changeCurrentFile);
    connect(EditorManager::instance(), &EditorManager::currentDocumentStateChanged,
            this, [this] { changeCurrentFile(); });

    connect(target, &Target::kitChanged,
            this, &QmlProjectRunConfiguration::updateEnabledState);
    m_scriptFile = M_CURRENT_FILE;

    setDisplayName(tr("QML Scene", "QMLRunConfiguration display name."));
    updateEnabledState();
}

Runnable QmlProjectRunConfiguration::runnable() const
{
    Runnable r;
    r.executable = executable();
    r.commandLineArguments = commandLineArguments();
    r.environment = extraAspect<QmlProjectEnvironmentAspect>()->environment();
    r.workingDirectory = static_cast<QmlProject *>(project())->targetDirectory(target()).toString();
    return r;
}

QString QmlProjectRunConfiguration::disabledReason() const
{
    if (mainScript().isEmpty())
        return tr("No script file to execute.");
    if (DeviceTypeKitInformation::deviceTypeId(target()->kit())
            == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE
            && !QFileInfo::exists(executable())) {
        return tr("No qmlscene found.");
    }
    if (executable().isEmpty())
        return tr("No qmlscene binary specified for target device.");
    return RunConfiguration::disabledReason();
}

QString QmlProjectRunConfiguration::executable() const
{
    BaseQtVersion *version = QtKitInformation::qtVersion(target()->kit());
    if (!version) // No Qt version in Kit. Don't try to run qmlscene.
        return QString();

    const Id deviceType = DeviceTypeKitInformation::deviceTypeId(target()->kit());
    if (deviceType == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        // If not given explicitly by Qt Version, try to pick it from $PATH.
        return version->type() == QtSupport::Constants::DESKTOPQT
                ? static_cast<QtSupport::DesktopQtVersion *>(version)->qmlsceneCommand()
                : QString("qmlscene");
    }

    IDevice::ConstPtr dev = DeviceKitInformation::device(target()->kit());
    if (dev.isNull()) // No device set. We don't know where to run qmlscene.
        return QString();

    const QString qmlscene = dev->qmlsceneCommand();
    // If not given explicitly by device, try to pick it from $PATH.
    return qmlscene.isEmpty() ? QString("qmlscene") : qmlscene;
}

QString QmlProjectRunConfiguration::commandLineArguments() const
{
    // arguments in .user file
    QString args = m_qmlViewerArgs;
    const Target *currentTarget = target();
    const IDevice::ConstPtr device = DeviceKitInformation::device(currentTarget->kit());
    const Utils::OsType osType = device ? device->osType() : Utils::HostOsInfo::hostOs();

    // arguments from .qmlproject file
    const QmlProject *project = static_cast<QmlProject *>(currentTarget->project());
    foreach (const QString &importPath,
             QmlProject::makeAbsolute(project->targetDirectory(currentTarget), project->customImportPaths())) {
        Utils::QtcProcess::addArg(&args, QLatin1String("-I"), osType);
        Utils::QtcProcess::addArg(&args, importPath, osType);
    }

    const QString main = project->targetFile(Utils::FileName::fromString(mainScript()),
                                             currentTarget).toString();
    if (!main.isEmpty())
        Utils::QtcProcess::addArg(&args, main, osType);
    return args;
}

QWidget *QmlProjectRunConfiguration::createConfigurationWidget()
{
    return wrapWidget(new Internal::QmlProjectRunConfigurationWidget(this));
}

QmlProjectRunConfiguration::MainScriptSource QmlProjectRunConfiguration::mainScriptSource() const
{
    QmlProject *project = static_cast<QmlProject *>(target()->project());
    if (!project->mainFile().isEmpty())
        return FileInProjectFile;
    if (!m_mainScriptFilename.isEmpty())
        return FileInSettings;
    return FileInEditor;
}

/**
  Returns absolute path to main script file.
  */
QString QmlProjectRunConfiguration::mainScript() const
{
    QmlProject *project = qobject_cast<QmlProject *>(target()->project());
    if (!project)
        return m_currentFileFilename;
    if (!project->mainFile().isEmpty()) {
        const QString pathInProject = project->mainFile();
        if (QFileInfo(pathInProject).isAbsolute())
            return pathInProject;
        else
            return QDir(project->canonicalProjectDir().toString()).absoluteFilePath(pathInProject);
    }

    if (!m_mainScriptFilename.isEmpty())
        return m_mainScriptFilename;

    return m_currentFileFilename;
}

void QmlProjectRunConfiguration::setScriptSource(MainScriptSource source,
                                                 const QString &settingsPath)
{
    if (source == FileInEditor) {
        m_scriptFile = QLatin1String(M_CURRENT_FILE);
        m_mainScriptFilename.clear();
    } else if (source == FileInProjectFile) {
        m_scriptFile.clear();
        m_mainScriptFilename.clear();
    } else { // FileInSettings
        m_scriptFile = settingsPath;
        m_mainScriptFilename
                = target()->project()->projectDirectory().toString() + QLatin1Char('/') + m_scriptFile;
    }
    updateEnabledState();

    emit scriptSourceChanged();
}

Abi QmlProjectRunConfiguration::abi() const
{
    Abi hostAbi = Abi::hostAbi();
    return Abi(hostAbi.architecture(), hostAbi.os(), hostAbi.osFlavor(),
               Abi::RuntimeQmlFormat, hostAbi.wordWidth());
}

QVariantMap QmlProjectRunConfiguration::toMap() const
{
    QVariantMap map(RunConfiguration::toMap());

    map.insert(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY), m_qmlViewerArgs);
    map.insert(QLatin1String(Constants::QML_MAINSCRIPT_KEY),  m_scriptFile);
    return map;
}

bool QmlProjectRunConfiguration::fromMap(const QVariantMap &map)
{
    m_qmlViewerArgs = map.value(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY)).toString();
    m_scriptFile = map.value(QLatin1String(Constants::QML_MAINSCRIPT_KEY), QLatin1String(M_CURRENT_FILE)).toString();

    if (m_scriptFile == QLatin1String(M_CURRENT_FILE))
        setScriptSource(FileInEditor);
    else if (m_scriptFile.isEmpty())
        setScriptSource(FileInProjectFile);
    else
        setScriptSource(FileInSettings, m_scriptFile);

    return RunConfiguration::fromMap(map);
}

void QmlProjectRunConfiguration::changeCurrentFile(IEditor *editor)
{
    if (!editor)
        editor = EditorManager::currentEditor();

    if (editor)
        m_currentFileFilename = editor->document()->filePath().toString();
    updateEnabledState();
}

void QmlProjectRunConfiguration::updateEnabledState()
{
    bool qmlFileFound = false;
    if (mainScriptSource() == FileInEditor) {
        IDocument *document = EditorManager::currentDocument();
        Utils::MimeType mainScriptMimeType = Utils::mimeTypeForFile(mainScript());
        if (document) {
            m_currentFileFilename = document->filePath().toString();
            if (mainScriptMimeType.matchesName(
                        QLatin1String(ProjectExplorer::Constants::QML_MIMETYPE))
                    || mainScriptMimeType.matchesName(
                        QLatin1String(ProjectExplorer::Constants::QMLUI_MIMETYPE))) {
                qmlFileFound = true;
            }
        }
        if (!document
                || mainScriptMimeType.matchesName(QLatin1String(QmlJSTools::Constants::QMLPROJECT_MIMETYPE))) {
            // find a qml file with lowercase filename. This is slow, but only done
            // in initialization/other border cases.
            foreach (const Utils::FileName &filename, target()->project()->files(Project::AllFiles)) {
                const QFileInfo fi = filename.toFileInfo();

                if (!filename.isEmpty() && fi.baseName()[0].isLower()) {
                    Utils::MimeType type = Utils::mimeTypeForFile(fi);
                    if (type.matchesName(QLatin1String(ProjectExplorer::Constants::QML_MIMETYPE))
                            || type.matchesName(
                                QLatin1String(ProjectExplorer::Constants::QMLUI_MIMETYPE))) {
                        m_currentFileFilename = filename.toString();
                        qmlFileFound = true;
                        break;
                    }
                }

            }
        }
    } else { // use default one
        qmlFileFound = !mainScript().isEmpty();
    }

    if (!qmlFileFound) {
        setEnabled(false);
    } else {
        const QString exe = executable();
        if (exe.isEmpty()) {
            setEnabled(false);
        } else {
            RunConfiguration::updateEnabledState();
        }
    }
}

namespace Internal {

QmlProjectRunConfigurationFactory::QmlProjectRunConfigurationFactory()
    : FixedRunConfigurationFactory(QmlProjectRunConfiguration::tr("QML Scene"), false)
{
    registerRunConfiguration<QmlProjectRunConfiguration>
            ("QmlProjectManager.QmlRunConfiguration.QmlScene");
    addSupportedProjectType(QmlProjectManager::Constants::QML_PROJECT_ID);

    addRunWorkerFactory<SimpleTargetRunner>(ProjectExplorer::Constants::NORMAL_RUN_MODE);
}

} // namespace Internal
} // namespace QmlProjectManager
