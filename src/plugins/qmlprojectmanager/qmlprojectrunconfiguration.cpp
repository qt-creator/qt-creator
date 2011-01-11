/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmlprojectrunconfiguration.h"
#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectrunconfigurationwidget.h"
#include <coreplugin/mimedatabase.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <qt4projectmanager/qtversionmanager.h>
#include <qt4projectmanager/qtoutputformatter.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#ifdef Q_OS_WIN32
#include <utils/winutils.h>
#endif

using Core::EditorManager;
using Core::ICore;
using Core::IEditor;
using Qt4ProjectManager::QtVersionManager;

using namespace QmlProjectManager::Internal;

namespace QmlProjectManager {

const char * const M_CURRENT_FILE = "CurrentFile";

QmlProjectRunConfiguration::QmlProjectRunConfiguration(QmlProjectTarget *parent) :
    ProjectExplorer::RunConfiguration(parent, QLatin1String(Constants::QML_RC_ID)),
    m_qtVersionId(-1),
    m_projectTarget(parent),
    m_usingCurrentFile(true),
    m_isEnabled(false)
{
    ctor();
    updateQtVersions();
}

QmlProjectRunConfiguration::QmlProjectRunConfiguration(QmlProjectTarget *parent,
                                                       QmlProjectRunConfiguration *source) :
    ProjectExplorer::RunConfiguration(parent, source),
    m_qtVersionId(source->m_qtVersionId),
    m_scriptFile(source->m_scriptFile),
    m_qmlViewerArgs(source->m_qmlViewerArgs),
    m_projectTarget(parent),
    m_usingCurrentFile(source->m_usingCurrentFile),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges)
{
    ctor();
    updateQtVersions();
}

bool QmlProjectRunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration *bc) const
{
    Q_UNUSED(bc);

    return m_isEnabled;
}

void QmlProjectRunConfiguration::ctor()
{
    // reset default settings in constructor
    setUseCppDebugger(false);
    setUseQmlDebugger(true);

    EditorManager *em = Core::EditorManager::instance();
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(changeCurrentFile(Core::IEditor*)));

    QtVersionManager *qtVersions = QtVersionManager::instance();
    connect(qtVersions, SIGNAL(qtVersionsChanged(QList<int>)), this, SLOT(updateQtVersions()));

    setDisplayName(tr("QML Viewer", "QMLRunConfiguration display name."));
}

QmlProjectRunConfiguration::~QmlProjectRunConfiguration()
{
}

QmlProjectTarget *QmlProjectRunConfiguration::qmlTarget() const
{
    return static_cast<QmlProjectTarget *>(target());
}

QString QmlProjectRunConfiguration::viewerPath() const
{
    Qt4ProjectManager::QtVersion *version = qtVersion();
    if (!version) {
        return QString();
    } else {
        return version->qmlviewerCommand();
    }
}

QString QmlProjectRunConfiguration::observerPath() const
{
    Qt4ProjectManager::QtVersion *version = qtVersion();
    if (!version) {
        return QString();
    } else {
        return version->qmlObserverTool();
    }
}

