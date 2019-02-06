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
// CMakeKitAspectWidget:
// --------------------------------------------------------------------

CMakeKitAspectWidget::CMakeKitAspectWidget(Kit *kit,
                                           const KitAspect *ki) :
    KitAspectWidget(kit, ki),
    m_comboBox(new QComboBox),
    m_manageButton(new QPushButton(KitAspectWidget::msgManage()))
{
    m_comboBox->setSizePolicy(QSizePolicy::Ignored, m_comboBox->sizePolicy().verticalPolicy());
    m_comboBox->setEnabled(false);
    m_comboBox->setToolTip(toolTip());

    foreach (CMakeTool *tool, CMakeToolManager::cmakeTools())
        cmakeToolAdded(tool->id());

    updateComboBox();

    refresh();
    connect(m_comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &CMakeKitAspectWidget::currentCMakeToolChanged);

    m_manageButton->setContentsMargins(0, 0, 0, 0);
    connect(m_manageButton, &QPushButton::clicked,
            this, &CMakeKitAspectWidget::manageCMakeTools);

    CMakeToolManager *cmakeMgr = CMakeToolManager::instance();
    connect(cmakeMgr, &CMakeToolManager::cmakeAdded,
            this, &CMakeKitAspectWidget::cmakeToolAdded);
    connect(cmakeMgr, &CMakeToolManager::cmakeRemoved,
            this, &CMakeKitAspectWidget::cmakeToolRemoved);
    connect(cmakeMgr, &CMakeToolManager::cmakeUpdated,
            this, &CMakeKitAspectWidget::cmakeToolUpdated);
}

CMakeKitAspectWidget::~CMakeKitAspectWidget()
{
    delete m_comboBox;
    delete m_manageButton;
}

QString CMakeKitAspectWidget::displayName() const
{
    return tr("CMake Tool");
}

void CMakeKitAspectWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

void CMakeKitAspectWidget::refresh()
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(m_kit);
    m_comboBox->setCurrentIndex(tool ? indexOf(tool->id()) : -1);
}

QWidget *CMakeKitAspectWidget::mainWidget() const
{
    return m_comboBox;
}

QWidget *CMakeKitAspectWidget::buttonWidget() const
{
    return m_manageButton;
}

QString CMakeKitAspectWidget::toolTip() const
{
    return tr("The CMake Tool to use when building a project with CMake.<br>"
              "This setting is ignored when using other build systems.");
}

int CMakeKitAspectWidget::indexOf(const Core::Id &id)
{
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (id == Core::Id::fromSetting(m_comboBox->itemData(i)))
            return i;
    }
    return -1;
}

void CMakeKitAspectWidget::cmakeToolAdded(const Core::Id &id)
{
    const CMakeTool *tool = CMakeToolManager::findById(id);
    QTC_ASSERT(tool, return);

    m_comboBox->addItem(tool->displayName(), tool->id().toSetting());
    updateComboBox();
    refresh();
}

void CMakeKitAspectWidget::cmakeToolUpdated(const Core::Id &id)
{
    const int pos = indexOf(id);
    QTC_ASSERT(pos >= 0, return);

    const CMakeTool *tool = CMakeToolManager::findById(id);
    QTC_ASSERT(tool, return);

    m_comboBox->setItemText(pos, tool->displayName());
}

void CMakeKitAspectWidget::cmakeToolRemoved(const Core::Id &id)
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

void CMakeKitAspectWidget::updateComboBox()
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

void CMakeKitAspectWidget::currentCMakeToolChanged(int index)
{
    if (m_removingItem)
        return;

    const Core::Id id = Core::Id::fromSetting(m_comboBox->itemData(index));
    CMakeKitAspect::setCMakeTool(m_kit, id);
}

void CMakeKitAspectWidget::manageCMakeTools()
{
    Core::ICore::showOptionsDialog(Constants::CMAKE_SETTINGSPAGE_ID,
                                   buttonWidget());
}

// --------------------------------------------------------------------
// CMakeGeneratorKitAspectWidget:
// --------------------------------------------------------------------


CMakeGeneratorKitAspectWidget::CMakeGeneratorKitAspectWidget(Kit *kit,
                                                             const KitAspect *ki) :
    KitAspectWidget(kit, ki),
    m_label(new QLabel),
    m_changeButton(new QPushButton)
{
    m_label->setToolTip(toolTip());
    m_changeButton->setText(tr("Change..."));

    refresh();
    connect(m_changeButton, &QPushButton::clicked,
            this, &CMakeGeneratorKitAspectWidget::changeGenerator);
}

CMakeGeneratorKitAspectWidget::~CMakeGeneratorKitAspectWidget()
{
    delete m_label;
    delete m_changeButton;
}

QString CMakeGeneratorKitAspectWidget::displayName() const
{
    return tr("CMake generator");
}

void CMakeGeneratorKitAspectWidget::makeReadOnly()
{
    m_changeButton->setEnabled(false);
}

void CMakeGeneratorKitAspectWidget::refresh()
{
    if (m_ignoreChange)
        return;

    CMakeTool *const tool = CMakeKitAspect::cmakeTool(m_kit);
    if (tool != m_currentTool)
        m_currentTool = tool;

    m_changeButton->setEnabled(m_currentTool);
    const QString generator = CMakeGeneratorKitAspect::generator(kit());
    const QString extraGenerator = CMakeGeneratorKitAspect::extraGenerator(kit());
    const QString platform = CMakeGeneratorKitAspect::platform(kit());
    const QString toolset = CMakeGeneratorKitAspect::toolset(kit());

    const QString message = tr("%1 - %2, Platform: %3, Toolset: %4")
            .arg(extraGenerator.isEmpty() ? tr("<none>") : extraGenerator)
            .arg(generator.isEmpty() ? tr("<none>") : generator)
            .arg(platform.isEmpty() ? tr("<none>") : platform)
            .arg(toolset.isEmpty() ? tr("<none>") : toolset);
    m_label->setText(message);
}

