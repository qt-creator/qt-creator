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
#include "cpptools_clangtidychecks.h"
#include "cpptoolsconstants.h"
#include "cpptoolsreuse.h"
#include "ui_clangdiagnosticconfigswidget.h"
#include "ui_clangbasechecks.h"
#include "ui_clazychecks.h"
#include "ui_tidychecks.h"

#include <projectexplorer/selectablefilesmodel.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QDebug>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QPushButton>
#include <QUuid>

namespace CppTools {

static constexpr const char CLANG_STATIC_ANALYZER_URL[]
    = "https://clang-analyzer.llvm.org/available_checks.html";

static void buildTree(ProjectExplorer::Tree *parent,
                      ProjectExplorer::Tree *current,
                      const Constants::TidyNode &node)
{
    current->name = QString::fromUtf8(node.name);
    current->isDir = node.children.size();
    if (parent) {
        current->fullPath = parent->fullPath + current->name;
        parent->childDirectories.push_back(current);
    } else {
        current->fullPath = Utils::FileName::fromString(current->name);
    }
    current->parent = parent;
    for (const Constants::TidyNode &nodeChild : node.children)
        buildTree(current, new ProjectExplorer::Tree, nodeChild);
}

static bool needsLink(ProjectExplorer::Tree *node) {
    if (node->name == "clang-analyzer-")
        return true;
    return !node->isDir && !node->fullPath.toString().startsWith("clang-analyzer-");
}

class TidyChecksTreeModel final : public ProjectExplorer::SelectableFilesModel
{
    Q_OBJECT

public:
    TidyChecksTreeModel()
        : ProjectExplorer::SelectableFilesModel(nullptr)
    {
        buildTree(nullptr, m_root, Constants::CLANG_TIDY_CHECKS_ROOT);
    }

    QString selectedChecks() const
    {
        QString checks;
        collectChecks(m_root, checks);
        return "-*" + checks;
    }

    void selectChecks(const QString &checks)
    {
        m_root->checked = Qt::Unchecked;
        propagateDown(index(0, 0, QModelIndex()));

        QStringList checksList = checks.simplified().remove(" ")
                .split(",", QString::SkipEmptyParts);

        for (QString &check : checksList) {
            Qt::CheckState state;
            if (check.startsWith("-")) {
                check = check.right(check.length() - 1);
                state = Qt::Unchecked;
            } else {
                state = Qt::Checked;
            }
            const QModelIndex index = indexForCheck(check);
            if (!index.isValid())
                continue;
            auto *node = static_cast<ProjectExplorer::Tree *>(index.internalPointer());
            node->checked = state;
            propagateUp(index);
            propagateDown(index);
        }
    }

    int columnCount(const QModelIndex &/*parent*/) const override
    {
        return 2;
    }

    QVariant data(const QModelIndex &fullIndex, int role = Qt::DisplayRole) const final
    {
        if (!fullIndex.isValid() || role == Qt::DecorationRole)
            return QVariant();
        QModelIndex index = this->index(fullIndex.row(), 0, fullIndex.parent());
        auto *node = static_cast<ProjectExplorer::Tree *>(index.internalPointer());

        if (fullIndex.column() == 1) {
            if (!needsLink(node))
                return QVariant();
            switch (role) {
            case Qt::DisplayRole:
                return tr("Web Page");
            case Qt::FontRole: {
                QFont font = QApplication::font();
                font.setUnderline(true);
                return font;
            }
            case Qt::ForegroundRole:
                return QApplication::palette().link().color();
            case Qt::UserRole: {
                // 'clang-analyzer-' group
                if (node->isDir)
                    return QString::fromUtf8(CLANG_STATIC_ANALYZER_URL);
                return QString::fromUtf8(Constants::TIDY_DOCUMENTATION_URL_TEMPLATE)
                        .arg(node->fullPath.toString());
            }
            }
            return QVariant();
        }

        if (role == Qt::DisplayRole)
            return node->isDir ? (node->name + "*") : node->name;

        return ProjectExplorer::SelectableFilesModel::data(index, role);
    }

