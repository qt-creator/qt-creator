/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangdiagnosticconfigswidget.h"

#include "cppcodemodelsettings.h"
#include "ui_clangdiagnosticconfigswidget.h"
#include "ui_clangbasechecks.h"

#include <utils/executeondestruction.h>
#include <utils/stringutils.h>
#include <utils/treemodel.h>

#include <QInputDialog>
#include <QPushButton>

namespace CppTools {

class ConfigNode : public Utils::TreeItem
{
public:
    ConfigNode(const ClangDiagnosticConfig &config)
        : config(config)
    {}

    QVariant data(int, int role) const override
    {
        if (role == Qt::DisplayRole)
            return config.displayName();
        return QVariant();
    }

    ClangDiagnosticConfig config;
};

class GroupNode : public Utils::StaticTreeItem
{
public:
    GroupNode(const QString &text)
        : Utils::StaticTreeItem(text)
    {}

    Qt::ItemFlags flags(int) const { return {}; }
    QVariant data(int column, int role) const
    {
        if (role == Qt::ForegroundRole) {
            // Avoid disabled color.
            return QApplication::palette().color(QPalette::ColorGroup::Normal,
                                                 QPalette::ColorRole::Text);
        }
        return Utils::StaticTreeItem::data(column, role);
    }
};

class ConfigsModel : public Utils::TreeModel<Utils::TreeItem, GroupNode, ConfigNode>
{
    Q_OBJECT

public:
    ConfigsModel(const ClangDiagnosticConfigs &configs)
    {
        m_builtinRoot = new GroupNode(tr("Built-in"));
        m_customRoot = new GroupNode(tr("Custom"));
        rootItem()->appendChild(m_builtinRoot);
        rootItem()->appendChild(m_customRoot);

        for (const ClangDiagnosticConfig &config : configs) {
            Utils::TreeItem *parent = config.isReadOnly() ? m_builtinRoot : m_customRoot;
            parent->appendChild(new ConfigNode(config));
        }
    }

    int customConfigsCount() const { return m_customRoot->childCount(); }
    QModelIndex fallbackConfigIndex() const { return m_builtinRoot->lastChild()->index(); }

    ClangDiagnosticConfigs configs() const
    {
        ClangDiagnosticConfigs configs;
        forItemsAtLevel<2>([&configs](const ConfigNode *node) {
            configs << node->config;
        });
        return configs;
    }

    void appendCustomConfig(const ClangDiagnosticConfig &config)
    {
        m_customRoot->appendChild(new ConfigNode(config));
    }

    void removeConfig(const Utils::Id &id)
    {
       ConfigNode *node = itemForConfigId(id);
       node->parent()->removeChildAt(node->indexInParent());
    }