QString QmlProjectRunConfiguration::viewerArguments() const
{
    // arguments in .user file
    QString args = m_qmlViewerArgs;

    // arguments from .qmlproject file
    foreach (const QString &importPath, qmlTarget()->qmlProject()->importPaths()) {
        Utils::QtcProcess::addArg(&args, "-I");
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
    QFileInfo projectFile(qmlTarget()->qmlProject()->file()->fileName());
    return canonicalCapsPath(projectFile.absolutePath());
}

int QmlProjectRunConfiguration::qtVersionId() const
{
    return m_qtVersionId;
}

void QmlProjectRunConfiguration::setQtVersionId(int id)
{
    if (m_qtVersionId == id)
        return;

    m_qtVersionId = id;
    qmlTarget()->qmlProject()->refresh(QmlProject::Configuration);
    if (m_configurationWidget)
        m_configurationWidget.data()->updateQtVersionComboBox();
}

/* QtDeclarative checks explicitly that the capitalization for any URL / path
   is exactly like the capitalization on disk. This method is uses the same
   native Windows API's to get the exact canonical path. */
QString QmlProjectRunConfiguration::canonicalCapsPath(const QString &fileName)
{
    QString canonicalPath = QFileInfo(fileName).canonicalFilePath();

#if defined(Q_OS_WIN32)
    QString error;
    // don't know whether the shortpath step is really needed,
    // but we do this in QtDeclarative too.
    QString path = Utils::getShortPathName(canonicalPath, &error);
    if (!path.isEmpty())
        path = Utils::getLongPathName(canonicalPath, &error);
    if (!path.isEmpty())
        canonicalPath = path;
#endif

    return canonicalPath;
}


Qt4ProjectManager::QtVersion *QmlProjectRunConfiguration::qtVersion() const
{
    if (m_qtVersionId == -1)
        return 0;

    QtVersionManager *versionManager = QtVersionManager::instance();
    Qt4ProjectManager::QtVersion *version = versionManager->version(m_qtVersionId);
    QTC_ASSERT(version, return 0);

    return version;
}

QWidget *QmlProjectRunConfiguration::createConfigurationWidget()
{
    QTC_ASSERT(m_configurationWidget.isNull(), return m_configurationWidget.data());
    m_configurationWidget = new QmlProjectRunConfigurationWidget(this);
    return m_configurationWidget.data();
}

ProjectExplorer::OutputFormatter *QmlProjectRunConfiguration::createOutputFormatter() const
{
    return new Qt4ProjectManager::QtOutputFormatter(qmlTarget()->qmlProject());
}

QmlProjectRunConfiguration::MainScriptSource QmlProjectRunConfiguration::mainScriptSource() const
{
    if (m_usingCurrentFile) {
        return FileInEditor;
    }
    if (!m_mainScriptFilename.isEmpty()) {
        return FileInSettings;
    }
    return FileInProjectFile;
}

/**
  Returns absolute path to main script file.
  */
QString QmlProjectRunConfiguration::mainScript() const
{
    if (m_usingCurrentFile) {
        return m_currentFileFilename;
    }

    if (!m_mainScriptFilename.isEmpty()) {
        return m_mainScriptFilename;
    }

    QString path = qmlTarget()->qmlProject()->mainFile();
    if (QFileInfo(path).isAbsolute()) {
        return path;
    } else {
        return qmlTarget()->qmlProject()->projectDir().absoluteFilePath(path);
    }
}

void QmlProjectRunConfiguration::setScriptSource(MainScriptSource source,
                                                 const QString &settingsPath)
{
    if (source == FileInEditor) {
        m_scriptFile = M_CURRENT_FILE;
        m_mainScriptFilename.clear();
        m_usingCurrentFile = true;
    } else if (source == FileInProjectFile) {
        m_scriptFile.clear();
        m_mainScriptFilename.clear();
        m_usingCurrentFile = false;
    } else { // FileInSettings
        m_scriptFile = settingsPath;
        m_mainScriptFilename
                = qmlTarget()->qmlProject()->projectDir().absoluteFilePath(m_scriptFile);
        m_usingCurrentFile = false;
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

QVariantMap QmlProjectRunConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::RunConfiguration::toMap());

    map.insert(QLatin1String(Constants::QML_VIEWER_QT_KEY), m_qtVersionId);
    map.insert(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY), m_qmlViewerArgs);
    map.insert(QLatin1String(Constants::QML_MAINSCRIPT_KEY),  m_scriptFile);
    map.insert(QLatin1String(Constants::USER_ENVIRONMENT_CHANGES_KEY),
               Utils::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    return map;
}

bool QmlProjectRunConfiguration::fromMap(const QVariantMap &map)
{
    setQtVersionId(map.value(QLatin1String(Constants::QML_VIEWER_QT_KEY), -1).toInt());
    m_qmlViewerArgs = map.value(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY)).toString();
    m_scriptFile = map.value(QLatin1String(Constants::QML_MAINSCRIPT_KEY), M_CURRENT_FILE).toString();
    m_userEnvironmentChanges = Utils::EnvironmentItem::fromStringList(
                map.value(QLatin1String(Constants::USER_ENVIRONMENT_CHANGES_KEY)).toStringList());


    updateQtVersions();
    if (m_scriptFile == M_CURRENT_FILE) {
        setScriptSource(FileInEditor);
    } else if (m_scriptFile.isEmpty()) {
        setScriptSource(FileInProjectFile);
    } else {
        setScriptSource(FileInSettings, m_scriptFile);
    }

    return RunConfiguration::fromMap(map);
}

void QmlProjectRunConfiguration::changeCurrentFile(Core::IEditor * /*editor*/)
{
    updateEnabled();
}

void QmlProjectRunConfiguration::updateEnabled()
{
    bool qmlFileFound = false;
    if (m_usingCurrentFile) {
        Core::IEditor *editor = Core::EditorManager::instance()->currentEditor();
        Core::MimeDatabase *db = ICore::instance()->mimeDatabase();
        if (editor) {
            m_currentFileFilename = editor->file()->fileName();
            if (db->findByFile(mainScript()).type() == QLatin1String("application/x-qml"))
                qmlFileFound = true;
        }
        if (!editor
                || db->findByFile(mainScript()).type() == QLatin1String("application/x-qmlproject")) {
            // find a qml file with lowercase filename. This is slow, but only done
            // in initialization/other border cases.
            foreach(const QString &filename, m_projectTarget->qmlProject()->files()) {
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
    emit isEnabledChanged(m_isEnabled);
}

void QmlProjectRunConfiguration::updateQtVersions()
{
    QtVersionManager *qtVersions = QtVersionManager::instance();

    //
    // update m_qtVersionId
    //
    if (!qtVersions->isValidId(m_qtVersionId)
            || !isValidVersion(qtVersions->version(m_qtVersionId))) {
        int newVersionId = -1;
        // take first one you find
        foreach (Qt4ProjectManager::QtVersion *version, qtVersions->validVersions()) {
            if (isValidVersion(version)) {
                newVersionId = version->uniqueId();
                break;
            }
        }
        setQtVersionId(newVersionId);
    }

    updateEnabled();
}

bool QmlProjectRunConfiguration::isValidVersion(Qt4ProjectManager::QtVersion *version)
{
    if (version
            && (version->supportsTargetId(Qt4ProjectManager::Constants::DESKTOP_TARGET_ID)
                || version->supportsTargetId(Qt4ProjectManager::Constants::QT_SIMULATOR_TARGET_ID))
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
