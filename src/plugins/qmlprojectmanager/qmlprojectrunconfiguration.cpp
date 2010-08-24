/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlprojecttarget.h"
#include "projectexplorer/projectexplorer.h"

#include <coreplugin/mimedatabase.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <utils/synchronousprocess.h>
#include <utils/pathchooser.h>
#include <qt4projectmanager/qtversionmanager.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <QFormLayout>
#include <QComboBox>
#include <QCoreApplication>
#include <QLineEdit>
#include <QSpinBox>
#include <QStringListModel>
#include <QDebug>

namespace QmlProjectManager {


QmlProjectRunConfigurationDebugData::QmlProjectRunConfigurationDebugData() :
        serverAddress("127.0.0.1"), serverPort(Constants::QML_DEFAULT_DEBUG_SERVER_PORT)
{
}

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Internal::QmlProjectTarget *parent) :
    ProjectExplorer::RunConfiguration(parent, QLatin1String(Constants::QML_RC_ID)),
    m_fileListModel(new QStringListModel(this)),
    m_projectTarget(parent),
    m_usingCurrentFile(true),
    m_isEnabled(false)
{
    ctor();
}

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Internal::QmlProjectTarget *parent, QmlProjectRunConfiguration *source) :
    ProjectExplorer::RunConfiguration(parent, source),
    m_qmlViewerCustomPath(source->m_qmlViewerCustomPath),
    m_qmlViewerArgs(source->m_qmlViewerArgs),
    m_fileListModel(new QStringListModel(this)),
    m_projectTarget(parent)
{
    ctor();
    m_debugData.serverAddress = source->m_debugData.serverAddress;
    m_debugData.serverPort = source->m_debugData.serverPort;
    setMainScript(source->m_scriptFile);
}

bool QmlProjectRunConfiguration::isEnabled(ProjectExplorer::BuildConfiguration *bc) const
{
    Q_UNUSED(bc);

    return m_isEnabled;
}

void QmlProjectRunConfiguration::ctor()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(changeCurrentFile(Core::IEditor*)));

    Qt4ProjectManager::QtVersionManager *qtVersions = Qt4ProjectManager::QtVersionManager::instance();
    connect(qtVersions, SIGNAL(qtVersionsChanged(QList<int>)), this, SLOT(updateEnabled()));
    connect(qtVersions, SIGNAL(qtVersionsChanged(QList<int>)), this, SLOT(onViewerChanged()));


    setDisplayName(tr("QML Viewer", "QMLRunConfiguration display name."));
}

QmlProjectRunConfiguration::~QmlProjectRunConfiguration()
{
}

QString QmlProjectRunConfiguration::debugServerAddress() const
{
    return m_debugData.serverAddress;
}

Internal::QmlProjectTarget *QmlProjectRunConfiguration::qmlTarget() const
{
    return static_cast<Internal::QmlProjectTarget *>(target());
}

QString QmlProjectRunConfiguration::viewerPath() const
{
    if (!m_qmlViewerCustomPath.isEmpty())
        return m_qmlViewerCustomPath;

    return viewerDefaultPath();
}

QStringList QmlProjectRunConfiguration::viewerArguments() const
{
    QStringList args;

    // arguments in .user file
    if (!m_qmlViewerArgs.isEmpty())
        args.append(m_qmlViewerArgs.split(QLatin1Char(' ')));

    // arguments from .qmlproject file
    foreach (const QString &importPath, qmlTarget()->qmlProject()->importPaths()) {
        args.append(QLatin1String("-I"));
        args.append(importPath);
    }

    const QString s = mainScript();
    if (! s.isEmpty())
        args.append(s);
    return args;
}

QString QmlProjectRunConfiguration::workingDirectory() const
{
    QFileInfo projectFile(qmlTarget()->qmlProject()->file()->fileName());
    return projectFile.absolutePath();
}

uint QmlProjectRunConfiguration::debugServerPort() const
{
    return m_debugData.serverPort;
}

static bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

