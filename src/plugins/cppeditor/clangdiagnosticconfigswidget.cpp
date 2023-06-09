// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdiagnosticconfigswidget.h"

#include "clangdiagnosticconfigsmodel.h"
#include "cppeditortr.h"
#include "wrappablelineedit.h"

#include <utils/environment.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/stringutils.h>
#include <utils/treemodel.h>

#include <QApplication>
#include <QCheckBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QPushButton>
#include <QScopeGuard>
#include <QTabWidget>
#include <QTreeView>

using namespace Utils;

namespace CppEditor {

class ConfigNode : public TreeItem
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

class GroupNode : public StaticTreeItem
{
public:
    GroupNode(const QString &text)
        : StaticTreeItem(text)
    {}

    Qt::ItemFlags flags(int) const final { return {}; }
    QVariant data(int column, int role) const final
    {
        if (role == Qt::ForegroundRole) {
            // Avoid disabled color.
            return QApplication::palette().color(QPalette::ColorGroup::Normal,
                                                 QPalette::ColorRole::Text);
        }
        return StaticTreeItem::data(column, role);
    }
};

class ConfigsModel : public TreeModel<TreeItem, GroupNode, ConfigNode>
{
public:
    ConfigsModel(const ClangDiagnosticConfigs &configs)
    {
        m_builtinRoot = new GroupNode(Tr::tr("Built-in"));
        m_customRoot = new GroupNode(Tr::tr("Custom"));
        rootItem()->appendChild(m_builtinRoot);
        rootItem()->appendChild(m_customRoot);

        for (const ClangDiagnosticConfig &config : configs) {
            TreeItem *parent = config.isReadOnly() ? m_builtinRoot : m_customRoot;
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

    void removeConfig(const Id &id)
    {
       ConfigNode *node = itemForConfigId(id);
       node->parent()->removeChildAt(node->indexInParent());
    }

    ConfigNode *itemForConfigId(const Id &id) const
    {
        return findItemAtLevel<2>([id](const ConfigNode *node) {
            return node->config.id() == id;
        });
    }

private:
    TreeItem *m_builtinRoot = nullptr;
    TreeItem *m_customRoot = nullptr;
};

class ClangBaseChecksWidget : public QWidget
{
public:
    ClangBaseChecksWidget()
    {
        auto label = new QLabel;
        label->setTextFormat(Qt::MarkdownText);
        label->setText(Tr::tr("For appropriate options, consult the GCC or Clang manual "
                              "pages or the [GCC online documentation](%1).")
                           .arg("https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html"));
        label->setOpenExternalLinks(true);

        useFlagsFromBuildSystemCheckBox = new QCheckBox(Tr::tr("Use diagnostic flags from build system"));

        diagnosticOptionsTextEdit = new WrappableLineEdit;

        using namespace Layouting;

        Column {
            label,
            useFlagsFromBuildSystemCheckBox,
            diagnosticOptionsTextEdit
        }.attachTo(this);
    }

    QCheckBox *useFlagsFromBuildSystemCheckBox;
    WrappableLineEdit *diagnosticOptionsTextEdit;
};

ClangDiagnosticConfigsWidget::ClangDiagnosticConfigsWidget(const ClangDiagnosticConfigs &configs,
                                                           const Id &configToSelect,
                                                           QWidget *parent)
    : QWidget(parent)
    , m_configsModel(new ConfigsModel(configs))
{
    auto copyButton = new QPushButton(Tr::tr("Copy..."));
    m_renameButton = new QPushButton(Tr::tr("Rename..."));
    m_removeButton = new QPushButton(Tr::tr("Remove"));

    m_infoLabel = new InfoLabel;

    m_configsView = new QTreeView;
    m_configsView->setHeaderHidden(true);
    m_configsView->setUniformRowHeights(true);
    m_configsView->setRootIsDecorated(false);
    m_configsView->setModel(m_configsModel);
    m_configsView->setCurrentIndex(m_configsModel->itemForConfigId(configToSelect)->index());
    m_configsView->setItemsExpandable(false);
    m_configsView->expandAll();

    m_clangBaseChecks = new ClangBaseChecksWidget;

    m_tabWidget = new QTabWidget;
    m_tabWidget->addTab(m_clangBaseChecks, Tr::tr("Clang Warnings"));

    using namespace Layouting;

    Column {
        Row {
            m_configsView,
            Column {
                copyButton,
                m_renameButton,
                m_removeButton,
                st
            }
        },
        m_infoLabel,
        m_tabWidget
    }.attachTo(this);

    connect(copyButton, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsWidget::onCopyButtonClicked);
    connect(m_renameButton, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsWidget::onRenameButtonClicked);
    connect(m_removeButton, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsWidget::onRemoveButtonClicked);
    connect(m_configsView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ClangDiagnosticConfigsWidget::sync);

    connectClangOnlyOptionsChanged();
}

ClangDiagnosticConfigsWidget::~ClangDiagnosticConfigsWidget() = default;

void ClangDiagnosticConfigsWidget::onCopyButtonClicked()
{
    const ClangDiagnosticConfig &config = currentConfig();
    bool dialogAccepted = false;
    const QString newName = QInputDialog::getText(this,
                                                  Tr::tr("Copy Diagnostic Configuration"),
                                                  Tr::tr("Diagnostic configuration name:"),
                                                  QLineEdit::Normal,
                                                  Tr::tr("%1 (Copy)").arg(config.displayName()),
                                                  &dialogAccepted);
    if (dialogAccepted) {
        const ClangDiagnosticConfig customConfig
            = ClangDiagnosticConfigsModel::createCustomConfig(config, newName);

        m_configsModel->appendCustomConfig(customConfig);
        m_configsView->setCurrentIndex(
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
                                                  Tr::tr("Rename Diagnostic Configuration"),
                                                  Tr::tr("New name:"),
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
    TreeItem *item = m_configsModel->itemForIndex(m_configsView->currentIndex());
    return static_cast<ConfigNode *>(item)->config;
}

void ClangDiagnosticConfigsWidget::onRemoveButtonClicked()
{
    const Id configToRemove = currentConfig().id();
    if (m_configsModel->customConfigsCount() == 1)
        m_configsView->setCurrentIndex(m_configsModel->fallbackConfigIndex());
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
    if (Utils::qtcEnvironmentVariableIntValue("QTC_CLANG_NO_DIAGNOSTIC_CHECK"))
        return QString();

    for (const QString &option : options) {
        if (!isValidOption(option))
            return Tr::tr("Option \"%1\" is invalid.").arg(option);
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
    if (!m_configsView->currentIndex().isValid())
        return;

    disconnectClangOnlyOptionsChanged();
    const QScopeGuard cleanup([this] { connectClangOnlyOptionsChanged(); });

    // Update main button row
    const ClangDiagnosticConfig &config = currentConfig();
    m_removeButton->setEnabled(!config.isReadOnly());
    m_renameButton->setEnabled(!config.isReadOnly());

    // Update check box
    m_clangBaseChecks->useFlagsFromBuildSystemCheckBox->setChecked(config.useBuildSystemWarnings());

    // Update Text Edit
    const QString options = m_notAcceptedOptions.contains(config.id())
            ? m_notAcceptedOptions.value(config.id())
            : config.clangOptions().join(QLatin1Char(' '));
    setDiagnosticOptions(options);
    m_clangBaseChecks->setEnabled(!config.isReadOnly());

    if (config.isReadOnly()) {
        m_infoLabel->setType(InfoLabel::Information);
        m_infoLabel->setText(Tr::tr("Copy this configuration to customize it."));
        m_infoLabel->setFilled(false);
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
        m_infoLabel->setType(InfoLabel::Information);
        m_infoLabel->setText(Tr::tr("Configuration passes sanity checks."));
        m_infoLabel->setFilled(false);
    } else {
        m_infoLabel->setType(InfoLabel::Error);
        m_infoLabel->setText(errorMessage);
        m_infoLabel->setFilled(true);
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
    disconnect(m_clangBaseChecks->useFlagsFromBuildSystemCheckBox, &QCheckBox::stateChanged,
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
    return m_tabWidget;
}

} // CppEditor
