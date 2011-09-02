/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "testsettingspropertiespage.h"
#include "testconfigurations.h"

#include <projectexplorer/project.h>

#include <QFileDialog>
#include <QDebug>

using namespace QtTest;
using namespace QtTest::Internal;

QString TestSettingsPanelFactory::id() const
{
    return QLatin1String(TESTSETTINGS_PANEL_ID);
}

QString TestSettingsPanelFactory::displayName() const
{
    return QCoreApplication::translate("TestSettingsPanelFactory", "Test Settings");
}

bool TestSettingsPanelFactory::supports(ProjectExplorer::Project *project)
{
    Q_UNUSED(project);
    return true;
}

ProjectExplorer::PropertiesPanel *TestSettingsPanelFactory::createPanel(ProjectExplorer::Project *project)
{
    ProjectExplorer::PropertiesPanel *panel = new ProjectExplorer::PropertiesPanel;
    panel->setWidget(new TestSettingsWidget(project));
    panel->setIcon(QIcon(":/projectexplorer/images/EditorSettings.png"));
    panel->setDisplayName(QCoreApplication::translate("TestSettingsPanel", "Test Settings"));
    return panel;
}

TestSettingsWidget::TestSettingsWidget(ProjectExplorer::Project *project) :
    QWidget(),
    m_project(project)
{
    m_ui.setupUi(this);

    TestConfigurations::instance().delayConfigUpdates(true);

    m_uploadMode.addButton(m_ui.upload_auto, 0);
    m_uploadMode.addButton(m_ui.upload_ask_me, 1);
    m_uploadMode.addButton(m_ui.upload_no_thanks, 2);
    m_uploadMethod.addButton(m_ui.upload_scp, 0);
    m_uploadMethod.addButton(m_ui.upload_email, 1);

    m_testConfig = TestConfigurations::instance().config(project->displayName());

    if (m_testConfig) {
        m_ui.auto_detect_platform_configuration->setChecked(m_testConfig->autoDetectPlatformConfiguration());
        m_ui.upload_change->setText(m_testConfig->uploadChange());
        m_ui.upload_branch->setText(m_testConfig->uploadBranch());
        m_ui.upload_branch_Specialization->setText(m_testConfig->uploadBranchSpecialization());
        m_ui.host_platform->setText(m_testConfig->uploadPlatform());
        m_ui.qmakespec->setText(m_testConfig->QMAKESPEC());
        m_ui.qmakespecSpecialization->setText(m_testConfig->QMAKESPECSpecialization());

        m_ui.upload_scp->setChecked(m_testConfig->uploadUsingScp());
        m_ui.upload_email->setChecked(m_testConfig->uploadUsingEMail());
        setExtraDirectories(m_testConfig->extraTests());


        switch (m_testConfig->uploadMode()) {
        case TestConfig::UploadAuto:
            m_ui.upload_auto->setChecked(true);
            break;
        case TestConfig::UploadAskMe:
            m_ui.upload_ask_me->setChecked(true);
            break;
        case TestConfig::UploadNoThanks:
            m_ui.upload_no_thanks->setChecked(true);
            break;
        }
    }
    m_ui.upload_server->setText(m_testSettings.uploadServer());
    m_ui.show_passed_results->setChecked(m_testSettings.showPassedResults());
    m_ui.show_debug_results->setChecked(m_testSettings.showDebugResults());
    m_ui.show_skipped_results->setChecked(m_testSettings.showSkippedResults());
    m_ui.show_verbose_make_results->setChecked(m_testSettings.showVerbose());

    onChanged();

    connect(m_ui.upload_change, SIGNAL(textEdited(QString)),
        this, SLOT(onChanged()), Qt::DirectConnection);

    connect(m_ui.upload_branch, SIGNAL(textEdited(QString)),
        this, SLOT(onChanged()), Qt::DirectConnection);
    connect(m_ui.upload_branch_Specialization, SIGNAL(textEdited(QString)),
        this, SLOT(onChanged()), Qt::DirectConnection);

    connect(m_ui.auto_detect_platform_configuration, SIGNAL(toggled(bool)),
        this, SLOT(onChanged()), Qt::DirectConnection);

    connect(m_ui.qmakespec, SIGNAL(textEdited(QString)),
        this, SLOT(onChanged()), Qt::DirectConnection);
    connect(m_ui.qmakespecSpecialization, SIGNAL(textEdited(QString)),
        this, SLOT(onChanged()), Qt::DirectConnection);

    connect(m_ui.upload_server, SIGNAL(textEdited(QString)),
        this, SLOT(onChanged()), Qt::DirectConnection);
    connect(m_ui.host_platform, SIGNAL(textEdited(QString)),
        this, SLOT(onChanged()), Qt::DirectConnection);
    connect(m_ui.show_passed_results, SIGNAL(toggled(bool)),
        this, SLOT(onChanged()), Qt::DirectConnection);
    connect(m_ui.show_debug_results, SIGNAL(toggled(bool)),
        this, SLOT(onChanged()), Qt::DirectConnection);
    connect(m_ui.show_skipped_results, SIGNAL(toggled(bool)),
        this, SLOT(onChanged()), Qt::DirectConnection);

    connect(m_ui.show_verbose_make_results, SIGNAL(toggled(bool)),
        this, SLOT(onChanged()), Qt::DirectConnection);

    connect(&m_uploadMode, SIGNAL(buttonClicked(int)),
        this, SLOT(onChanged()), Qt::DirectConnection);
    connect(&m_uploadMethod, SIGNAL(buttonClicked(int)),
        this, SLOT(onChanged()), Qt::DirectConnection);

    connect(m_ui.add_directory_btn, SIGNAL(pressed()),
        this, SLOT(onAddDirectoryBtnClicked()), Qt::DirectConnection);
    connect(m_ui.clear_directory_btn, SIGNAL(pressed()),
        this, SLOT(onClearDirectoryBtnClicked()), Qt::DirectConnection);

    connect(&m_testSettings, SIGNAL(changed()),
        this, SLOT(onSettingsChanged()), Qt::DirectConnection);

    setFocusPolicy(Qt::StrongFocus);
}

