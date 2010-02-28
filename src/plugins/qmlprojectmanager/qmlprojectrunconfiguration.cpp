/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <utils/synchronousprocess.h>
#include <utils/pathchooser.h>

#include <QFormLayout>
#include <QComboBox>
#include <QCoreApplication>
#include <QLineEdit>
#include <QSpinBox>
#include <QDebug>

namespace QmlProjectManager {

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Internal::QmlProjectTarget *parent) :
    ProjectExplorer::RunConfiguration(parent, QLatin1String(Constants::QML_RC_ID)),
    m_debugServerAddress("127.0.0.1"),
    m_debugServerPort(Constants::QML_DEFAULT_DEBUG_SERVER_PORT)
{
    ctor();
}

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Internal::QmlProjectTarget *parent, QmlProjectRunConfiguration *source) :
    ProjectExplorer::RunConfiguration(parent, source),
    m_scriptFile(source->m_scriptFile),
    m_qmlViewerCustomPath(source->m_qmlViewerCustomPath),
    m_qmlViewerArgs(source->m_qmlViewerArgs),
    m_debugServerAddress(source->m_debugServerAddress),
    m_debugServerPort(source->m_debugServerPort)
{
    ctor();
}

void QmlProjectRunConfiguration::ctor()
{
    setDisplayName(tr("QML Viewer", "QMLRunConfiguration display name."));

    // prepend creator/bin dir to search path (only useful for special creator-qml package)
    const QString searchPath = QCoreApplication::applicationDirPath()
                               + Utils::SynchronousProcess::pathSeparator()
                               + QString(qgetenv("PATH"));
    m_qmlViewerDefaultPath = Utils::SynchronousProcess::locateBinary(searchPath, QLatin1String("qmlviewer"));
}

QmlProjectRunConfiguration::~QmlProjectRunConfiguration()
{
}

QString QmlProjectRunConfiguration::debugServerAddress() const
{
    return m_debugServerAddress;
}

Internal::QmlProjectTarget *QmlProjectRunConfiguration::qmlTarget() const
{
    return static_cast<Internal::QmlProjectTarget *>(target());
}

QString QmlProjectRunConfiguration::viewerPath() const
{
    if (!m_qmlViewerCustomPath.isEmpty())
        return m_qmlViewerCustomPath;
    qDebug() << m_qmlViewerDefaultPath;
    return m_qmlViewerDefaultPath;
}