QWidget *CMakeGeneratorKitAspectWidget::mainWidget() const
{
    return m_label;
}

QWidget *CMakeGeneratorKitAspectWidget::buttonWidget() const
{
    return m_changeButton;
}

QString CMakeGeneratorKitAspectWidget::toolTip() const
{
    return tr("CMake generator defines how a project is built when using CMake.<br>"
              "This setting is ignored when using other build systems.");
}

void CMakeGeneratorKitAspectWidget::changeGenerator()
{
    QPointer<QDialog> changeDialog = new QDialog(m_changeButton);

    // Disable help button in titlebar on windows:
    Qt::WindowFlags flags = changeDialog->windowFlags();
    flags &= ~Qt::WindowContextHelpButtonHint;
    flags |= Qt::MSWindowsFixedSizeDialogHint;
    changeDialog->setWindowFlags(flags);

    changeDialog->setWindowTitle(tr("CMake Generator"));

    auto *layout = new QGridLayout(changeDialog);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    auto *cmakeLabel = new QLabel;
    cmakeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

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

    auto updateDialog = [&generatorList, generatorCombo, extraGeneratorCombo,
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

    updateDialog(CMakeGeneratorKitAspect::generator(kit()));

    generatorCombo->setCurrentText(CMakeGeneratorKitAspect::generator(kit()));
    extraGeneratorCombo->setCurrentText(CMakeGeneratorKitAspect::extraGenerator(kit()));
    platformEdit->setText(platformEdit->isEnabled() ? CMakeGeneratorKitAspect::platform(kit()) : QLatin1String("<unsupported>"));
    toolsetEdit->setText(toolsetEdit->isEnabled() ? CMakeGeneratorKitAspect::toolset(kit()) : QLatin1String("<unsupported>"));

    connect(generatorCombo, &QComboBox::currentTextChanged, updateDialog);

    if (changeDialog->exec() == QDialog::Accepted) {
        if (!changeDialog)
            return;

        CMakeGeneratorKitAspect::set(kit(), generatorCombo->currentText(),
                                          extraGeneratorCombo->currentData().toString(),
                                          platformEdit->isEnabled() ? platformEdit->text() : QString(),
                                          toolsetEdit->isEnabled() ? toolsetEdit->text() : QString());
    }
}

// --------------------------------------------------------------------
// CMakeConfigurationKitAspectWidget:
// --------------------------------------------------------------------

CMakeConfigurationKitAspectWidget::CMakeConfigurationKitAspectWidget(Kit *kit,
                                                                     const KitAspect *ki) :
    KitAspectWidget(kit, ki),
    m_summaryLabel(new Utils::ElidingLabel),
    m_manageButton(new QPushButton)
{
    refresh();
    m_manageButton->setText(tr("Change..."));
    connect(m_manageButton, &QAbstractButton::clicked,
            this, &CMakeConfigurationKitAspectWidget::editConfigurationChanges);
}

QString CMakeConfigurationKitAspectWidget::displayName() const
{
    return tr("CMake Configuration");
}

void CMakeConfigurationKitAspectWidget::makeReadOnly()
{
    m_manageButton->setEnabled(false);
    if (m_dialog)
        m_dialog->reject();
}

void CMakeConfigurationKitAspectWidget::refresh()
{
    const QStringList current = CMakeConfigurationKitAspect::toStringList(kit());

    m_summaryLabel->setText(current.join("; "));
    if (m_editor)
        m_editor->setPlainText(current.join('\n'));
}

QWidget *CMakeConfigurationKitAspectWidget::mainWidget() const
{
    return m_summaryLabel;
}

QWidget *CMakeConfigurationKitAspectWidget::buttonWidget() const
{
    return m_manageButton;
}

QString CMakeConfigurationKitAspectWidget::toolTip() const
{
    return tr("Default configuration passed to CMake when setting up a project.");
}

void CMakeConfigurationKitAspectWidget::editConfigurationChanges()
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
        CMakeConfigurationKitAspect::setConfiguration(kit(),
                                                           CMakeConfigurationKitAspect::defaultConfiguration(kit()));
    });
    connect(m_dialog, &QDialog::accepted, this, &CMakeConfigurationKitAspectWidget::acceptChangesDialog);
    connect(m_dialog, &QDialog::rejected, this, &CMakeConfigurationKitAspectWidget::closeChangesDialog);
    connect(buttons->button(QDialogButtonBox::Apply), &QAbstractButton::clicked,
            this, &CMakeConfigurationKitAspectWidget::applyChanges);

    refresh();
    m_dialog->show();
}

void CMakeConfigurationKitAspectWidget::applyChanges()
{
    QTC_ASSERT(m_editor, return);
    CMakeConfigurationKitAspect::fromStringList(kit(), m_editor->toPlainText().split(QLatin1Char('\n')));
}

void CMakeConfigurationKitAspectWidget::closeChangesDialog()
{
    m_dialog->deleteLater();
    m_dialog = nullptr;
    m_editor = nullptr;
}

void CMakeConfigurationKitAspectWidget::acceptChangesDialog()
{
    applyChanges();
    closeChangesDialog();
}

} // namespace Internal
} // namespace CMakeProjectManager
