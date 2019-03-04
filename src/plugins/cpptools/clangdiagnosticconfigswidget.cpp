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
#include "cpptools_clazychecks.h"
#include "cpptoolsconstants.h"
#include "cpptoolsreuse.h"
#include "ui_clangdiagnosticconfigswidget.h"
#include "ui_clangbasechecks.h"
#include "ui_clazychecks.h"
#include "ui_tidychecks.h"

#include <projectexplorer/selectablefilesmodel.h>

#include <utils/algorithm.h>
#include <utils/executeondestruction.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QDebug>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QUuid>

#include <memory>

namespace CppTools {

using namespace Constants;

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

static void selectAll(QAbstractItemView *view)
{
    view->setSelectionMode(QAbstractItemView::MultiSelection);
    view->selectAll();
    view->setSelectionMode(QAbstractItemView::SingleSelection);
}

class BaseChecksTreeModel : public ProjectExplorer::SelectableFilesModel
{
    Q_OBJECT

public:
    enum Roles { LinkRole = Qt::UserRole + 1 };
    enum Columns { NameColumn, LinkColumn };

    BaseChecksTreeModel()
        : ProjectExplorer::SelectableFilesModel(nullptr)
    {}

    int columnCount(const QModelIndex &) const override { return 2; }

    QVariant data(const QModelIndex &fullIndex, int role = Qt::DisplayRole) const override
    {
        if (fullIndex.column() == LinkColumn) {
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
            }
            return QVariant();
        }
        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (role == Qt::CheckStateRole && !m_enabled)
            return false;
        ProjectExplorer::SelectableFilesModel::setData(index, value, role);
        return true;
    }

    void setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

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

protected:
    bool m_enabled = true;
};

static void openUrl(QAbstractItemModel *model, const QModelIndex &index)
{
    const QString link = model->data(index, BaseChecksTreeModel::LinkRole).toString();
    if (link.isEmpty())
        return;

    QDesktopServices::openUrl(QUrl(link));
};

class TidyChecksTreeModel final : public BaseChecksTreeModel
{
    Q_OBJECT

public:
    TidyChecksTreeModel()
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

private:
    QVariant data(const QModelIndex &fullIndex, int role = Qt::DisplayRole) const final
    {
        if (!fullIndex.isValid() || role == Qt::DecorationRole)
            return QVariant();
        QModelIndex index = this->index(fullIndex.row(), 0, fullIndex.parent());
        auto *node = static_cast<ProjectExplorer::Tree *>(index.internalPointer());

        if (fullIndex.column() == 1) {
            if (!needsLink(node))
                return QVariant();

            if (role == LinkRole) {
                // 'clang-analyzer-' group
                if (node->isDir)
                    return QString::fromUtf8(CLANG_STATIC_ANALYZER_URL);
                return QString::fromUtf8(Constants::TIDY_DOCUMENTATION_URL_TEMPLATE)
                        .arg(node->fullPath.toString());
            }

            return BaseChecksTreeModel::data(fullIndex, role);
        }

        if (role == Qt::DisplayRole)
            return node->isDir ? (node->name + "*") : node->name;

        return ProjectExplorer::SelectableFilesModel::data(index, role);
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
};

class ClazyChecksTree : public ProjectExplorer::Tree
{
public:
    enum Kind { TopLevelNode, LevelNode, CheckNode };
    ClazyChecksTree(const QString &name, Kind kind)
    {
        this->name = name;
        this->kind = kind;
        this->isDir = kind == TopLevelNode || kind == LevelNode;
    }

    static ClazyChecksTree *fromIndex(const QModelIndex &index)
    {
        return static_cast<ClazyChecksTree *>(index.internalPointer());
    }

public:
    ClazyCheckInfo checkInfo;
    Kind kind = TopLevelNode;
};

class ClazyChecksTreeModel final : public BaseChecksTreeModel
{
    Q_OBJECT

public:
    ClazyChecksTreeModel() { buildTree(); }