QWidget *QmlProjectRunConfiguration::createConfigurationWidget()
{
    QWidget *config = new QWidget;
    QFormLayout *form = new QFormLayout(config);

    m_fileListCombo = new QComboBox;
    m_fileListCombo.data()->setModel(m_fileListModel);
    updateFileComboBox();

    connect(m_fileListCombo.data(), SIGNAL(activated(QString)), this, SLOT(setMainScript(QString)));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(fileListChanged()), SLOT(updateFileComboBox()));

    Utils::PathChooser *qmlViewer = new Utils::PathChooser;
    qmlViewer->setExpectedKind(Utils::PathChooser::Command);
    qmlViewer->setPath(m_qmlViewerCustomPath);

    connect(qmlViewer, SIGNAL(changed(QString)), this, SLOT(onViewerChanged()));

    m_qmlViewerExecutable = new QLabel;
    m_qmlViewerExecutable.data()->setText(viewerPath() + " " + m_qmlViewerArgs);

    QLineEdit *qmlViewerArgs = new QLineEdit;
    qmlViewerArgs->setText(m_qmlViewerArgs);
    connect(qmlViewerArgs, SIGNAL(textChanged(QString)), this, SLOT(onViewerArgsChanged()));

    form->addRow(tr("QML Viewer"), qmlViewer);
    form->addRow(tr("QML Viewer arguments:"), qmlViewerArgs);
    form->addRow(QString(), m_qmlViewerExecutable.data());

    form->addRow(tr("Main QML File:"), m_fileListCombo.data());

    return config;
}


QString QmlProjectRunConfiguration::mainScript() const
{
    if (m_usingCurrentFile)
        return m_currentFileFilename;

    return m_mainScriptFilename;
}

void QmlProjectRunConfiguration::updateFileComboBox()
{
    if (m_fileListCombo.isNull())
        return;

    QDir projectDir = qmlTarget()->qmlProject()->projectDir();
    QStringList files;

    files.append(CURRENT_FILE);
    int currentIndex = -1;
    QStringList sortedFiles = qmlTarget()->qmlProject()->files();
    qStableSort(sortedFiles.begin(), sortedFiles.end(), caseInsensitiveLessThan);

    foreach (const QString &fn, sortedFiles) {
        QFileInfo fileInfo(fn);
        if (fileInfo.suffix() != QLatin1String("qml"))
            continue;

        QString fileName = projectDir.relativeFilePath(fn);
        if (fileName == m_scriptFile)
            currentIndex = files.size();

        files.append(fileName);
    }
    m_fileListModel->setStringList(files);

    if (currentIndex != -1)
        m_fileListCombo.data()->setCurrentIndex(currentIndex);
    else
        m_fileListCombo.data()->setCurrentIndex(0);
}

void QmlProjectRunConfiguration::onDebugServerAddressChanged()
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender()))
        m_debugData.serverAddress = lineEdit->text();
}

void QmlProjectRunConfiguration::setMainScript(const QString &scriptFile)
{
    m_scriptFile = scriptFile;
    // replace with locale-agnostic string
    if (m_scriptFile == CURRENT_FILE)
        m_scriptFile = M_CURRENT_FILE;

    if (m_scriptFile.isEmpty() || m_scriptFile == M_CURRENT_FILE) {
        m_usingCurrentFile = true;
        changeCurrentFile(Core::EditorManager::instance()->currentEditor());
    } else {
        m_usingCurrentFile = false;
        m_mainScriptFilename = qmlTarget()->qmlProject()->projectDir().absoluteFilePath(scriptFile);
        updateEnabled();
    }
}

void QmlProjectRunConfiguration::onViewerChanged()
{
    if (Utils::PathChooser *chooser = qobject_cast<Utils::PathChooser *>(sender())) {
        m_qmlViewerCustomPath = chooser->path();
    }
    if (!m_qmlViewerExecutable.isNull()) {
        m_qmlViewerExecutable.data()->setText(viewerPath() + " " + m_qmlViewerArgs);
    }
}

void QmlProjectRunConfiguration::onViewerArgsChanged()
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender()))
        m_qmlViewerArgs = lineEdit->text();

    if (!m_qmlViewerExecutable.isNull()) {
        m_qmlViewerExecutable.data()->setText(viewerPath() + " " + m_qmlViewerArgs);
    }
}

