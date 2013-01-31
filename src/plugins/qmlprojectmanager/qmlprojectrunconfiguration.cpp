/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprojectrunconfiguration.h"
#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectrunconfigurationwidget.h"
#include <coreplugin/mimedatabase.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>

#ifdef Q_OS_WIN
#include <utils/winutils.h>
#endif

using Core::EditorManager;
using Core::ICore;
using Core::IEditor;

using namespace QmlProjectManager::Internal;

namespace QmlProjectManager {

const char * const M_CURRENT_FILE = "CurrentFile";

QmlProjectRunConfiguration::QmlProjectRunConfiguration(ProjectExplorer::Target *parent,
                                                       Core::Id id) :
    ProjectExplorer::RunConfiguration(parent, id),
    m_scriptFile(QLatin1String(M_CURRENT_FILE)),
    m_isEnabled(false)
{
    ctor();
}

QmlProjectRunConfiguration::QmlProjectRunConfiguration(ProjectExplorer::Target *parent,
                                                       QmlProjectRunConfiguration *source) :
    ProjectExplorer::RunConfiguration(parent, source),
    m_scriptFile(source->m_scriptFile),
    m_qmlViewerArgs(source->m_qmlViewerArgs),
    m_isEnabled(source->m_isEnabled),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges)
{
    ctor();
}

bool QmlProjectRunConfiguration::isEnabled() const
{
    return m_isEnabled;
}

QString QmlProjectRunConfiguration::disabledReason() const
{
    if (!m_isEnabled)
        return tr("No qmlviewer or qmlscene found.");
    return QString();
}

void QmlProjectRunConfiguration::ctor()
{
    // reset default settings in constructor
    debuggerAspect()->setUseCppDebugger(false);
    debuggerAspect()->setUseQmlDebugger(true);

    EditorManager *em = Core::EditorManager::instance();
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(changeCurrentFile(Core::IEditor*)));

    connect(target(), SIGNAL(kitChanged()),
            this, SLOT(updateEnabled()));

    if (id() == Constants::QML_SCENE_RC_ID)
        setDisplayName(tr("QML Scene", "QMLRunConfiguration display name."));
    else
        setDisplayName(tr("QML Viewer", "QMLRunConfiguration display name."));
}

QmlProjectRunConfiguration::~QmlProjectRunConfiguration()
{
}

QString QmlProjectRunConfiguration::viewerPath() const
{
    QtSupport::BaseQtVersion *version = qtVersion();
    if (!version)
        return QString();

    if (id() == Constants::QML_SCENE_RC_ID)
        return version->qmlsceneCommand();
    else
        return version->qmlviewerCommand();
}

QString QmlProjectRunConfiguration::observerPath() const
{
    QtSupport::BaseQtVersion *version = qtVersion();
    if (!version) {
        return QString();
    } else {
        if (id() == Constants::QML_SCENE_RC_ID)
            return version->qmlsceneCommand();
        if (!version->needsQmlDebuggingLibrary())
            return version->qmlviewerCommand();
        return version->qmlObserverTool();
    }
}

QString QmlProjectRunConfiguration::viewerArguments() const
{
    // arguments in .user file
    QString args = m_qmlViewerArgs;

    // arguments from .qmlproject file
    QmlProject *project = static_cast<QmlProject *>(target()->project());
    foreach (const QString &importPath, project->customImportPaths()) {
        Utils::QtcProcess::addArg(&args, QLatin1String("-I"));
        Utils::QtcProcess::addArg(&args, importPath);
    }

    QString s = mainScript();
    if (!s.isEmpty()) {
        s = canonicalCapsPath(s);
        Utils::QtcProcess::addArg(&args, s);
    }
    return args;
}

QString QmlProjectRunConfiguration::workingDirectory() const
{
    QFileInfo projectFile(target()->project()->document()->fileName());
    return canonicalCapsPath(projectFile.absolutePath());
}

/* QtDeclarative checks explicitly that the capitalization for any URL / path
   is exactly like the capitalization on disk.*/
QString QmlProjectRunConfiguration::canonicalCapsPath(const QString &fileName)
{
    QString canonicalPath = QFileInfo(fileName).canonicalFilePath();

#if defined(Q_OS_WIN)
    canonicalPath = Utils::normalizePathName(canonicalPath);
#endif

    return canonicalPath;
}


QtSupport::BaseQtVersion *QmlProjectRunConfiguration::qtVersion() const
{
    return QtSupport::QtKitInformation::qtVersion(target()->kit());
}

QWidget *QmlProjectRunConfiguration::createConfigurationWidget()
{
    QTC_ASSERT(m_configurationWidget.isNull(), return m_configurationWidget.data());
    m_configurationWidget = new QmlProjectRunConfigurationWidget(this);
    return m_configurationWidget.data();
}