    void setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (role == Qt::CheckStateRole && !m_enabled)
            return false;
        return ProjectExplorer::SelectableFilesModel::setData(index, value, role);
    }

private:

    // TODO: Remove/replace this method after base class refactoring is done.
    void traverse(const QModelIndex &index,
                  const std::function<bool(const QModelIndex &)> &visit) const
    {
        if (!index.isValid())
            return;

        if (!visit(index))
            return;

        if (!hasChildren(index))
            return;

        const int rows = rowCount(index);
        const int cols = columnCount(index);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j)
                traverse(this->index(i, j, index), visit);
        }
    }

    QModelIndex indexForCheck(const QString &check) const {
        if (check == "*")
            return index(0, 0, QModelIndex());

        QModelIndex result;
        traverse(index(0, 0, QModelIndex()), [&](const QModelIndex &index){
            using ProjectExplorer::Tree;
            if (result.isValid())
                return false;

            auto *node = static_cast<Tree *>(index.internalPointer());
            const QString nodeName = node->fullPath.toString();
            if ((check.endsWith("*") && nodeName.startsWith(check.left(check.length() - 1)))
                    || (!node->isDir && nodeName == check)) {
                result = index;
                return false;
            }

            return check.startsWith(nodeName);
        });
        return result;
    }

    static void collectChecks(const ProjectExplorer::Tree *root, QString &checks)
    {
        if (root->checked == Qt::Unchecked)
            return;
        if (root->checked == Qt::Checked) {
            checks += "," + root->fullPath.toString();
            if (root->isDir)
                checks += "*";
            return;
        }
        for (const ProjectExplorer::Tree *t : root->childDirectories)
            collectChecks(t, checks);
    }

    bool m_enabled = true;
};

ClangDiagnosticConfigsWidget::ClangDiagnosticConfigsWidget(const Core::Id &configToSelect,
                                                           QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ClangDiagnosticConfigsWidget)
    , m_diagnosticConfigsModel(codeModelSettings()->clangCustomDiagnosticConfigs())
    , m_tidyTreeModel(new TidyChecksTreeModel())
{
    m_ui->setupUi(this);
    setupTabs();

    m_selectedConfigIndex = m_diagnosticConfigsModel.indexOfConfig(
                codeModelSettings()->clangDiagnosticConfigId());

    connectConfigChooserCurrentIndex();
    connect(m_ui->copyButton, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsWidget::onCopyButtonClicked);
    connect(m_ui->removeButton, &QPushButton::clicked,
            this, &ClangDiagnosticConfigsWidget::onRemoveButtonClicked);
    connectDiagnosticOptionsChanged();

    connect(m_tidyChecks->checksPrefixesTree, &QTreeView::clicked,
            [this](const QModelIndex &index) {
        const QString link = m_tidyTreeModel->data(index, Qt::UserRole).toString();
        if (link.isEmpty())
            return;

        QDesktopServices::openUrl(QUrl(link));
    });

    syncWidgetsToModel(configToSelect);
}

ClangDiagnosticConfigsWidget::~ClangDiagnosticConfigsWidget()
{
    delete m_ui;
}

void ClangDiagnosticConfigsWidget::onCurrentConfigChanged(int index)
{
    m_selectedConfigIndex = index;
    syncOtherWidgetsToComboBox();
}

static ClangDiagnosticConfig createCustomConfig(const ClangDiagnosticConfig &config,
                                                const QString &displayName)
{
    ClangDiagnosticConfig copied = config;
    copied.setId(Core::Id::fromString(QUuid::createUuid().toString()));
    copied.setDisplayName(displayName);
    copied.setIsReadOnly(false);

    return copied;
}

