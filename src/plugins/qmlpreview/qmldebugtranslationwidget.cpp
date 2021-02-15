/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qmldebugtranslationwidget.h"
#include "qmlpreviewruncontrol.h"
#include "qmlpreviewplugin.h"
#include "projectfileselectionswidget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/outputwindow.h>

#include <projectexplorer/runcontrol.h>
#include <projectexplorer/projecttree.h>

#include <utils/outputformatter.h>
#include <utils/utilsicons.h>
#include <utils/fileutils.h>
#include <utils/qtcolorbutton.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/iplugin.h>

#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <qmlprojectmanager/qmlmultilanguageaspect.h>

#include <qtsupport/qtoutputformatter.h>

#include <QIcon>
#include <QRegularExpression>

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QAction>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QButtonGroup>
#include <QRadioButton>
#include <QSpacerItem>
#include <QToolButton>
#include <QTextBlock>
#include <QFileDialog>

namespace {
QObject *getPreviewPlugin()
{
    const QVector<ExtensionSystem::PluginSpec *> &specs = ExtensionSystem::PluginManager::plugins();
    const auto pluginIt = std::find_if(specs.cbegin(), specs.cend(),
                                 [](const ExtensionSystem::PluginSpec *p) {
        return p->name() == "QmlPreview";
    });

    if (pluginIt != specs.cend())
        return (*pluginIt)->plugin();

    return nullptr;
}

}

