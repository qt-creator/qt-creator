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

#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmlProjectManager::Internal;

namespace QmlProjectManager {

const char M_CURRENT_FILE[] = "CurrentFile";

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Target *parent, Id id) :
    LocalApplicationRunConfiguration(parent, id),
    m_scriptFile(QLatin1String(M_CURRENT_FILE)),
    m_isEnabled(false)
{
    addExtraAspect(new QmlProjectEnvironmentAspect(this));

    ctor();
}

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Target *parent,
                                                       QmlProjectRunConfiguration *source) :
    LocalApplicationRunConfiguration(parent, source),
    m_currentFileFilename(source->m_currentFileFilename),
    m_mainScriptFilename(source->m_mainScriptFilename),
    m_scriptFile(source->m_scriptFile),
    m_qmlViewerArgs(source->m_qmlViewerArgs),
    m_isEnabled(source->m_isEnabled)
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
    connect(EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(changeCurrentFile(Core::IEditor*)));
    connect(EditorManager::instance(), SIGNAL(currentDocumentStateChanged()),
            this, SLOT(changeCurrentFile()));

    connect(target(), SIGNAL(kitChanged()),
            this, SLOT(updateEnabled()));

    if (id() == Constants::QML_SCENE_RC_ID)
        setDisplayName(tr("QML Scene", "QMLRunConfiguration display name."));
    else
        setDisplayName(tr("QML Viewer", "QMLRunConfiguration display name."));
    updateEnabled();
}

QString QmlProjectRunConfiguration::executable() const
{
    QtSupport::BaseQtVersion *version = qtVersion();
    if (!version)
        return QString();

    if (id() == Constants::QML_SCENE_RC_ID)
        return version->qmlsceneCommand();
    return version->qmlviewerCommand();
}

ApplicationLauncher::Mode QmlProjectRunConfiguration::runMode() const
{
    return ApplicationLauncher::Gui;
}

QString QmlProjectRunConfiguration::commandLineArguments() const
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
    return canonicalCapsPath(target()->project()->projectFilePath().toFileInfo().absolutePath());
}

/* QtDeclarative checks explicitly that the capitalization for any URL / path
   is exactly like the capitalization on disk.*/
QString QmlProjectRunConfiguration::canonicalCapsPath(const QString &fileName)
{
    return Utils::FileUtils::normalizePathName(QFileInfo(fileName).canonicalFilePath());
}


QtSupport::BaseQtVersion *QmlProjectRunConfiguration::qtVersion() const
{
    return QtSupport::QtKitInformation::qtVersion(target()->kit());
}

QWidget *QmlProjectRunConfiguration::createConfigurationWidget()
{
    return new QmlProjectRunConfigurationWidget(this);
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
                = target()->project()->projectDirectory().toString() + QLatin1Char('/') + m_scriptFile;
    }
    updateEnabled();

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
    updateEnabled();
}

void QmlProjectRunConfiguration::updateEnabled()
{
    bool qmlFileFound = false;
    Utils::MimeDatabase mdb;
    if (mainScriptSource() == FileInEditor) {
        IDocument *document = EditorManager::currentDocument();
        if (document) {
            m_currentFileFilename = document->filePath().toString();
            if (mdb.mimeTypeForFile(mainScript()).matchesName(QLatin1String("text/x-qml")))
                qmlFileFound = true;
        }
        if (!document
                || mdb.mimeTypeForFile(mainScript()).matchesName(QLatin1String("application/x-qmlproject"))) {
            // find a qml file with lowercase filename. This is slow, but only done
            // in initialization/other border cases.
            foreach (const QString &filename, target()->project()->files(Project::AllFiles)) {
                const QFileInfo fi(filename);

                if (!filename.isEmpty() && fi.baseName()[0].isLower()
                        && mdb.mimeTypeForFile(fi).matchesName(QLatin1String("text/x-qml")))
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

    bool newValue = QFileInfo::exists(executable()) && qmlFileFound;

    // Always emit change signal to force reevaluation of run/debug buttons
    m_isEnabled = newValue;
    emit enabledChanged();
}

bool QmlProjectRunConfiguration::isValidVersion(QtSupport::BaseQtVersion *version)
{
    if (version
            && version->type() == QLatin1String(QtSupport::Constants::DESKTOPQT)
            && !version->qmlviewerCommand().isEmpty()) {
        return true;
    }
    return false;
}

} // namespace QmlProjectManager