void ClangDiagnosticConfigsWidget::onCopyButtonClicked()
{
    const ClangDiagnosticConfig &config = selectedConfig();

    bool diaglogAccepted = false;
    const QString newName = QInputDialog::getText(this,
                                                  tr("Copy Diagnostic Configuration"),
                                                  tr("Diagnostic configuration name:"),
                                                  QLineEdit::Normal,
                                                  tr("%1 (Copy)").arg(config.displayName()),
                                                  &diaglogAccepted);
    if (diaglogAccepted) {
        const ClangDiagnosticConfig customConfig = createCustomConfig(config, newName);
        m_diagnosticConfigsModel.appendOrUpdate(customConfig);
        emit customConfigsChanged(customConfigs());

        syncConfigChooserToModel(customConfig.id());
        m_clangBaseChecks->diagnosticOptionsTextEdit->setFocus();
    }
}

const ClangDiagnosticConfig &ClangDiagnosticConfigsWidget::selectedConfig() const
{
    return m_diagnosticConfigsModel.at(m_selectedConfigIndex);
}

Core::Id ClangDiagnosticConfigsWidget::selectedConfigId() const
{
    return selectedConfig().id();
}

void ClangDiagnosticConfigsWidget::onRemoveButtonClicked()
{
    m_diagnosticConfigsModel.removeConfigWithId(selectedConfigId());
    emit customConfigsChanged(customConfigs());

    syncConfigChooserToModel();
}

void ClangDiagnosticConfigsWidget::onClangTidyModeChanged(int index)
{
    ClangDiagnosticConfig config = selectedConfig();
    config.setClangTidyMode(static_cast<ClangDiagnosticConfig::TidyMode>(index));
    updateConfig(config);
    syncClangTidyWidgets(config);
}

void ClangDiagnosticConfigsWidget::onClangTidyTreeChanged()
{
    ClangDiagnosticConfig config = selectedConfig();
    config.setClangTidyChecks(m_tidyTreeModel->selectedChecks());
    updateConfig(config);
}

void ClangDiagnosticConfigsWidget::onClazyRadioButtonChanged(bool checked)
{
    if (!checked)
        return;

    QString checks;
    if (m_clazyChecks->clazyRadioDisabled->isChecked())
        checks = QString();
    else if (m_clazyChecks->clazyRadioLevel0->isChecked())
        checks = "level0";
    else if (m_clazyChecks->clazyRadioLevel1->isChecked())
        checks = "level1";
    else if (m_clazyChecks->clazyRadioLevel2->isChecked())
        checks = "level2";
    else if (m_clazyChecks->clazyRadioLevel3->isChecked())
        checks = "level3";

    ClangDiagnosticConfig config = selectedConfig();
    config.setClazyChecks(checks);
    updateConfig(config);
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
    return options.simplified().split(QLatin1Char(' '), QString::SkipEmptyParts);
}

void ClangDiagnosticConfigsWidget::onDiagnosticOptionsEdited()
{
    // Clean up input
    const QString diagnosticOptions = m_clangBaseChecks->diagnosticOptionsTextEdit->document()
                                          ->toPlainText();
    const QStringList normalizedOptions = normalizeDiagnosticInputOptions(diagnosticOptions);

    // Validate
    const QString errorMessage = validateDiagnosticOptions(normalizedOptions);
    updateValidityWidgets(errorMessage);
    if (!errorMessage.isEmpty()) {
        // Remember the entered options in case the user will switch back.
        m_notAcceptedOptions.insert(selectedConfigId(), diagnosticOptions);
        return;
    }
    m_notAcceptedOptions.remove(selectedConfigId());

    // Commit valid changes
    ClangDiagnosticConfig updatedConfig = selectedConfig();
    updatedConfig.setClangOptions(normalizedOptions);
    updateConfig(updatedConfig);
}

void ClangDiagnosticConfigsWidget::syncWidgetsToModel(const Core::Id &configToSelect)
{
    syncConfigChooserToModel(configToSelect);
    syncOtherWidgetsToComboBox();
}