    QStringList enabledChecks() const
    {
        QStringList checks;
        collectChecks(m_root, checks);
        return checks;
    }

    void enableChecks(const QStringList &checks)
    {
        // Unselect all
        m_root->checked = Qt::Unchecked;
        propagateDown(index(0, 0, QModelIndex()));

        for (const QString &check : checks) {
            const QModelIndex index = indexForCheck(check);
            if (!index.isValid())
                continue;
            ClazyChecksTree::fromIndex(index)->checked = Qt::Checked;
            propagateUp(index);
            propagateDown(index);
        }
    }

    bool hasEnabledButNotVisibleChecks(
        const std::function<bool(const QModelIndex &index)> &isHidden) const
    {
        bool enabled = false;
        traverse(index(0, 0, QModelIndex()), [&](const QModelIndex &index){
            if (enabled)
                return false;
            const auto *node = ClazyChecksTree::fromIndex(index);
            if (node->kind == ClazyChecksTree::CheckNode && index.column() == NameColumn) {
                const bool isChecked = data(index, Qt::CheckStateRole).toInt() == Qt::Checked;
                const bool isVisible = isHidden(index);
                if (isChecked && isVisible) {
                    enabled = true;
                    return false;
                }
            }
            return true;
        });

        return enabled;
    }

    bool enableLowerLevels() const { return m_enableLowerLevels; }
    void setEnableLowerLevels(bool enable) { m_enableLowerLevels = enable; }

    QSet<QString> topics() const { return m_topics; }

private:
    void buildTree()
    {
        // Top level node
        m_root = new ClazyChecksTree("*", ClazyChecksTree::TopLevelNode);

        for (const ClazyCheckInfo &check : CLAZY_CHECKS) {
            // Level node
            ClazyChecksTree *&levelNode = m_levelNodes[check.level];
            if (!levelNode) {
                levelNode = new ClazyChecksTree(levelDescription(check.level), ClazyChecksTree::LevelNode);
                levelNode->parent = m_root;
                levelNode->checkInfo.level = check.level; // Pass on the level for sorting
                m_root->childDirectories << levelNode;
            }

            // Check node
            auto checkNode = new ClazyChecksTree(check.name, ClazyChecksTree::CheckNode);
            checkNode->parent = levelNode;
            checkNode->checkInfo = check;

            levelNode->childDirectories.append(checkNode);

            m_topics.unite(check.topics.toSet());
        }
    }

    QVariant data(const QModelIndex &fullIndex, int role = Qt::DisplayRole) const final
    {
        if (!fullIndex.isValid() || role == Qt::DecorationRole)
            return QVariant();
        const QModelIndex index = this->index(fullIndex.row(), 0, fullIndex.parent());
        const auto *node = ClazyChecksTree::fromIndex(index);

        if (fullIndex.column() == LinkColumn) {
            if (role == LinkRole) {
                if (node->checkInfo.name.isEmpty())
                    return QVariant();
                return QString::fromUtf8(Constants::CLAZY_DOCUMENTATION_URL_TEMPLATE).arg(node->name);
            }
            if (role == Qt::DisplayRole && node->kind != ClazyChecksTree::CheckNode)
                return QVariant();

            return BaseChecksTreeModel::data(fullIndex, role);
        }

        if (role == Qt::DisplayRole)
            return node->name;

        return ProjectExplorer::SelectableFilesModel::data(index, role);
    }

    static QString levelDescription(int level)
    {
        switch (level) {
        case -1:
            return tr("Manual Level: Very few false positives");
        case 0:
            return tr("Level 0: No false positives");
        case 1:
            return tr("Level 1: Very few false positives");
        case 2:
            return tr("Level 2: More false positives");
        case 3:
            return tr("Level 3: Experimental checks");
        default:
            QTC_CHECK(false && "No clazy level description");
            return tr("Level %1").arg(QString::number(level));
        }
    }