namespace QmlPreview {

QmlDebugTranslationWidget::QmlDebugTranslationWidget(QWidget *parent, TestLanguageGetter languagesGetterMethod)
    : QWidget(parent)
    , m_testLanguagesGetter(languagesGetterMethod)
    , m_warningColor(Qt::red)
    //, m_foundTrColor(Qt::green) // invalid color -> init without the frame
    , m_lastWarningColor(m_warningColor)
    , m_lastfoundTrColor(Qt::green)

{
    auto mainLayout = new QVBoxLayout(this);

    auto buttonGroup = new QButtonGroup(this);
    // it gets the text from updateCurrentEditor method
    m_singleFileButton = new QRadioButton();
    m_singleFileButton->setChecked(true);
    buttonGroup->addButton(m_singleFileButton);

    const QString projectSettingsKey = "QmlPreview.DisabledDebugTranslationFiles";
    const ProjectExplorer::FileType filterFileType = ProjectExplorer::FileType::QML;
    m_checkableProjectFileView = new ProjectFileSelectionsWidget(projectSettingsKey, filterFileType);
    m_checkableProjectFileView->setVisible(false);
    connect(m_checkableProjectFileView, &ProjectFileSelectionsWidget::selectionChanged, this, &QmlDebugTranslationWidget::setFiles);
    m_multipleFileButton = new QRadioButton(tr("Multiple files"));
    buttonGroup->addButton(m_multipleFileButton);
    connect(m_multipleFileButton, &QAbstractButton::toggled, m_checkableProjectFileView, &QWidget::setVisible);
    connect(m_multipleFileButton, &QAbstractButton::toggled, this, &QmlDebugTranslationWidget::updateFiles);

    mainLayout->addWidget(m_singleFileButton);
    mainLayout->addWidget(m_multipleFileButton);
    mainLayout->addWidget(m_checkableProjectFileView);

    // language checkboxes are add in updateAvailableTranslations method
    m_selectLanguageLayout = new QHBoxLayout;
    mainLayout->addLayout(m_selectLanguageLayout);

    auto settingsLayout = new QHBoxLayout();
    mainLayout->addLayout(settingsLayout);

    auto elideWarningCheckBox = new QCheckBox(tr("Elide warning"));
    connect(elideWarningCheckBox, &QCheckBox::stateChanged, [this] (int state) {
        m_elideWarning = (state == Qt::Checked);
    });
    settingsLayout->addWidget(elideWarningCheckBox);

    auto warningColorCheckbox = new QCheckBox(tr("Warning color: "));
    settingsLayout->addWidget(warningColorCheckbox);
    auto warningColorButton = new Utils::QtColorButton();
    connect(warningColorCheckbox, &QCheckBox::stateChanged, [warningColorButton, this] (int state) {
        if (state == Qt::Checked) {
            warningColorButton->setColor(m_lastWarningColor);
            warningColorButton->setEnabled(true);
        } else {
            m_lastWarningColor = warningColorButton->color();
            warningColorButton->setColor({});
            warningColorButton->setEnabled(false);
        }
    });
    connect(warningColorButton, &Utils::QtColorButton::colorChanged, [this](const QColor &color) {
        m_warningColor = color;
    });
    warningColorCheckbox->setCheckState(Qt::Checked);
    settingsLayout->addWidget(warningColorButton);

    auto foundTrColorCheckbox = new QCheckBox(tr("Found \"tr\" color: "));
    settingsLayout->addWidget(foundTrColorCheckbox);
    auto foundTrColorButton = new Utils::QtColorButton();
    foundTrColorButton->setDisabled(true);
    connect(foundTrColorCheckbox, &QCheckBox::stateChanged, [foundTrColorButton, this] (int state) {
        if (state == Qt::Checked) {
            foundTrColorButton->setColor(m_lastfoundTrColor);
            foundTrColorButton->setEnabled(true);
        } else {
            m_lastfoundTrColor = foundTrColorButton->color();
            foundTrColorButton->setColor({});
            foundTrColorButton->setEnabled(false);
        }
    });
    connect(foundTrColorButton, &Utils::QtColorButton::colorChanged, [this](const QColor &color) {
        m_foundTrColor = color;
    });
    settingsLayout->addWidget(foundTrColorButton);

    settingsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

    auto controlLayout = new QHBoxLayout;
    mainLayout->addLayout(controlLayout);

    auto showLogButton = new QToolButton;
    showLogButton->setText(tr("Show Log"));
    showLogButton->setCheckable(true);
    controlLayout->addWidget(showLogButton);

    // TODO: do we still need this buttons?
//    auto pauseButton = new QToolButton;
//    pauseButton->setText(tr("Pause"));
//    pauseButton->setCheckable(true);
//    controlLayout->addWidget(pauseButton);

//    auto onTheFlyButton = new QToolButton;
//    onTheFlyButton->setText(tr("On the Fly"));
//    controlLayout->addWidget(onTheFlyButton);

    m_runTestButton = new QPushButton();
    m_runTestButton->setCheckable(true);
    m_runTestButton->setText(runButtonText());
    connect(m_runTestButton, &QPushButton::toggled, [this](bool checked) {
        m_runTestButton->setText(runButtonText(checked));
    });

    connect(m_runTestButton, &QPushButton::clicked, [this](bool checked) {
        if (checked)
            runTest();
        else {
            if (m_currentRunControl)
                m_currentRunControl->initiateStop();
            // TODO: what happens if we already have a preview running?
//            QmlPreviewPlugin::stopAllRunControls();
//            qWarning() << "not implemented"; // TODO: stop still running tests
        }
    });
    controlLayout->addWidget(m_runTestButton);

    m_runOutputWindow = new Core::OutputWindow(Core::Context("QmlPreview.DebugTranslation"),
                                               "QmlPreview/OutputWindow/Zoom");

    m_runOutputWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_runOutputWindow->setReadOnly(true);
    m_runOutputWindow->setVisible(false);
    mainLayout->addWidget(m_runOutputWindow);

    QSpacerItem *endSpacerItem = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addItem(endSpacerItem);

    connect(showLogButton, &QToolButton::toggled, m_runOutputWindow, [this, mainLayout, endSpacerItem](bool checked) {
        m_runOutputWindow->setVisible(checked);
        if (m_runOutputWindow->isVisible())
            mainLayout->takeAt(mainLayout->count() - 1);
        else
            mainLayout->addItem(endSpacerItem);
    });

    auto loadLogButton = new QToolButton;
    loadLogButton->setText(tr("Load"));
    controlLayout->addWidget(loadLogButton);
    connect(loadLogButton, &QToolButton::clicked, this, &QmlDebugTranslationWidget::loadLogFile);

    auto saveLogButton = new QToolButton;
    saveLogButton->setText(tr("Save"));
    controlLayout->addWidget(saveLogButton);
    connect(saveLogButton, &QToolButton::clicked, this, &QmlDebugTranslationWidget::saveLogToFile);

    auto clearButton = new QToolButton;
    clearButton->setText(tr("Clear"));
    controlLayout->addWidget(clearButton);
    connect(clearButton, &QToolButton::clicked, this, &QmlDebugTranslationWidget::clear);

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::currentEditorChanged, this, &QmlDebugTranslationWidget::updateCurrentEditor);
    updateCurrentEditor(Core::EditorManager::currentEditor());

    connect(ProjectExplorer::SessionManager::instance(),  &ProjectExplorer::SessionManager::startupProjectChanged,
        this, &QmlDebugTranslationWidget::updateCurrentTranslations);

    updateStartupProjectTranslations();

    ProjectExplorer::TaskHub::addCategory("QmlPreview.Translation", tr("Translation issues"));
}

QmlDebugTranslationWidget::~QmlDebugTranslationWidget()
{

}

void QmlDebugTranslationWidget::updateCurrentEditor(const Core::IEditor *editor)
{
    if (editor && editor->document())
        m_currentFilePath = editor->document()->filePath();
    else
        m_currentFilePath.clear();
    m_singleFileButton->setText(singleFileButtonText(m_currentFilePath.toString()));
    updateFiles();
}

