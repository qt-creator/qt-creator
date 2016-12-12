/****************************************************************************
**
** Copyright (C) 2016 Canonical Ltd.
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

#include "cmakeprojectconstants.h"
#include "cmakekitconfigwidget.h"
#include "cmakekitinformation.h"
#include "cmaketoolmanager.h"
#include "cmaketool.h"

#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/elidinglabel.h>
#include <utils/qtcassert.h>

#include <QBoxLayout>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// CMakeKitConfigWidget:
// --------------------------------------------------------------------

CMakeKitConfigWidget::CMakeKitConfigWidget(Kit *kit,
                                           const KitInformation *ki) :
    KitConfigWidget(kit, ki),
    m_comboBox(new QComboBox),
    m_manageButton(new QPushButton(KitConfigWidget::msgManage()))
{
    m_comboBox->setEnabled(false);
    m_comboBox->setToolTip(toolTip());

    foreach (CMakeTool *tool, CMakeToolManager::cmakeTools())
        cmakeToolAdded(tool->id());

    updateComboBox();

    refresh();
    connect(m_comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &CMakeKitConfigWidget::currentCMakeToolChanged);

    m_manageButton->setContentsMargins(0, 0, 0, 0);
    connect(m_manageButton, &QPushButton::clicked,
            this, &CMakeKitConfigWidget::manageCMakeTools);

    CMakeToolManager *cmakeMgr = CMakeToolManager::instance();
    connect(cmakeMgr, &CMakeToolManager::cmakeAdded,
            this, &CMakeKitConfigWidget::cmakeToolAdded);
    connect(cmakeMgr, &CMakeToolManager::cmakeRemoved,
            this, &CMakeKitConfigWidget::cmakeToolRemoved);
    connect(cmakeMgr, &CMakeToolManager::cmakeUpdated,
            this, &CMakeKitConfigWidget::cmakeToolUpdated);
}

CMakeKitConfigWidget::~CMakeKitConfigWidget()
{
    delete m_comboBox;
    delete m_manageButton;
}

QString CMakeKitConfigWidget::displayName() const
{
    return tr("CMake Tool:");
}

void CMakeKitConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

void CMakeKitConfigWidget::refresh()
{
    CMakeTool *tool = CMakeKitInformation::cmakeTool(m_kit);
    m_comboBox->setCurrentIndex(tool == 0 ? -1 : indexOf(tool->id()));
}

QWidget *CMakeKitConfigWidget::mainWidget() const
{
    return m_comboBox;
}

QWidget *CMakeKitConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

QString CMakeKitConfigWidget::toolTip() const
{
    return tr("The CMake Tool to use when building a project with CMake.<br>"
              "This setting is ignored when using other build systems.");
}

int CMakeKitConfigWidget::indexOf(const Core::Id &id)
{
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (id == Core::Id::fromSetting(m_comboBox->itemData(i)))
            return i;
    }
    return -1;
}

void CMakeKitConfigWidget::cmakeToolAdded(const Core::Id &id)
{
    const CMakeTool *tool = CMakeToolManager::findById(id);
    QTC_ASSERT(tool, return);

    m_comboBox->addItem(tool->displayName(), tool->id().toSetting());
    updateComboBox();
    refresh();
}

void CMakeKitConfigWidget::cmakeToolUpdated(const Core::Id &id)
{
    const int pos = indexOf(id);
    QTC_ASSERT(pos >= 0, return);

    const CMakeTool *tool = CMakeToolManager::findById(id);
    QTC_ASSERT(tool, return);

    m_comboBox->setItemText(pos, tool->displayName());
}

void CMakeKitConfigWidget::cmakeToolRemoved(const Core::Id &id)
{
    const int pos = indexOf(id);
    QTC_ASSERT(pos >= 0, return);

    // do not handle the current index changed signal
    m_removingItem = true;
    m_comboBox->removeItem(pos);
    m_removingItem = false;

    // update the checkbox and set the current index
    updateComboBox();
    refresh();
}

void CMakeKitConfigWidget::updateComboBox()
{
    // remove unavailable cmake tool:
    int pos = indexOf(Core::Id());
    if (pos >= 0)
        m_comboBox->removeItem(pos);

    if (m_comboBox->count() == 0) {
        m_comboBox->addItem(tr("<No CMake Tool available>"),
                            Core::Id().toSetting());
        m_comboBox->setEnabled(false);
    } else {
        m_comboBox->setEnabled(true);
    }
}

void CMakeKitConfigWidget::currentCMakeToolChanged(int index)
{
    if (m_removingItem)
        return;

    const Core::Id id = Core::Id::fromSetting(m_comboBox->itemData(index));
    CMakeKitInformation::setCMakeTool(m_kit, id);
}

void CMakeKitConfigWidget::manageCMakeTools()
{
    Core::ICore::showOptionsDialog(Constants::CMAKE_SETTINGSPAGE_ID,
                                   buttonWidget());
}

// --------------------------------------------------------------------
// CMakeGeneratorKitConfigWidget:
// --------------------------------------------------------------------


CMakeGeneratorKitConfigWidget::CMakeGeneratorKitConfigWidget(Kit *kit,
                                                             const KitInformation *ki) :
    KitConfigWidget(kit, ki),
    m_label(new QLabel),
    m_changeButton(new QPushButton)
{
    m_label->setToolTip(toolTip());
    m_changeButton->setText(tr("Change..."));

    refresh();
    connect(m_changeButton, &QPushButton::clicked,
            this, &CMakeGeneratorKitConfigWidget::changeGenerator);
}

CMakeGeneratorKitConfigWidget::~CMakeGeneratorKitConfigWidget()
{
    delete m_label;
    delete m_changeButton;
}

QString CMakeGeneratorKitConfigWidget::displayName() const
{
    return tr("CMake generator:");
}

void CMakeGeneratorKitConfigWidget::makeReadOnly()
{
    m_changeButton->setEnabled(false);
}

void CMakeGeneratorKitConfigWidget::refresh()
{
    if (m_ignoreChange)
        return;

    CMakeTool *const tool = CMakeKitInformation::cmakeTool(m_kit);
    if (tool != m_currentTool)
        m_currentTool = tool;

    m_changeButton->setEnabled(m_currentTool);
    const QString generator = CMakeGeneratorKitInformation::generator(kit());
    const QString extraGenerator = CMakeGeneratorKitInformation::extraGenerator(kit());
    const QString platform = CMakeGeneratorKitInformation::platform(kit());
    const QString toolset = CMakeGeneratorKitInformation::toolset(kit());

    const QString message = tr("%1 - %2, Platform: %3, Toolset: %4")
            .arg(extraGenerator.isEmpty() ? tr("<none>") : extraGenerator)
            .arg(generator.isEmpty() ? tr("<none>") : generator)
            .arg(platform.isEmpty() ? tr("<none>") : platform)
            .arg(toolset.isEmpty() ? tr("<none>") : toolset);
    m_label->setText(message);
}

QWidget *CMakeGeneratorKitConfigWidget::mainWidget() const
{
    return m_label;
}

QWidget *CMakeGeneratorKitConfigWidget::buttonWidget() const
{
    return m_changeButton;
}

QString CMakeGeneratorKitConfigWidget::toolTip() const
{
    return tr("CMake generator defines how a project is built when using CMake.<br>"
              "This setting is ignored when using other build systems.");
}

void CMakeGeneratorKitConfigWidget::changeGenerator()
{
    QPointer<QDialog> changeDialog = new QDialog(m_changeButton);
    changeDialog->setWindowTitle(tr("CMake Generator"));

    auto *layout = new QGridLayout(changeDialog);

    auto *cmakeLabel = new QLabel;

    auto *generatorCombo = new QComboBox;
    auto *extraGeneratorCombo = new QComboBox;
    auto *platformEdit = new QLineEdit;
    auto *toolsetEdit = new QLineEdit;

    int row = 0;
    layout->addWidget(new QLabel(QLatin1String("Executable:")));
    layout->addWidget(cmakeLabel, row, 1);

    ++row;
    layout->addWidget(new QLabel(tr("Generator:")), row, 0);
    layout->addWidget(generatorCombo, row, 1);

    ++row;
    layout->addWidget(new QLabel(tr("Extra generator:")), row, 0);
    layout->addWidget(extraGeneratorCombo, row, 1);

    ++row;
    layout->addWidget(new QLabel(tr("Platform:")), row, 0);
    layout->addWidget(platformEdit, row, 1);

    ++row;
    layout->addWidget(new QLabel(tr("Toolset:")), row, 0);
    layout->addWidget(toolsetEdit, row, 1);

    ++row;
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    layout->addWidget(bb, row, 0, 1, 2);

    connect(bb, &QDialogButtonBox::accepted, changeDialog.data(), &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, changeDialog.data(), &QDialog::reject);

    cmakeLabel->setText(m_currentTool->cmakeExecutable().toUserOutput());

    QList<CMakeTool::Generator> generatorList = m_currentTool->supportedGenerators();
    Utils::sort(generatorList, &CMakeTool::Generator::name);

    for (auto it = generatorList.constBegin(); it != generatorList.constEnd(); ++it)
        generatorCombo->addItem(it->name);

    auto updateDialog = [this, &generatorList, generatorCombo, extraGeneratorCombo,
            platformEdit, toolsetEdit](const QString &name) {
        auto it = std::find_if(generatorList.constBegin(), generatorList.constEnd(),
                               [name](const CMakeTool::Generator &g) { return g.name == name; });
        QTC_ASSERT(it != generatorList.constEnd(), return);
        generatorCombo->setCurrentText(name);

        extraGeneratorCombo->clear();
        extraGeneratorCombo->addItem(tr("<none>"), QString());
        foreach (const QString &eg, it->extraGenerators)
            extraGeneratorCombo->addItem(eg, eg);
        extraGeneratorCombo->setEnabled(extraGeneratorCombo->count() > 1);

        platformEdit->setEnabled(it->supportsPlatform);
        toolsetEdit->setEnabled(it->supportsToolset);
    };

    updateDialog(CMakeGeneratorKitInformation::generator(kit()));

    generatorCombo->setCurrentText(CMakeGeneratorKitInformation::generator(kit()));
    extraGeneratorCombo->setCurrentText(CMakeGeneratorKitInformation::extraGenerator(kit()));
    platformEdit->setText(platformEdit->isEnabled() ? CMakeGeneratorKitInformation::platform(kit()) : QLatin1String("<unsupported>"));
    toolsetEdit->setText(toolsetEdit->isEnabled() ? CMakeGeneratorKitInformation::toolset(kit()) : QLatin1String("<unsupported>"));

    connect(generatorCombo, &QComboBox::currentTextChanged, updateDialog);

    if (changeDialog->exec() == QDialog::Accepted) {
        if (!changeDialog)
            return;

        CMakeGeneratorKitInformation::set(kit(), generatorCombo->currentText(),
                                          extraGeneratorCombo->currentData().toString(),
                                          platformEdit->isEnabled() ? platformEdit->text() : QString(),
                                          toolsetEdit->isEnabled() ? toolsetEdit->text() : QString());
    }
}

// --------------------------------------------------------------------
// CMakeConfigurationKitConfigWidget:
// --------------------------------------------------------------------

CMakeConfigurationKitConfigWidget::CMakeConfigurationKitConfigWidget(Kit *kit,
                                                                     const KitInformation *ki) :
    KitConfigWidget(kit, ki),
    m_summaryLabel(new Utils::ElidingLabel),
    m_manageButton(new QPushButton)
{
    refresh();
    m_manageButton->setText(tr("Change..."));
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &CMakeConfigurationKitConfigWidget::editConfigurationChanges);
}

QString CMakeConfigurationKitConfigWidget::displayName() const
{
    return tr("CMake Configuration");
}

void CMakeConfigurationKitConfigWidget::makeReadOnly()
{
    m_manageButton->setEnabled(false);
    if (m_dialog)
        m_dialog->reject();
}

void CMakeConfigurationKitConfigWidget::refresh()
{
    const QStringList current = CMakeConfigurationKitInformation::toStringList(kit());

    m_summaryLabel->setText(current.join("; "));
    if (m_editor)
        m_editor->setPlainText(current.join('\n'));
}

QWidget *CMakeConfigurationKitConfigWidget::mainWidget() const
{
    return m_summaryLabel;
}

QWidget *CMakeConfigurationKitConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

QString CMakeConfigurationKitConfigWidget::toolTip() const
{
    return tr("Default configuration passed to CMake when setting up a project.");
}

void CMakeConfigurationKitConfigWidget::editConfigurationChanges()
{
    if (m_dialog) {
        m_dialog->activateWindow();
        m_dialog->raise();
        return;
    }

    QTC_ASSERT(!m_editor, return);

    m_dialog = new QDialog(m_summaryLabel->window());
    m_dialog->setWindowTitle(tr("Edit CMake Configuration"));
    auto layout = new QVBoxLayout(m_dialog);
    m_editor = new QPlainTextEdit;
    m_editor->setToolTip(tr("Enter one variable per line with the variable name "
                            "separated from the variable value by \"=\".<br>"
                            "You may provide a type hint by adding \":TYPE\" before the \"=\"."));
    m_editor->setMinimumSize(800, 200);

    auto chooser = new Core::VariableChooser(m_dialog);
    chooser->addSupportedWidget(m_editor);
    chooser->addMacroExpanderProvider([this]() { return kit()->macroExpander(); });

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Apply
                                        |QDialogButtonBox::Reset|QDialogButtonBox::Cancel);

    layout->addWidget(m_editor);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, m_dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, m_dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::clicked, m_dialog, [buttons, this](QAbstractButton *button) {
        if (button != buttons->button(QDialogButtonBox::Reset))
            return;
        CMakeConfigurationKitInformation::setConfiguration(kit(),
                                                           CMakeConfigurationKitInformation::defaultConfiguration(kit()));
    });
    connect(m_dialog, &QDialog::accepted, this, &CMakeConfigurationKitConfigWidget::acceptChangesDialog);
    connect(m_dialog, &QDialog::rejected, this, &CMakeConfigurationKitConfigWidget::closeChangesDialog);
    connect(buttons->button(QDialogButtonBox::Apply), &QAbstractButton::clicked,
            this, &CMakeConfigurationKitConfigWidget::applyChanges);

    refresh();
    m_dialog->show();
}

void CMakeConfigurationKitConfigWidget::applyChanges()
{
    QTC_ASSERT(m_editor, return);
    CMakeConfigurationKitInformation::fromStringList(kit(), m_editor->toPlainText().split(QLatin1Char('\n')));
}

void CMakeConfigurationKitConfigWidget::closeChangesDialog()
{
    m_dialog->deleteLater();
    m_dialog = nullptr;
    m_editor = nullptr;
}

void CMakeConfigurationKitConfigWidget::acceptChangesDialog()
{
    applyChanges();
    closeChangesDialog();
}

} // namespace Internal
} // namespace CMakeProjectManager