TestSettingsWidget::~TestSettingsWidget()
{
    m_testSettings.save();

    TestConfigurations::instance().delayConfigUpdates(false);
}


// Update settings before focus moves away from widget
void TestSettingsWidget::leaveEvent(QEvent *event)
{
    m_testSettings.save();

    TestConfigurations::instance().delayConfigUpdates(false);
    QWidget::leaveEvent(event);
}


void TestSettingsWidget::enterEvent(QEvent *event)
{
    TestConfigurations::instance().delayConfigUpdates(true);
    QWidget::enterEvent(event);
}


void TestSettingsWidget::onChanged()
{
    if (m_testConfig) {
        bool auto_detect = m_ui.auto_detect_platform_configuration->isChecked();
        bool auto_detect_enabled = (auto_detect && !m_testConfig->autoDetectPlatformConfiguration());

        m_testConfig->setAutoDetectPlatformConfiguration(auto_detect);
        m_testConfig->setUploadMethod(m_ui.upload_scp->isChecked());
        m_testConfig->setExtraTests(extraDirectories());

        m_ui.upload_change->setEnabled(!auto_detect);
        m_ui.upload_branch->setEnabled(!auto_detect);
        m_ui.host_platform->setEnabled(!auto_detect);
        m_ui.qmakespec->setEnabled(!auto_detect);
        m_ui.platform_label->setEnabled(!auto_detect);
        m_ui.change_label->setEnabled(!auto_detect);
        m_ui.branch_label->setEnabled(!auto_detect);
        m_ui.qmakespec_label->setEnabled(!auto_detect);

        if (auto_detect) {
            m_ui.upload_change->setText(m_testConfig->uploadChange());
            m_ui.upload_branch->setText(m_testConfig->uploadBranch());
            m_ui.host_platform->setText(m_testConfig->uploadPlatform());
            QString device_name;
            Utils::SshConnectionParameters dummy(Utils::SshConnectionParameters::DefaultProxy);
            QString device_type;

            // do not override current choice using remote target name unless
            // auto detect was just enabled by user.
            if (auto_detect_enabled) {
                m_ui.qmakespec->setText(m_testConfig->QMAKESPEC());
                // refresh qmakespec value from autodetected value
                m_testConfig->setQMAKESPEC(m_ui.qmakespec->text().trimmed());
                if (m_testConfig->isRemoteTarget(device_name, device_type, dummy)){
                    // update branch and qmakespec specialization
                    m_ui.qmakespecSpecialization->setText(device_name);
                    m_testConfig->setQMAKESPECSpecialization(m_ui.qmakespecSpecialization->text().trimmed());
                    m_ui.upload_branch_Specialization->setText(device_name);
                    m_testConfig->setUploadBranchSpecialization(m_ui.upload_branch_Specialization->text().trimmed());
                }
            }
        } else {
            m_testConfig->setUploadChange(m_ui.upload_change->text().trimmed());
            m_testConfig->setUploadBranch(m_ui.upload_branch->text().trimmed());
            m_testConfig->setUploadPlatform(m_ui.host_platform->text().trimmed());
            m_testConfig->setQMAKESPEC(m_ui.qmakespec->text().trimmed());
            m_testConfig->setQMAKESPECSpecialization(m_ui.qmakespecSpecialization->text().trimmed());
            m_testConfig->setUploadBranchSpecialization(m_ui.upload_branch_Specialization->text().trimmed());
        }

        m_testConfig->setUploadMode((TestConfig::UploadMode)m_uploadMode.checkedId());

    }

    // the effective branch and qmakespec is computed in TestExecuter::runSelectedTests()
    // use same logic here to support the display of the actual branch and qmakespec
    // to be used when uploading test results
    if (!m_ui.upload_branch_Specialization->text().trimmed().isEmpty())
        m_ui.effectiveBranchName->setText(m_ui.upload_branch->text().trimmed()
            + QLatin1Char('-') + m_ui.upload_branch_Specialization->text().trimmed());
    else
        m_ui.effectiveBranchName->setText(m_ui.upload_branch->text().trimmed());

    if (!m_ui.qmakespecSpecialization->text().trimmed().isEmpty())
        m_ui.effectiveQMakespec->setText(m_ui.qmakespec->text().trimmed()
            + QLatin1Char('_') + m_ui.qmakespecSpecialization->text().trimmed());
    else
        m_ui.effectiveQMakespec->setText(m_ui.qmakespec->text().trimmed());

    m_testSettings.setUploadServer(m_ui.upload_server->text());
    m_testSettings.setShowPassedResults(m_ui.show_passed_results->isChecked());
    m_testSettings.setShowDebugResults(m_ui.show_debug_results->isChecked());
    m_testSettings.setShowSkippedResults(m_ui.show_skipped_results->isChecked());
    m_testSettings.setShowVerbose(m_ui.show_verbose_make_results->isChecked());
}