void QmlDebugTranslationWidget::updateStartupProjectTranslations()
{
    updateCurrentTranslations(ProjectExplorer::SessionManager::startupProject());
}

QColor QmlDebugTranslationWidget::warningColor()
{
    return m_warningColor;
}

QColor QmlDebugTranslationWidget::foundTrColor()
{
    return m_foundTrColor;
}

void QmlDebugTranslationWidget::updateCurrentTranslations(ProjectExplorer::Project *project)
{
    m_testLanguages.clear();
    for (int i = m_selectLanguageLayout->count()-1; i >= 0; --i) {
        auto layoutItem = m_selectLanguageLayout->takeAt(i);
        delete layoutItem->widget();
        delete layoutItem;
    }
    if (!project)
        return;

    if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current(project)) {
        connect(multiLanguageAspect, &QmlProjectManager::QmlMultiLanguageAspect::changed,
                this, &QmlDebugTranslationWidget::updateStartupProjectTranslations,
                Qt::UniqueConnection);
        auto languageLabel = new QLabel();
        languageLabel->setText(tr("Language to test:"));
        m_selectLanguageLayout->addWidget(languageLabel);
        if (multiLanguageAspect->value()) {
            addLanguageCheckBoxes({multiLanguageAspect->currentLocale()});
            if (m_testLanguagesGetter) {
                auto addTestLanguages = new QPushButton(tr("Add Test Languages"));
                m_selectLanguageLayout->addWidget(addTestLanguages);
                connect(addTestLanguages, &QPushButton::clicked, [this]() {
                    addLanguageCheckBoxes(m_testLanguagesGetter());
                });
            }
        } else {
            QString errorMessage;
            addLanguageCheckBoxes(project->availableQmlPreviewTranslations(&errorMessage));
        }
        m_selectLanguageLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    }
}

void QmlDebugTranslationWidget::updateFiles()
{
    if (m_multipleFileButton->isChecked())
        setFiles(m_checkableProjectFileView->checkedFiles());
    else
        setFiles({m_currentFilePath});
}

void QmlDebugTranslationWidget::setFiles(const Utils::FilePaths &filePathes)
{
    m_selectedFilePaths = filePathes;
}