    QModelIndex indexForCheck(const QString &check) const {
        if (check == "*")
            return index(0, 0, QModelIndex());

        QModelIndex result;
        traverse(index(0, 0, QModelIndex()), [&](const QModelIndex &index){
            if (result.isValid())
                return false;
            const auto *node = ClazyChecksTree::fromIndex(index);
            if (node->kind == ClazyChecksTree::CheckNode && node->checkInfo.name == check) {
                result = index;
                return false;
            }
            return true;
        });
        return result;
    }

    QModelIndex indexForTree(const ClazyChecksTree *tree) const {
        if (!tree)
            return {};

        QModelIndex result;
        traverse(index(0, 0, QModelIndex()), [&](const QModelIndex &index){
            if (result.isValid())
                return false;
            if (index.internalPointer() == tree) {
                result = index;
                return false;
            }
            return true;
        });
        return result;
    }

    static void collectChecks(const ProjectExplorer::Tree *root, QStringList &checks)
    {
        if (root->checked == Qt::Unchecked)
            return;
        if (root->checked == Qt::Checked && !root->isDir) {
            checks.append(root->name);
            return;
        }
        for (const ProjectExplorer::Tree *t : root->childDirectories)
            collectChecks(t, checks);
    }

    static QStringList toStringList(const QVariantList &variantList)
    {
        QStringList list;
        for (auto &item : variantList)
            list.append(item.toString());
        return list;
    }

private:
    QHash<int, ClazyChecksTree *> m_levelNodes;
    QSet<QString> m_topics;
    bool m_enableLowerLevels = true;
};

class ClazyChecksSortFilterModel : public QSortFilterProxyModel
{
public:
    ClazyChecksSortFilterModel(QObject *parent)
        : QSortFilterProxyModel(parent)
    {}

    void setTopics(const QStringList &value)
    {
        m_topics = value;
        invalidateFilter();
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!index.isValid())
            return false;

        const auto *node = ClazyChecksTree::fromIndex(index);
        if (node->kind == ClazyChecksTree::CheckNode) {
            const QStringList topics = node->checkInfo.topics;
            return Utils::anyOf(m_topics, [topics](const QString &topic) {
                return topics.contains(topic);
            });
        }

        return true;
    }

private:
    // Note that sort order of levels is important for "enableLowerLevels" mode, see setData().
    bool lessThan(const QModelIndex &l, const QModelIndex &r) const override
    {
        const int leftLevel = adaptLevel(ClazyChecksTree::fromIndex(l)->checkInfo.level);
        const int rightLevel = adaptLevel(ClazyChecksTree::fromIndex(r)->checkInfo.level);

        if (leftLevel == rightLevel)
            return sourceModel()->data(l).toString() < sourceModel()->data(r).toString();
        return leftLevel < rightLevel;
    }

    static int adaptLevel(int level)
    {
        if (level == -1) // "Manual Level"
            return 1000;
        return level;
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (!index.isValid())
            return false;

        if (role == Qt::CheckStateRole
              && static_cast<ClazyChecksTreeModel *>(sourceModel())->enableLowerLevels()
              && QSortFilterProxyModel::setData(index, value, role)) {
            const auto *node = ClazyChecksTree::fromIndex(mapToSource(index));
            if (node->kind == ClazyChecksTree::LevelNode && node->checkInfo.level >= 0) {
                // Rely on the sort order to find the lower level index/node
                const auto previousIndex = this->index(index.row() - 1,
                                                       index.column(),
                                                       index.parent());
                if (previousIndex.isValid()
                    && ClazyChecksTree::fromIndex(mapToSource(previousIndex))->checkInfo.level
                           >= 0) {
                    setData(previousIndex, value, role);
                }
            }
        }

        return QSortFilterProxyModel::setData(index, value, role);
    }

private:
    QStringList m_topics;
};

