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
#include <utils/debuggerlanguagechooser.h>
#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <qt4projectmanager/qtversionmanager.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qmlobservertool.h>
#include <qt4projectmanager/qtoutputformatter.h>

#include <QFormLayout>
#include <QComboBox>
#include <QCoreApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStringListModel>
#include <QDebug>

namespace QmlProjectManager {

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Internal::QmlProjectTarget *parent) :
    ProjectExplorer::RunConfiguration(parent, QLatin1String(Constants::QML_RC_ID)),
    m_qtVersionId(-1),
    m_fileListModel(new QStringListModel(this)),
    m_projectTarget(parent),
    m_usingCurrentFile(true),
    m_isEnabled(false)
{
    ctor();
    updateQtVersions();
}

QmlProjectRunConfiguration::QmlProjectRunConfiguration(Internal::QmlProjectTarget *parent, QmlProjectRunConfiguration *source) :
    ProjectExplorer::RunConfiguration(parent, source),
    m_qtVersionId(source->m_qtVersionId),
    m_qmlViewerArgs(source->m_qmlViewerArgs),
    m_fileListModel(new QStringListModel(this)),
    m_projectTarget(parent)
{
    ctor();
    setMainScript(source->m_scriptFile);
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

    Core::EditorManager *em = Core::EditorManager::instance();
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(changeCurrentFile(Core::IEditor*)));

    Qt4ProjectManager::QtVersionManager *qtVersions = Qt4ProjectManager::QtVersionManager::instance();
    connect(qtVersions, SIGNAL(qtVersionsChanged(QList<int>)), this, SLOT(updateQtVersions()));

    setDisplayName(tr("QML Viewer", "QMLRunConfiguration display name."));
}

QmlProjectRunConfiguration::~QmlProjectRunConfiguration()
{
}

Internal::QmlProjectTarget *QmlProjectRunConfiguration::qmlTarget() const
{
    return static_cast<Internal::QmlProjectTarget *>(target());
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
}

Qt4ProjectManager::QtVersion *QmlProjectRunConfiguration::qtVersion() const
{
    if (m_qtVersionId == -1)
        return 0;

    Qt4ProjectManager::QtVersionManager *versionManager = Qt4ProjectManager::QtVersionManager::instance();
    Qt4ProjectManager::QtVersion *version = versionManager->version(m_qtVersionId);
    QTC_ASSERT(version, return 0);

    return version;
}

static bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}

QWidget *QmlProjectRunConfiguration::createConfigurationWidget()
{
    Utils::DetailsWidget *detailsWidget = new Utils::DetailsWidget();
    detailsWidget->setState(Utils::DetailsWidget::NoSummary);

    QWidget *formWidget = new QWidget(detailsWidget);
    detailsWidget->setWidget(formWidget);
    QFormLayout *form = new QFormLayout(formWidget);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_fileListCombo = new QComboBox;
    m_fileListCombo.data()->setModel(m_fileListModel);
    updateFileComboBox();

    connect(m_fileListCombo.data(), SIGNAL(activated(QString)), this, SLOT(setMainScript(QString)));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(fileListChanged()), SLOT(updateFileComboBox()));

    m_qtVersionComboBox = new QComboBox;
    m_qtVersionComboBox.data()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(m_qtVersionComboBox.data(), SIGNAL(activated(int)), this, SLOT(onQtVersionSelectionChanged()));

    QPushButton *pushButton = new QPushButton;
    pushButton->setText(tr("Manage Qt versions"));
    connect(pushButton, SIGNAL(clicked()), this, SLOT(manageQtVersions()));

    QHBoxLayout *qtVersionLayout = new QHBoxLayout;
    qtVersionLayout->addWidget(m_qtVersionComboBox.data());
    qtVersionLayout->addWidget(pushButton);

    QLineEdit *qmlViewerArgs = new QLineEdit;
    qmlViewerArgs->setText(m_qmlViewerArgs);
    connect(qmlViewerArgs, SIGNAL(textChanged(QString)), this, SLOT(onViewerArgsChanged()));

    form->addRow(tr("Qt version:"), qtVersionLayout);
    form->addRow(tr("Arguments:"), qmlViewerArgs);

    QWidget *debuggerLabelWidget = new QWidget;
    QVBoxLayout *debuggerLabelLayout = new QVBoxLayout(debuggerLabelWidget);
    debuggerLabelLayout->setMargin(0);
    debuggerLabelLayout->setSpacing(0);
    debuggerLabelWidget->setLayout(debuggerLabelLayout);
    QLabel *debuggerLabel = new QLabel(tr("Debugger:"));
    debuggerLabelLayout->addWidget(debuggerLabel);
    debuggerLabelLayout->addStretch(10);

    Utils::DebuggerLanguageChooser *debuggerLanguageChooser = new Utils::DebuggerLanguageChooser(formWidget);

    form->addRow(tr("Main QML file:"), m_fileListCombo.data());
    form->addRow(debuggerLabelWidget, debuggerLanguageChooser);

    debuggerLanguageChooser->setCppChecked(useCppDebugger());
    debuggerLanguageChooser->setQmlChecked(useQmlDebugger());
    debuggerLanguageChooser->setQmlDebugServerPort(qmlDebugServerPort());

    connect(debuggerLanguageChooser, SIGNAL(cppLanguageToggled(bool)),
            this, SLOT(useCppDebuggerToggled(bool)));
    connect(debuggerLanguageChooser, SIGNAL(qmlLanguageToggled(bool)),
            this, SLOT(useQmlDebuggerToggled(bool)));
    connect(debuggerLanguageChooser, SIGNAL(qmlDebugServerPortChanged(uint)),
            this, SLOT(qmlDebugServerPortChanged(uint)));

    updateQtVersions();
    updateEnabled();

    return detailsWidget;
}