void ClangDiagnosticConfigsWidget::syncConfigChooserToModel(const Core::Id &configToSelect)
{
    disconnectConfigChooserCurrentIndex();

    m_ui->configChooserList->clear();
    m_selectedConfigIndex = std::max(std::min(m_selectedConfigIndex,
                                              m_diagnosticConfigsModel.size() - 1),
                                     0);

    const int size = m_diagnosticConfigsModel.size();
    for (int i = 0; i < size; ++i) {
        const ClangDiagnosticConfig &config = m_diagnosticConfigsModel.at(i);
        const QString displayName
                = ClangDiagnosticConfigsModel::displayNameWithBuiltinIndication(config);
        m_ui->configChooserList->addItem(displayName);

        if (configToSelect == config.id())
            m_selectedConfigIndex = i;
    }

    connectConfigChooserCurrentIndex();

    m_ui->configChooserList->setCurrentRow(m_selectedConfigIndex);
}

void ClangDiagnosticConfigsWidget::syncOtherWidgetsToComboBox()
{
    if (isConfigChooserEmpty())
        return;

    const ClangDiagnosticConfig &config = selectedConfig();

    // Update main button row
    m_ui->removeButton->setEnabled(!config.isReadOnly());

    // Update Text Edit
    const QString options = m_notAcceptedOptions.contains(config.id())
            ? m_notAcceptedOptions.value(config.id())
            : config.clangOptions().join(QLatin1Char(' '));
    setDiagnosticOptions(options);
    m_clangBaseChecksWidget->setEnabled(!config.isReadOnly());

    if (config.isReadOnly()) {
        m_ui->infoIcon->setPixmap(Utils::Icons::INFO.pixmap());
        m_ui->infoLabel->setText(tr("Copy this configuration to customize it."));
        m_ui->infoLabel->setStyleSheet(QString());
    }

    syncClangTidyWidgets(config);
    syncClazyWidgets(config);
}

void ClangDiagnosticConfigsWidget::syncClangTidyWidgets(const ClangDiagnosticConfig &config)
{
    disconnectClangTidyItemChanged();

    ClangDiagnosticConfig::TidyMode tidyMode = config.clangTidyMode();

    m_tidyChecks->tidyMode->setCurrentIndex(static_cast<int>(tidyMode));
    switch (tidyMode) {
    case ClangDiagnosticConfig::TidyMode::Disabled:
    case ClangDiagnosticConfig::TidyMode::File:
        m_tidyChecks->plainTextEditButton->setVisible(false);
        m_tidyChecks->checksListWrapper->setCurrentIndex(1);
        break;
    case ClangDiagnosticConfig::TidyMode::ChecksPrefixList:
        m_tidyChecks->plainTextEditButton->setVisible(true);
        m_tidyChecks->checksListWrapper->setCurrentIndex(0);
        syncTidyChecksToTree(config);
        break;
    }

    const bool enabled = !config.isReadOnly();
    m_tidyChecks->tidyMode->setEnabled(enabled);
    m_tidyChecks->plainTextEditButton->setText(enabled ? tr("Edit Checks as String...")
                                                       : tr("View Checks as String..."));
    m_tidyTreeModel->setEnabled(enabled);
    connectClangTidyItemChanged();
}

void ClangDiagnosticConfigsWidget::syncTidyChecksToTree(const ClangDiagnosticConfig &config)
{
    m_tidyTreeModel->selectChecks(config.clangTidyChecks());
}

void ClangDiagnosticConfigsWidget::syncClazyWidgets(const ClangDiagnosticConfig &config)
{
    const QString clazyChecks = config.clazyChecks();

    QRadioButton *button = m_clazyChecks->clazyRadioDisabled;
    if (clazyChecks.isEmpty())
        button = m_clazyChecks->clazyRadioDisabled;
    else if (clazyChecks == "level0")
        button = m_clazyChecks->clazyRadioLevel0;
    else if (clazyChecks == "level1")
        button = m_clazyChecks->clazyRadioLevel1;
    else if (clazyChecks == "level2")
        button = m_clazyChecks->clazyRadioLevel2;
    else if (clazyChecks == "level3")
        button = m_clazyChecks->clazyRadioLevel3;

    button->setChecked(true);
    m_clazyChecksWidget->setEnabled(!config.isReadOnly());
}