ClangDiagnosticConfigsWidget::ClangDiagnosticConfigsWidget(const Core::Id &configToSelect,
                                                           QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ClangDiagnosticConfigsWidget)
    , m_diagnosticConfigsModel(codeModelSettings()->clangCustomDiagnosticConfigs())
    , m_clazyTreeModel(new ClazyChecksTreeModel())
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
    connectClangOnlyOptionsChanged();

    connect(m_tidyChecks->checksPrefixesTree,
            &QTreeView::clicked,
            [model = m_tidyTreeModel.get()](const QModelIndex &index) { openUrl(model, index); });
    connect(m_clazyChecks->checksView,
            &QTreeView::clicked,
            [model = m_clazySortFilterProxyModel](const QModelIndex &index) { openUrl(model, index); });

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
        const ClangDiagnosticConfig customConfig
            = ClangDiagnosticConfigsModel::createCustomConfig(config, newName);
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

void ClangDiagnosticConfigsWidget::onClazyTreeChanged()
{
    syncClazyChecksGroupBox();

    ClangDiagnosticConfig config = selectedConfig();
    config.setClazyChecks(m_clazyTreeModel->enabledChecks().join(","));
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
        m_notAcceptedOptions.insert(selectedConfigId(), diagnosticOptions);
        return;
    }
    m_notAcceptedOptions.remove(selectedConfigId());

    // Commit valid changes
    ClangDiagnosticConfig updatedConfig = selectedConfig();
    updatedConfig.setClangOptions(normalizedOptions);
    updatedConfig.setUseBuildSystemWarnings(useBuildSystemWarnings);
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

    disconnectClangOnlyOptionsChanged();
    Utils::ExecuteOnDestruction e([this]() { connectClangOnlyOptionsChanged(); });

    const ClangDiagnosticConfig &config = selectedConfig();

    // Update main button row
    m_ui->removeButton->setEnabled(!config.isReadOnly());

    // Update check box
    m_clangBaseChecks->useFlagsFromBuildSystemCheckBox->setChecked(config.useBuildSystemWarnings());

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
    disconnectClazyItemChanged();

    const QString clazyChecks = config.clazyChecks();

    m_clazyTreeModel->enableChecks(clazyChecks.split(',', QString::SkipEmptyParts));

    syncClazyChecksGroupBox();

    const bool enabled = !config.isReadOnly();
    m_clazyChecks->topicsResetButton->setEnabled(enabled);
    m_clazyChecks->enableLowerLevelsCheckBox->setEnabled(enabled);
    selectAll(m_clazyChecks->topicsView);
    m_clazyChecks->topicsView->setEnabled(enabled);
    m_clazyTreeModel->setEnabled(enabled);

    connectClazyItemChanged();
}