QStringList QmlProjectRunConfiguration::viewerArguments() const
{
    QStringList args;

    // arguments in .user file
    if (!m_qmlViewerArgs.isEmpty())
        args.append(m_qmlViewerArgs);

    // arguments from .qmlproject file
    foreach (const QString &libraryPath, qmlTarget()->qmlProject()->libraryPaths()) {
        args.append(QLatin1String("-L"));
        args.append(libraryPath);
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
    return m_debugServerPort;
}

QWidget *QmlProjectRunConfiguration::configurationWidget()
{
    QWidget *config = new QWidget;
    QFormLayout *form = new QFormLayout(config);

    QComboBox *combo = new QComboBox;

    QDir projectDir = qmlTarget()->qmlProject()->projectDir();
    QStringList files;

    files.append(tr("<Current File>"));

    int currentIndex = -1;

    foreach (const QString &fn, qmlTarget()->qmlProject()->files()) {
        QFileInfo fileInfo(fn);
        if (fileInfo.suffix() != QLatin1String("qml"))
            continue;

        QString fileName = projectDir.relativeFilePath(fn);
        if (fileName == m_scriptFile)
            currentIndex = files.size();

        files.append(fileName);
    }

    combo->addItems(files);
    if (currentIndex != -1)
        combo->setCurrentIndex(currentIndex);

    connect(combo, SIGNAL(activated(QString)), this, SLOT(setMainScript(QString)));

    Utils::PathChooser *qmlViewer = new Utils::PathChooser;
    qmlViewer->setExpectedKind(Utils::PathChooser::Command);
    qmlViewer->setPath(viewerPath());
    connect(qmlViewer, SIGNAL(changed(QString)), this, SLOT(onQmlViewerChanged()));

    QLineEdit *qmlViewerArgs = new QLineEdit;
    qmlViewerArgs->setText(m_qmlViewerArgs);
    connect(qmlViewerArgs, SIGNAL(textChanged(QString)), this, SLOT(onQmlViewerArgsChanged()));

    QLineEdit *debugServer = new QLineEdit;
    debugServer->setText(m_debugServerAddress);
    connect(debugServer, SIGNAL(textChanged(QString)), this, SLOT(onDebugServerAddressChanged()));

    QSpinBox *debugPort = new QSpinBox;
    debugPort->setMinimum(1024); // valid registered/dynamic/free ports according to http://www.iana.org/assignments/port-numbers
    debugPort->setMaximum(65535);
    debugPort->setValue(m_debugServerPort);
    connect(debugPort, SIGNAL(valueChanged(int)), this, SLOT(onDebugServerPortChanged()));

    form->addRow(tr("QML Viewer"), qmlViewer);
    form->addRow(tr("QML Viewer arguments:"), qmlViewerArgs);
    form->addRow(tr("Main QML File:"), combo);
    form->addRow(tr("Debugging Address:"), debugServer);
    form->addRow(tr("Debugging Port:"), debugPort);

    return config;
}

QString QmlProjectRunConfiguration::mainScript() const
{
    if (m_scriptFile.isEmpty() || m_scriptFile == tr("<Current File>")) {
        Core::EditorManager *editorManager = Core::ICore::instance()->editorManager();
        if (Core::IEditor *editor = editorManager->currentEditor()) {
            return editor->file()->fileName();
        }
    }

    return qmlTarget()->qmlProject()->projectDir().absoluteFilePath(m_scriptFile);
}

void QmlProjectRunConfiguration::onDebugServerAddressChanged()
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender()))
        m_debugServerAddress = lineEdit->text();
}

void QmlProjectRunConfiguration::setMainScript(const QString &scriptFile)
{
    m_scriptFile = scriptFile;
}

void QmlProjectRunConfiguration::onQmlViewerChanged()
{
    if (Utils::PathChooser *chooser = qobject_cast<Utils::PathChooser *>(sender())) {
        m_qmlViewerCustomPath = chooser->path();
    }
}

void QmlProjectRunConfiguration::onQmlViewerArgsChanged()
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender()))
        m_qmlViewerArgs = lineEdit->text();
}

void QmlProjectRunConfiguration::onDebugServerPortChanged()
{
    if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(sender())) {
        m_debugServerPort = spinBox->value();
    }
}

QVariantMap QmlProjectRunConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::RunConfiguration::toMap());

    map.insert(QLatin1String(Constants::QML_VIEWER_KEY), m_qmlViewerCustomPath);
    map.insert(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY), m_qmlViewerArgs);
    map.insert(QLatin1String(Constants::QML_MAINSCRIPT_KEY),  m_scriptFile);
    map.insert(QLatin1String(Constants::QML_DEBUG_SERVER_PORT_KEY), m_debugServerPort);
    map.insert(QLatin1String(Constants::QML_DEBUG_SERVER_ADDRESS_KEY), m_debugServerAddress);
    return map;
}

bool QmlProjectRunConfiguration::fromMap(const QVariantMap &map)
{
    m_qmlViewerCustomPath = map.value(QLatin1String(Constants::QML_VIEWER_KEY)).toString();
    m_qmlViewerArgs = map.value(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY)).toString();
    m_scriptFile = map.value(QLatin1String(Constants::QML_MAINSCRIPT_KEY), tr("<Current File>")).toString();
    m_debugServerPort = map.value(QLatin1String(Constants::QML_DEBUG_SERVER_PORT_KEY), Constants::QML_DEFAULT_DEBUG_SERVER_PORT).toUInt();
    m_debugServerAddress = map.value(QLatin1String(Constants::QML_DEBUG_SERVER_ADDRESS_KEY), QLatin1String("127.0.0.1")).toString();

    return RunConfiguration::fromMap(map);
}

} // namespace QmlProjectManager