void ClangDiagnosticConfigsWidget::updateConfig(const ClangDiagnosticConfig &config)
{
    m_diagnosticConfigsModel.appendOrUpdate(config);
    emit customConfigsChanged(customConfigs());
}

bool ClangDiagnosticConfigsWidget::isConfigChooserEmpty() const
{
    return m_ui->configChooserList->count() == 0;
}

void ClangDiagnosticConfigsWidget::setDiagnosticOptions(const QString &options)
{
    if (options != m_clangBaseChecks->diagnosticOptionsTextEdit->document()->toPlainText()) {
        disconnectDiagnosticOptionsChanged();
        m_clangBaseChecks->diagnosticOptionsTextEdit->document()->setPlainText(options);
        connectDiagnosticOptionsChanged();
    }

    const QString errorMessage
            = validateDiagnosticOptions(normalizeDiagnosticInputOptions(options));
    updateValidityWidgets(errorMessage);
}

void ClangDiagnosticConfigsWidget::updateValidityWidgets(const QString &errorMessage)
{
    QString validationResult;
    const Utils::Icon *icon = nullptr;
    QString styleSheet;
    if (errorMessage.isEmpty()) {
        icon = &Utils::Icons::INFO;
        validationResult = tr("Configuration passes sanity checks.");
    } else {
        icon = &Utils::Icons::CRITICAL;
        validationResult = tr("%1").arg(errorMessage);
        styleSheet = "color: red;";
    }

    m_ui->infoIcon->setPixmap(icon->pixmap());
    m_ui->infoLabel->setText(validationResult);
    m_ui->infoLabel->setStyleSheet(styleSheet);
}

void ClangDiagnosticConfigsWidget::connectClangTidyItemChanged()
{
    connect(m_tidyChecks->tidyMode,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &ClangDiagnosticConfigsWidget::onClangTidyModeChanged);
    connect(m_tidyTreeModel.get(), &TidyChecksTreeModel::dataChanged,
            this, &ClangDiagnosticConfigsWidget::onClangTidyTreeChanged);
}

void ClangDiagnosticConfigsWidget::disconnectClangTidyItemChanged()
{
    disconnect(m_tidyChecks->tidyMode,
               static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
               this,
               &ClangDiagnosticConfigsWidget::onClangTidyModeChanged);
    disconnect(m_tidyTreeModel.get(), &TidyChecksTreeModel::dataChanged,
               this, &ClangDiagnosticConfigsWidget::onClangTidyTreeChanged);
}

void ClangDiagnosticConfigsWidget::connectClazyRadioButtonClicked(QRadioButton *button)
{
    connect(button,
            &QRadioButton::clicked,
            this,
            &ClangDiagnosticConfigsWidget::onClazyRadioButtonChanged);
}

void ClangDiagnosticConfigsWidget::connectConfigChooserCurrentIndex()
{
    connect(m_ui->configChooserList, &QListWidget::currentRowChanged,
            this, &ClangDiagnosticConfigsWidget::onCurrentConfigChanged);
}

void ClangDiagnosticConfigsWidget::disconnectConfigChooserCurrentIndex()
{
    disconnect(m_ui->configChooserList, &QListWidget::currentRowChanged,
               this, &ClangDiagnosticConfigsWidget::onCurrentConfigChanged);
}

void ClangDiagnosticConfigsWidget::connectDiagnosticOptionsChanged()
{
    connect(m_clangBaseChecks->diagnosticOptionsTextEdit->document(),
            &QTextDocument::contentsChanged,
            this,
            &ClangDiagnosticConfigsWidget::onDiagnosticOptionsEdited);
}