Utils::OutputFormatter *QmlProjectRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
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
            return project->projectDir().absoluteFilePath(pathInProject);
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
                = target()->project()->projectDirectory() + QLatin1Char('/') + m_scriptFile;
    }
    updateEnabled();
    if (m_configurationWidget)
        m_configurationWidget.data()->updateFileComboBox();
}

Utils::Environment QmlProjectRunConfiguration::environment() const
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

ProjectExplorer::Abi QmlProjectRunConfiguration::abi() const
{
    ProjectExplorer::Abi hostAbi = ProjectExplorer::Abi::hostAbi();
    return ProjectExplorer::Abi(hostAbi.architecture(), hostAbi.os(), hostAbi.osFlavor(),
                                ProjectExplorer::Abi::RuntimeQmlFormat, hostAbi.wordWidth());
}

QVariantMap QmlProjectRunConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::RunConfiguration::toMap());

    map.insert(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY), m_qmlViewerArgs);
    map.insert(QLatin1String(Constants::QML_MAINSCRIPT_KEY),  m_scriptFile);
    map.insert(QLatin1String(Constants::USER_ENVIRONMENT_CHANGES_KEY),
               Utils::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    return map;
}

bool QmlProjectRunConfiguration::fromMap(const QVariantMap &map)
{
    m_qmlViewerArgs = map.value(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY)).toString();
    m_scriptFile = map.value(QLatin1String(Constants::QML_MAINSCRIPT_KEY), QLatin1String(M_CURRENT_FILE)).toString();
    m_userEnvironmentChanges = Utils::EnvironmentItem::fromStringList(
                map.value(QLatin1String(Constants::USER_ENVIRONMENT_CHANGES_KEY)).toStringList());

    if (m_scriptFile == QLatin1String(M_CURRENT_FILE))
        setScriptSource(FileInEditor);
    else if (m_scriptFile.isEmpty())
        setScriptSource(FileInProjectFile);
    else
        setScriptSource(FileInSettings, m_scriptFile);

    return RunConfiguration::fromMap(map);
}

void QmlProjectRunConfiguration::changeCurrentFile(Core::IEditor *editor)
{
    if (editor)
        m_currentFileFilename = editor->document()->fileName();
    updateEnabled();
}

void QmlProjectRunConfiguration::updateEnabled()
{
    bool qmlFileFound = false;
    if (mainScriptSource() == FileInEditor) {
        Core::IEditor *editor = Core::EditorManager::currentEditor();
        Core::MimeDatabase *db = ICore::mimeDatabase();
        if (editor) {
            m_currentFileFilename = editor->document()->fileName();
            if (db->findByFile(mainScript()).type() == QLatin1String("application/x-qml"))
                qmlFileFound = true;
        }
        if (!editor
                || db->findByFile(mainScript()).type() == QLatin1String("application/x-qmlproject")) {
            // find a qml file with lowercase filename. This is slow, but only done
            // in initialization/other border cases.
            foreach (const QString &filename, target()->project()->files(ProjectExplorer::Project::AllFiles)) {
                const QFileInfo fi(filename);

                if (!filename.isEmpty() && fi.baseName()[0].isLower()
                        && db->findByFile(fi).type() == QLatin1String("application/x-qml"))
                {
                    m_currentFileFilename = filename;
                    qmlFileFound = true;
                    break;
                }

            }
        }
    } else { // use default one
        qmlFileFound = !mainScript().isEmpty();
    }

    bool newValue = (QFileInfo(viewerPath()).exists()
                     || QFileInfo(observerPath()).exists()) && qmlFileFound;


    // Always emit change signal to force reevaluation of run/debug buttons
    m_isEnabled = newValue;
    emit enabledChanged();
}

bool QmlProjectRunConfiguration::isValidVersion(QtSupport::BaseQtVersion *version)
{
    if (version
            && (version->type() == QLatin1String(QtSupport::Constants::DESKTOPQT)
                || version->type() == QLatin1String(QtSupport::Constants::SIMULATORQT))
            && !version->qmlviewerCommand().isEmpty()) {
        return true;
    }
    return false;
}

Utils::Environment QmlProjectRunConfiguration::baseEnvironment() const
{
    Utils::Environment env;
    if (qtVersion())
        env = qtVersion()->qmlToolsEnvironment();
    return env;
}

void QmlProjectRunConfiguration::setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges != diff) {
        m_userEnvironmentChanges = diff;
        if (m_configurationWidget)
            m_configurationWidget.data()->userEnvironmentChangesChanged();
    }
}

QList<Utils::EnvironmentItem> QmlProjectRunConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

} // namespace QmlProjectManager