void TestSettingsWidget::onUploadScpChanged()
{
    m_ui.upload_email->setChecked(!m_ui.upload_scp->isChecked());
    onChanged();
}

void TestSettingsWidget::onUploadEMailChanged()
{
    m_ui.upload_scp->setChecked(!m_ui.upload_email->isChecked());
    onChanged();
}

void TestSettingsWidget::onAddDirectoryBtnClicked()
{
    QString newDirectory = QFileDialog::getExistingDirectory(this, "Choose Extra Test Directory");
    if (!newDirectory.isEmpty())
        m_ui.extra_tests->addItem(newDirectory);
    onChanged();
}

void TestSettingsWidget::onClearDirectoryBtnClicked()
{
    m_ui.extra_tests->clear();
    onChanged();
}

void TestSettingsWidget::onSettingsChanged()
{
    if (m_ui.show_passed_results->isChecked() != m_testSettings.showPassedResults())
        m_ui.show_passed_results->setChecked(m_testSettings.showPassedResults());
    if (m_ui.show_debug_results->isChecked() != m_testSettings.showDebugResults())
        m_ui.show_debug_results->setChecked(m_testSettings.showDebugResults());
    if (m_ui.show_skipped_results->isChecked() != m_testSettings.showSkippedResults())
        m_ui.show_skipped_results->setChecked(m_testSettings.showSkippedResults());
}

QStringList TestSettingsWidget::extraDirectories() const
{
    QStringList result;
    for (int index = 0; index < m_ui.extra_tests->count(); ++index)
        result.append(m_ui.extra_tests->item(index)->text());

    return result;
}

void TestSettingsWidget::setExtraDirectories(const QStringList list)
{
    m_ui.extra_tests->clear();

    foreach (const QString &s, list)
        m_ui.extra_tests->addItem(s);
}