void QmlDebugTranslationWidget::runTest()
{
    m_runOutputWindow->grayOutOldContent();

    auto runControl = new ProjectExplorer::RunControl(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
    QTC_ASSERT(runControl, qWarning("Can not create a QmlPreviewRunner"); return;);
    auto previewPlugin = qobject_cast<Internal::QmlPreviewPlugin*>(getPreviewPlugin());

    connect(runControl, &ProjectExplorer::RunControl::started, [this, runControl, previewPlugin]() {
        //Q_ASSERT(m_currentRunControl == nullptr); //TODO: who deletes the runcontrol
        m_currentRunControl = runControl;
        m_runOutputWindow->setLineParsers(
            ProjectExplorer::OutputFormatterFactory::createFormatters(runControl->target()));
        int timerCounter = 1;
        const auto testLanguageList = m_testLanguages;

        if (m_elideWarning)
            previewPlugin->changeElideWarning(true);

        auto testLanguages = [previewPlugin, runControl, testLanguageList](int timerCounter, const QString &previewedFile) {
            for (auto language : testLanguageList) {
                QTimer::singleShot(timerCounter * 1000, previewPlugin, [previewPlugin, runControl, language, previewedFile]() {
                    if (runControl && runControl->isRunning()) {
                        if (!previewedFile.isEmpty())
                            previewPlugin->setPreviewedFile(previewedFile);
                        previewPlugin->setLocaleIsoCode(language);
                    }
                });
            }
        };
        for (auto filePath : qAsConst(m_selectedFilePaths)) {
            testLanguages(timerCounter++, filePath.toString());
        }
    });
    connect(runControl, &ProjectExplorer::RunControl::stopped, [this]() {
        m_runTestButton->setChecked(false);
        //delete m_currentRunControl; // who deletes the runcontrol?
        m_currentRunControl = nullptr;
        if (auto previewPlugin = qobject_cast<Internal::QmlPreviewPlugin*>(getPreviewPlugin()))
            previewPlugin->setLocaleIsoCode(m_lastUsedLanguageBeforeTest);
    });

    connect(runControl, &ProjectExplorer::RunControl::appendMessage,
            this, &QmlDebugTranslationWidget::appendMessage);

    if (auto project = ProjectExplorer::SessionManager::startupProject()) {
        if (auto target = project->activeTarget()) {
            if (auto multiLanguageAspect = QmlProjectManager::QmlMultiLanguageAspect::current(target))
                m_lastUsedLanguageBeforeTest = multiLanguageAspect->currentLocale();
            if (auto runConfiguration = target->activeRunConfiguration()) {
                runControl->setRunConfiguration(runConfiguration);
                if (runControl->createMainWorker()) {
                    previewPlugin->setLocaleIsoCode(QString());
                    runControl->initiateStart();
                }
            }
        }
    }
}

void QmlDebugTranslationWidget::clear()
{
    m_runOutputWindow->clear();
    ProjectExplorer::TaskHub::clearTasks("QmlPreview.Translation");
}

QString QmlDebugTranslationWidget::currentDir() const
{
    return m_lastDir.isEmpty() ?
        ProjectExplorer::ProjectTree::currentFilePath().parentDir().toString() : m_lastDir;
}

void QmlDebugTranslationWidget::setCurrentDir(const QString &path)
{
    m_lastDir = path;
}

void QmlDebugTranslationWidget::loadLogFile()
{
    const auto fileName = QFileDialog::getOpenFileName(this, QStringLiteral("Open File"), currentDir());
    if (!fileName.isEmpty()) {
        setCurrentDir(QFileInfo(fileName).absolutePath());
        QFile f(fileName);
        if (f.open(QFile::ReadOnly)) {
            clear();
            while (!f.atEnd())
                appendMessage(QString::fromUtf8(f.readLine()), Utils::GeneralMessageFormat);
        } else {
            // TODO: maybe add this message to log and tasks
            qWarning() << "Failed to open" << fileName << ":" << f.errorString();
        }
    }
}

void QmlDebugTranslationWidget::saveLogToFile()
{
    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Choose file to save logged issues."), currentDir());
    if (!fileName.isEmpty()) {
        setCurrentDir(QFileInfo(fileName).absolutePath());
        QFile f(fileName);
        if (f.open(QFile::WriteOnly | QFile::Text))
            f.write(m_runOutputWindow->toPlainText().toUtf8());
    }
}

void QmlDebugTranslationWidget::appendMessage(const QString &message, Utils::OutputFormat format)
{
    const auto newLine = QRegularExpression("[\r\n]");
    const auto messages = message.split(newLine, Qt::SkipEmptyParts);

    if (messages.count() > 1) {
        for (auto m : messages)
            appendMessage(m + "\n", format);
        return;
    }
    const QString serviceSeperator = ": QQmlDebugTranslationService: ";
    if (!message.contains(serviceSeperator))
        return;
    QString locationString = message;
    locationString = locationString.split(serviceSeperator).first();
    static const QRegularExpression qmlLineColumnLink("^(" QT_QML_URL_REGEXP ")" // url
                                                      ":(\\d+)"                  // line
                                                      ":(\\d+)$");               // column
    const QRegularExpressionMatch qmlLineColumnMatch = qmlLineColumnLink.match(locationString);

    auto fileLine = -1;
    QUrl fileUrl;
    if (qmlLineColumnMatch.hasMatch()) {
        fileUrl = QUrl(qmlLineColumnMatch.captured(1));
        fileLine = qmlLineColumnMatch.captured(2).toInt();
    }

    m_runOutputWindow->appendMessage(message, format);


    auto type = ProjectExplorer::Task::TaskType::Warning;
    auto description = message.split(serviceSeperator).at(1);
    auto filePath = Utils::FilePath::fromString(fileUrl.toLocalFile());
    auto category = "QmlPreview.Translation";
    auto icon = Utils::Icons::WARNING.icon();

    ProjectExplorer::TaskHub::addTask(ProjectExplorer::Task(type,
                                                            description,
                                                            filePath,
                                                            fileLine,
                                                            category,
                                                            icon,
                                                            ProjectExplorer::Task::NoOptions));
}

QString QmlDebugTranslationWidget::singleFileButtonText(const QString &filePath)
{
    auto buttonText = tr("Current file: %1");
    if (filePath.isEmpty())
        return buttonText.arg(tr("Empty"));
    return buttonText.arg(filePath);
}

QString QmlDebugTranslationWidget::runButtonText(bool isRunning)
{
    if (isRunning) {
        return tr("Stop");
    }
    return tr("Run Language Tests");
}

void QmlDebugTranslationWidget::addLanguageCheckBoxes(const QStringList &languages)
{
    for (auto language : languages) {
        auto languageCheckBox = new QCheckBox(language);
        m_selectLanguageLayout->addWidget(languageCheckBox);
        connect(languageCheckBox, &QCheckBox::stateChanged, [this, language] (int state) {
            if (state == Qt::Checked)
                m_testLanguages.append(language);
            else
                m_testLanguages.removeAll(language);
        });
        languageCheckBox->setChecked(true);
    }
}

} // namespace QmlPreview