ProjectExplorer::OutputFormatter *QmlProjectRunConfiguration::createOutputFormatter() const
{
    return new Qt4ProjectManager::QtOutputFormatter(qmlTarget()->qmlProject());
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

void QmlProjectRunConfiguration::onQtVersionSelectionChanged()
{
    QVariant data = m_qtVersionComboBox.data()->itemData(m_qtVersionComboBox.data()->currentIndex());
    QTC_ASSERT(data.isValid() && data.canConvert(QVariant::Int), return)
    setQtVersionId(data.toInt());
    updateEnabled();
}

void QmlProjectRunConfiguration::onViewerArgsChanged()
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender()))
        m_qmlViewerArgs = lineEdit->text();
}

void QmlProjectRunConfiguration::useCppDebuggerToggled(bool toggled)
{
    setUseCppDebugger(toggled);
}

void QmlProjectRunConfiguration::useQmlDebuggerToggled(bool toggled)
{
    setUseQmlDebugger(toggled);
}

void QmlProjectRunConfiguration::qmlDebugServerPortChanged(uint port)
{
    setQmlDebugServerPort(port);
}

QVariantMap QmlProjectRunConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::RunConfiguration::toMap());

    map.insert(QLatin1String(Constants::QML_VIEWER_QT_KEY), m_qtVersionId);
    map.insert(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY), m_qmlViewerArgs);
    map.insert(QLatin1String(Constants::QML_MAINSCRIPT_KEY),  m_scriptFile);
    return map;
}

bool QmlProjectRunConfiguration::fromMap(const QVariantMap &map)
{
    setQtVersionId(map.value(QLatin1String(Constants::QML_VIEWER_QT_KEY), -1).toInt());
    m_qmlViewerArgs = map.value(QLatin1String(Constants::QML_VIEWER_ARGUMENTS_KEY)).toString();
    m_scriptFile = map.value(QLatin1String(Constants::QML_MAINSCRIPT_KEY), M_CURRENT_FILE).toString();

    updateQtVersions();
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

    bool newValue = (QFileInfo(viewerPath()).exists()
                    || QFileInfo(observerPath()).exists()) && qmlFileFound;


    // Always emit change signal to force reevaluation of run/debug buttons
    m_isEnabled = newValue;
    emit isEnabledChanged(m_isEnabled);
}

void QmlProjectRunConfiguration::updateQtVersions()
{
    Qt4ProjectManager::QtVersionManager *qtVersions = Qt4ProjectManager::QtVersionManager::instance();

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

    if (!m_qtVersionComboBox)
        return;

    //
    // update combobox
    //
    m_qtVersionComboBox.data()->clear();

    foreach (Qt4ProjectManager::QtVersion *version, qtVersions->validVersions()) {
        if (isValidVersion(version)) {
            m_qtVersionComboBox.data()->addItem(version->displayName(), version->uniqueId());
        }
    }

    if (m_qtVersionId != -1) {
        int index = m_qtVersionComboBox.data()->findData(m_qtVersionId);
        QTC_ASSERT(index >= 0, return);
        m_qtVersionComboBox.data()->setCurrentIndex(index);
    } else {
        m_qtVersionComboBox.data()->addItem(tr("Invalid Qt version"), -1);
        m_qtVersionComboBox.data()->setCurrentIndex(0);
    }

}

void QmlProjectRunConfiguration::manageQtVersions()
{
    Core::ICore *core = Core::ICore::instance();
    core->showOptionsDialog(Qt4ProjectManager::Constants::QT_SETTINGS_CATEGORY,
                            Qt4ProjectManager::Constants::QTVERSION_SETTINGS_PAGE_ID);
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

} // namespace QmlProjectManager