void ClangDiagnosticConfigsWidget::disconnectDiagnosticOptionsChanged()
{
    disconnect(m_clangBaseChecks->diagnosticOptionsTextEdit->document(),
               &QTextDocument::contentsChanged,
               this,
               &ClangDiagnosticConfigsWidget::onDiagnosticOptionsEdited);
}

ClangDiagnosticConfigs ClangDiagnosticConfigsWidget::customConfigs() const
{
    const ClangDiagnosticConfigs allConfigs = m_diagnosticConfigsModel.configs();

    return Utils::filtered(allConfigs, [](const ClangDiagnosticConfig &config){
        return !config.isReadOnly();
    });
}

void ClangDiagnosticConfigsWidget::setupTabs()
{
    m_clangBaseChecks.reset(new CppTools::Ui::ClangBaseChecks);
    m_clangBaseChecksWidget = new QWidget();
    m_clangBaseChecks->setupUi(m_clangBaseChecksWidget);

    m_clazyChecks.reset(new CppTools::Ui::ClazyChecks);
    m_clazyChecksWidget = new QWidget();
    m_clazyChecks->setupUi(m_clazyChecksWidget);

    connectClazyRadioButtonClicked(m_clazyChecks->clazyRadioDisabled);
    connectClazyRadioButtonClicked(m_clazyChecks->clazyRadioLevel0);
    connectClazyRadioButtonClicked(m_clazyChecks->clazyRadioLevel1);
    connectClazyRadioButtonClicked(m_clazyChecks->clazyRadioLevel2);
    connectClazyRadioButtonClicked(m_clazyChecks->clazyRadioLevel3);

    m_tidyChecks.reset(new CppTools::Ui::TidyChecks);
    m_tidyChecksWidget = new QWidget();
    m_tidyChecks->setupUi(m_tidyChecksWidget);
    m_tidyChecks->checksPrefixesTree->setModel(m_tidyTreeModel.get());
    m_tidyChecks->checksPrefixesTree->expandToDepth(0);
    m_tidyChecks->checksPrefixesTree->header()->setStretchLastSection(false);
    m_tidyChecks->checksPrefixesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    connect(m_tidyChecks->plainTextEditButton, &QPushButton::clicked, this, [this]() {
        const bool readOnly = selectedConfig().isReadOnly();

        QDialog dialog;
        dialog.setWindowTitle(tr("Checks"));
        dialog.setLayout(new QVBoxLayout);
        auto *textEdit = new QTextEdit(&dialog);
        textEdit->setReadOnly(readOnly);
        dialog.layout()->addWidget(textEdit);
        auto *buttonsBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                | (readOnly ? QDialogButtonBox::NoButton
                                                            : QDialogButtonBox::Cancel));
        dialog.layout()->addWidget(buttonsBox);
        QObject::connect(buttonsBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(buttonsBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        const QString initialChecks = m_tidyTreeModel->selectedChecks();
        textEdit->setPlainText(initialChecks);

        QObject::connect(&dialog, &QDialog::accepted, [=, &initialChecks]() {
            const QString updatedChecks = textEdit->toPlainText();
            if (updatedChecks == initialChecks)
                return;

            disconnectClangTidyItemChanged();

            // Also throws away invalid options.
            m_tidyTreeModel->selectChecks(updatedChecks);
            onClangTidyTreeChanged();

            connectClangTidyItemChanged();
        });
        dialog.exec();
    });

    connectClangTidyItemChanged();

    m_ui->tabWidget->addTab(m_clangBaseChecksWidget, tr("Clang"));
    m_ui->tabWidget->addTab(m_tidyChecksWidget, tr("Clang-Tidy"));
    m_ui->tabWidget->addTab(m_clazyChecksWidget, tr("Clazy"));
    m_ui->tabWidget->setCurrentIndex(0);
}

} // CppTools namespace

#include "clangdiagnosticconfigswidget.moc"