    ConfigNode *itemForConfigId(const Utils::Id &id) const
    {
        return findItemAtLevel<2>([id](const ConfigNode *node) {
            return node->config.id() == id;
        });
    }

private:
    Utils::TreeItem *m_builtinRoot = nullptr;
    Utils::TreeItem *m_customRoot = nullptr;
};

ClangDiagnosticConfigsWidget::ClangDiagnosticConfigsWidget(const ClangDiagnosticConfigs &configs,
                                                           const Utils::Id &configToSelect,
                                                           QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ClangDiagnosticConfigsWidget)
    , m_configsModel(new ConfigsModel(configs))
{
    m_ui->setupUi(this);
    m_ui->configsView->setHeaderHidden(true);
    m_ui->configsView->setUniformRowHeights(true);
    m_ui->configsView->setRootIsDecorated(false);
    m_ui->configsView->setModel(m_configsModel);
    m_ui->configsView->setCurrentIndex(m_configsModel->itemForConfigId(configToSelect)->index());
    m_ui->configsView->setItemsExpandable(false);
    m_ui->configsView->expandAll();
    connect(m_ui->configsView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &ClangDiagnosticConfigsWidget::sync);

    m_clangBaseChecks = std::make_unique<CppTools::Ui::ClangBaseChecks>();
    m_clangBaseChecksWidget = new QWidget();
    m_clangBaseChecks->setupUi(m_clangBaseChecksWidget);

    m_ui->tabWidget->addTab(m_clangBaseChecksWidget, tr("Clang Warnings"));
    m_ui->tabWidget->setCurrentIndex(0);

    connect(m_ui->copyButton, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsWidget::onCopyButtonClicked);
    connect(m_ui->renameButton, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsWidget::onRenameButtonClicked);
    connect(m_ui->removeButton, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsWidget::onRemoveButtonClicked);
    connectClangOnlyOptionsChanged();
}

ClangDiagnosticConfigsWidget::~ClangDiagnosticConfigsWidget()
{
    delete m_ui;
}

void ClangDiagnosticConfigsWidget::onCopyButtonClicked()
{
    const ClangDiagnosticConfig &config = currentConfig();
    bool dialogAccepted = false;
    const QString newName = QInputDialog::getText(this,
                                                  tr("Copy Diagnostic Configuration"),
                                                  tr("Diagnostic configuration name:"),
                                                  QLineEdit::Normal,
                                                  tr("%1 (Copy)").arg(config.displayName()),
                                                  &dialogAccepted);
    if (dialogAccepted) {
        const ClangDiagnosticConfig customConfig
            = ClangDiagnosticConfigsModel::createCustomConfig(config, newName);

        m_configsModel->appendCustomConfig(customConfig);
        m_ui->configsView->setCurrentIndex(
            m_configsModel->itemForConfigId(customConfig.id())->index());
        sync();
        m_clangBaseChecks->diagnosticOptionsTextEdit->setFocus();
    }
}

void ClangDiagnosticConfigsWidget::onRenameButtonClicked()
{
    const ClangDiagnosticConfig &config = currentConfig();

    bool dialogAccepted = false;
    const QString newName = QInputDialog::getText(this,
                                                  tr("Rename Diagnostic Configuration"),
                                                  tr("New name:"),
                                                  QLineEdit::Normal,
                                                  config.displayName(),
                                                  &dialogAccepted);
    if (dialogAccepted) {
        ConfigNode *configNode = m_configsModel->itemForConfigId(config.id());
        configNode->config.setDisplayName(newName);
    }
}

const ClangDiagnosticConfig ClangDiagnosticConfigsWidget::currentConfig() const
{
    Utils::TreeItem *item = m_configsModel->itemForIndex(m_ui->configsView->currentIndex());
    return static_cast<ConfigNode *>(item)->config;
}

void ClangDiagnosticConfigsWidget::onRemoveButtonClicked()
{
    const Utils::Id configToRemove = currentConfig().id();
    if (m_configsModel->customConfigsCount() == 1)
        m_ui->configsView->setCurrentIndex(m_configsModel->fallbackConfigIndex());
    m_configsModel->removeConfig(configToRemove);
    sync();
}

static bool isAcceptedWarningOption(const QString &option)
{
    return option == "-w"
        || option == "-pedantic"
        || option == "-pedantic-errors";
}

// Reference:
// https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
// https://clang.llvm.org/docs/DiagnosticsReference.html
static bool isValidOption(const QString &option)
{
    if (option == "-Werror")
        return false; // Avoid errors due to unknown or misspelled warnings.
    return option.startsWith("-W") || isAcceptedWarningOption(option);
}

static QString validateDiagnosticOptions(const QStringList &options)
{
    // This is handy for testing, allow disabling validation.
    if (qEnvironmentVariableIntValue("QTC_CLANG_NO_DIAGNOSTIC_CHECK"))
        return QString();

    for (const QString &option : options) {
        if (!isValidOption(option))
            return ClangDiagnosticConfigsWidget::tr("Option \"%1\" is invalid.").arg(option);
    }

    return QString();
}

static QStringList normalizeDiagnosticInputOptions(const QString &options)
{
    return options.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

void ClangDiagnosticConfigsWidget::onClangOnlyOptionsChanged()
{
    const bool useBuildSystemWarnings = m_clangBaseChecks->useFlagsFromBuildSystemCheckBox
                                            ->isChecked();

    // Clean up options input
    const QString diagnosticOptions = m_clangBaseChecks->diagnosticOptionsTextEdit->document()
                                          ->toPlainText();
    const QStringList normalizedOptions = normalizeDiagnosticInputOptions(diagnosticOptions);

    // Validate options input
    const QString errorMessage = validateDiagnosticOptions(normalizedOptions);
    updateValidityWidgets(errorMessage);
    if (!errorMessage.isEmpty()) {
        // Remember the entered options in case the user will switch back.
        m_notAcceptedOptions.insert(currentConfig().id(), diagnosticOptions);
        return;
    }
    m_notAcceptedOptions.remove(currentConfig().id());

    // Commit valid changes
    ClangDiagnosticConfig updatedConfig = currentConfig();
    updatedConfig.setClangOptions(normalizedOptions);
    updatedConfig.setUseBuildSystemWarnings(useBuildSystemWarnings);
    updateConfig(updatedConfig);
}

void ClangDiagnosticConfigsWidget::sync()
{
    if (!m_ui->configsView->currentIndex().isValid())
        return;

    disconnectClangOnlyOptionsChanged();
    Utils::ExecuteOnDestruction e([this]() { connectClangOnlyOptionsChanged(); });

    // Update main button row
    const ClangDiagnosticConfig &config = currentConfig();
    m_ui->removeButton->setEnabled(!config.isReadOnly());
    m_ui->renameButton->setEnabled(!config.isReadOnly());

    // Update check box
    m_clangBaseChecks->useFlagsFromBuildSystemCheckBox->setChecked(config.useBuildSystemWarnings());

    // Update Text Edit
    const QString options = m_notAcceptedOptions.contains(config.id())
            ? m_notAcceptedOptions.value(config.id())
            : config.clangOptions().join(QLatin1Char(' '));
    setDiagnosticOptions(options);
    m_clangBaseChecksWidget->setEnabled(!config.isReadOnly());

    if (config.isReadOnly()) {
        m_ui->infoLabel->setType(Utils::InfoLabel::Information);
        m_ui->infoLabel->setText(tr("Copy this configuration to customize it."));
        m_ui->infoLabel->setFilled(false);
    }

    syncExtraWidgets(config);
}

void ClangDiagnosticConfigsWidget::updateConfig(const ClangDiagnosticConfig &config)
{
    m_configsModel->itemForConfigId(config.id())->config = config;
}

void ClangDiagnosticConfigsWidget::setDiagnosticOptions(const QString &options)
{
    if (options != m_clangBaseChecks->diagnosticOptionsTextEdit->document()->toPlainText())
        m_clangBaseChecks->diagnosticOptionsTextEdit->document()->setPlainText(options);

    const QString errorMessage
            = validateDiagnosticOptions(normalizeDiagnosticInputOptions(options));
    updateValidityWidgets(errorMessage);
}

void ClangDiagnosticConfigsWidget::updateValidityWidgets(const QString &errorMessage)
{
    if (errorMessage.isEmpty()) {
        m_ui->infoLabel->setType(Utils::InfoLabel::Information);
        m_ui->infoLabel->setText(tr("Configuration passes sanity checks."));
        m_ui->infoLabel->setFilled(false);
    } else {
        m_ui->infoLabel->setType(Utils::InfoLabel::Error);
        m_ui->infoLabel->setText(tr("%1").arg(errorMessage));
        m_ui->infoLabel->setFilled(true);
    }
}

void ClangDiagnosticConfigsWidget::connectClangOnlyOptionsChanged()
{
    connect(m_clangBaseChecks->useFlagsFromBuildSystemCheckBox,
            &QCheckBox::stateChanged,
            this,
            &ClangDiagnosticConfigsWidget::onClangOnlyOptionsChanged);
    connect(m_clangBaseChecks->diagnosticOptionsTextEdit->document(),
            &QTextDocument::contentsChanged,
            this,
            &ClangDiagnosticConfigsWidget::onClangOnlyOptionsChanged);
}

void ClangDiagnosticConfigsWidget::disconnectClangOnlyOptionsChanged()
{
    disconnect(m_clangBaseChecks->useFlagsFromBuildSystemCheckBox,
               &QCheckBox::stateChanged,
               this,
               &ClangDiagnosticConfigsWidget::onClangOnlyOptionsChanged);
    disconnect(m_clangBaseChecks->diagnosticOptionsTextEdit->document(),
               &QTextDocument::contentsChanged,
               this,
               &ClangDiagnosticConfigsWidget::onClangOnlyOptionsChanged);
}

ClangDiagnosticConfigs ClangDiagnosticConfigsWidget::configs() const
{
    return m_configsModel->configs();
}

QTabWidget *ClangDiagnosticConfigsWidget::tabWidget() const
{
    return m_ui->tabWidget;
}

} // CppTools namespace

#include "clangdiagnosticconfigswidget.moc"