void ClangDiagnosticConfigsWidget::syncClazyChecksGroupBox()
{
    const auto isHidden = [this](const QModelIndex &index) {
        return !m_clazySortFilterProxyModel->filterAcceptsRow(index.row(), index.parent());
    };
    const bool hasEnabledButHidden = m_clazyTreeModel->hasEnabledButNotVisibleChecks(isHidden);
    const int checksCount = m_clazyTreeModel->enabledChecks().count();
    const QString title = hasEnabledButHidden ? tr("Checks (%n enabled, some are filtered out)",
                                                   nullptr, checksCount)
                                              : tr("Checks (%n enabled)", nullptr, checksCount);
    m_clazyChecks->checksGroupBox->setTitle(title);
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
    if (options != m_clangBaseChecks->diagnosticOptionsTextEdit->document()->toPlainText())
        m_clangBaseChecks->diagnosticOptionsTextEdit->document()->setPlainText(options);

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

void ClangDiagnosticConfigsWidget::connectClazyItemChanged()
{
    connect(m_clazyTreeModel.get(), &ClazyChecksTreeModel::dataChanged,
            this, &ClangDiagnosticConfigsWidget::onClazyTreeChanged);
}

void ClangDiagnosticConfigsWidget::disconnectClazyItemChanged()
{
    disconnect(m_clazyTreeModel.get(), &ClazyChecksTreeModel::dataChanged,
               this, &ClangDiagnosticConfigsWidget::onClazyTreeChanged);
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

ClangDiagnosticConfigs ClangDiagnosticConfigsWidget::customConfigs() const
{
    return m_diagnosticConfigsModel.customConfigs();
}

static void setupTreeView(QTreeView *view, QAbstractItemModel *model, int expandToDepth = 0)
{
    view->setModel(model);
    view->expandToDepth(expandToDepth);
    view->header()->setStretchLastSection(false);
    view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    view->setHeaderHidden(true);
}

void ClangDiagnosticConfigsWidget::setupTabs()
{
    m_clangBaseChecks = std::make_unique<CppTools::Ui::ClangBaseChecks>();
    m_clangBaseChecksWidget = new QWidget();
    m_clangBaseChecks->setupUi(m_clangBaseChecksWidget);

    m_clazyChecks = std::make_unique<CppTools::Ui::ClazyChecks>();
    m_clazyChecksWidget = new QWidget();
    m_clazyChecks->setupUi(m_clazyChecksWidget);
    m_clazySortFilterProxyModel = new ClazyChecksSortFilterModel(this);
    m_clazySortFilterProxyModel->setSourceModel(m_clazyTreeModel.get());
    setupTreeView(m_clazyChecks->checksView, m_clazySortFilterProxyModel, 2);
    m_clazyChecks->checksView->setSortingEnabled(true);
    m_clazyChecks->checksView->sortByColumn(0, Qt::AscendingOrder);
    auto topicsModel = new QStringListModel(m_clazyTreeModel->topics().toList(), this);
    topicsModel->sort(0);
    m_clazyChecks->topicsView->setModel(topicsModel);
    connect(m_clazyChecks->topicsResetButton, &QPushButton::clicked, [this](){
        selectAll(m_clazyChecks->topicsView);
    });
    connect(m_clazyChecks->topicsView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            [this, topicsModel](const QItemSelection &, const QItemSelection &) {
                const auto indexes = m_clazyChecks->topicsView->selectionModel()->selectedIndexes();
                const QStringList topics
                    = Utils::transform(indexes, [topicsModel](const QModelIndex &index) {
                          return topicsModel->data(index).toString();
                      });
                m_clazySortFilterProxyModel->setTopics(topics);
                this->syncClazyChecksGroupBox();
            });

    selectAll(m_clazyChecks->topicsView);
    connect(m_clazyChecks->enableLowerLevelsCheckBox, &QCheckBox::stateChanged, [this](int) {
        const bool enable = m_clazyChecks->enableLowerLevelsCheckBox->isChecked();
        m_clazyTreeModel->setEnableLowerLevels(enable);
        codeModelSettings()->setEnableLowerClazyLevels(
            m_clazyChecks->enableLowerLevelsCheckBox->isChecked());
    });
    const Qt::CheckState checkEnableLowerClazyLevels
        = codeModelSettings()->enableLowerClazyLevels() ? Qt::Checked : Qt::Unchecked;
    m_clazyChecks->enableLowerLevelsCheckBox->setCheckState(checkEnableLowerClazyLevels);

    m_tidyChecks = std::make_unique<CppTools::Ui::TidyChecks>();
    m_tidyChecksWidget = new QWidget();
    m_tidyChecks->setupUi(m_tidyChecksWidget);
    setupTreeView(m_tidyChecks->checksPrefixesTree, m_tidyTreeModel.get());

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
    connectClazyItemChanged();

    m_ui->tabWidget->addTab(m_clangBaseChecksWidget, tr("Clang"));
    m_ui->tabWidget->addTab(m_tidyChecksWidget, tr("Clang-Tidy"));
    m_ui->tabWidget->addTab(m_clazyChecksWidget, tr("Clazy"));
    m_ui->tabWidget->setCurrentIndex(0);
}

} // CppTools namespace

#include "clangdiagnosticconfigswidget.moc"