void QmlProjectRunConfiguration::onDebugServerPortChanged()
{
    if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(sender())) {
        m_debugData.serverPort = spinBox->value();
    }
}

QVariantMap QmlProjectRunConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::RunConfiguration::toMap());

    map.insert(QLatin1String(Constants::QML_VIEWER_KEY), m_qmlViewerCustomPath);
    map.insert(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY), m_qmlViewerArgs);
    map.insert(QLatin1String(Constants::QML_MAINSCRIPT_KEY),  m_scriptFile);
    map.insert(QLatin1String(Constants::QML_DEBUG_SERVER_PORT_KEY), m_debugData.serverPort);
    map.insert(QLatin1String(Constants::QML_DEBUG_SERVER_ADDRESS_KEY), m_debugData.serverAddress);
    return map;
}

bool QmlProjectRunConfiguration::fromMap(const QVariantMap &map)
{
    m_qmlViewerCustomPath = map.value(QLatin1String(Constants::QML_VIEWER_KEY)).toString();
    m_qmlViewerArgs = map.value(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY)).toString();
    m_scriptFile = map.value(QLatin1String(Constants::QML_MAINSCRIPT_KEY), M_CURRENT_FILE).toString();
    m_debugData.serverPort = map.value(QLatin1String(Constants::QML_DEBUG_SERVER_PORT_KEY), Constants::QML_DEFAULT_DEBUG_SERVER_PORT).toUInt();
    m_debugData.serverAddress = map.value(QLatin1String(Constants::QML_DEBUG_SERVER_ADDRESS_KEY), QLatin1String("127.0.0.1")).toString();
    setMainScript(m_scriptFile);

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
        if (editor) {
            m_currentFileFilename = editor->file()->fileName();
            if (Core::ICore::instance()->mimeDatabase()->findByFile(mainScript()).type() == QLatin1String("application/x-qml"))
                qmlFileFound = true;
        }
        if (!editor
            || Core::ICore::instance()->mimeDatabase()->findByFile(mainScript()).type() == QLatin1String("application/x-qmlproject")) {
            // find a qml file with lowercase filename. This is slow but only done in initialization/other border cases.
            foreach(const QString &filename, m_projectTarget->qmlProject()->files()) {
                const QFileInfo fi(filename);

                if (!filename.isEmpty() && fi.baseName()[0].isLower()
                    && Core::ICore::instance()->mimeDatabase()->findByFile(fi).type() == QLatin1String("application/x-qml"))
                {
                    m_currentFileFilename = filename;
                    qmlFileFound = true;
                    break;
                }

            }
        }
    } else { // use default one
        qmlFileFound = !m_mainScriptFilename.isEmpty();
    }

    bool newValue = QFileInfo(viewerPath()).exists() && qmlFileFound;

    if (m_isEnabled != newValue) {
        m_isEnabled = newValue;
        emit isEnabledChanged(m_isEnabled);
    }
}

QString QmlProjectRunConfiguration::viewerDefaultPath() const
{
    QString path;

    // prepend creator/bin dir to search path (only useful for special creator-qml package)
    const QString searchPath = QCoreApplication::applicationDirPath()
            + Utils::SynchronousProcess::pathSeparator()
            + QString::fromLocal8Bit(qgetenv("PATH"));


#ifdef Q_OS_MAC
    const QString qmlViewerName = QLatin1String("QMLViewer");
#else
    const QString qmlViewerName = QLatin1String("qmlviewer");
#endif

    path = Utils::SynchronousProcess::locateBinary(searchPath, qmlViewerName);
    if (!path.isEmpty())
        return path;

    // Try to locate default path in Qt Versions
    Qt4ProjectManager::QtVersionManager *qtVersions = Qt4ProjectManager::QtVersionManager::instance();
    foreach (Qt4ProjectManager::QtVersion *version, qtVersions->validVersions()) {
        if (!version->qmlviewerCommand().isEmpty()
                && version->supportsTargetId(Qt4ProjectManager::Constants::DESKTOP_TARGET_ID)) {
            return version->qmlviewerCommand();
        }
    }

    return path;
}

} // namespace QmlProjectManager
