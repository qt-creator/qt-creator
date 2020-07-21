/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "diagnosticconfigswidget.h"

#include "ui_clazychecks.h"
#include "ui_tidychecks.h"

#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpptoolsreuse.h>
#include <projectexplorer/selectablefilesmodel.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QSortFilterProxyModel>
#include <QStringListModel>

using namespace CppTools;

namespace ClangTools {
namespace Internal {

namespace ClangTidyPrefixTree {

class Node
{
public:
    Node() = default;
    Node(const QString &name, const QVector<Node> &children = QVector<Node>())
        : name(name)
        , children(children)
    {}

    static Node fromCheckList(const QStringList &checks);

    QString name;
    QVector<Node> children;
};

class PrefixGroupIterator
{
public:
    // Assumes sorted checks.
    PrefixGroupIterator(const QStringList &checks, const QChar &itemSeparator)
        : m_checks(checks)
        , m_itemSeparator(itemSeparator)
    {}

    QStringList next()
    {
        m_groupPrefix.clear();

        int i = m_index;
        for (; i < m_checks.size(); ++i) {
            const QString item = m_checks[i];
            const QString prefix = itemPrefix(item);

            if (m_groupPrefix.isEmpty()) {
                if (prefix.isEmpty()) {
                    m_index = i + 1;
                    return {item};
                } else {
                    m_groupPrefix = prefix;
                }
            } else {
                if (!prefix.isEmpty() && prefix == groupPrefix())
                    continue;
                return groupUntil(i - 1);
            }
        }

        return groupUntil(i);
    }

    QString groupPrefix() const { return m_groupPrefix; }

private:
    QString itemPrefix(const QString &item) const
    {
        const int separatorIndex = item.indexOf(m_itemSeparator);
        if (separatorIndex != -1)
            return item.mid(0, separatorIndex);
        return {};
    }

    QStringList groupUntil(int i)
    {
        const QStringList result = m_checks.mid(m_index, i - m_index + 1);
        m_index = i + 1;
        return result;
    }

    QStringList m_checks;
    QString m_groupPrefix;
    QChar m_itemSeparator;
    int m_index = 0;
};

static QStringList groupWithPrefixRemoved(const QStringList &group, const QString &prefix)
{
    return Utils::transform(group, [&](const QString &item) { return item.mid(prefix.size() + 1); });
}

static Node groupToNode(const QString &nodeName,
                        const QString &fullNodeName,
                        const QStringList &checks,
                        int uncompressedLevels)
{
    // The clang (static) analyzer items are further separated by '.' instead of '-'.
    const QChar nodeNameSeparator = fullNodeName.startsWith("clang-analyzer-") ? '.' : '-';
    const QChar itemSeparator = fullNodeName.startsWith("clang-analyzer") ? '.' : '-';

    Node node = nodeName;
    if (!nodeName.isEmpty())
        node.name += nodeNameSeparator;

    // Iterate through prefix groups and add child nodes recursively
    PrefixGroupIterator it(checks, itemSeparator);
    for (QStringList group = it.next(); !group.isEmpty(); group = it.next()) {
        const QString groupPrefix = it.groupPrefix();
        const QString newFullNodeName = fullNodeName.isEmpty()
                                            ? groupPrefix
                                            : fullNodeName + nodeNameSeparator + groupPrefix;
        const Node childNode = groupPrefix.isEmpty()
                                   ? Node(group.first(), {})
                                   : groupToNode(groupPrefix,
                                                 newFullNodeName,
                                                 groupWithPrefixRemoved(group, groupPrefix),
                                                 std::max(uncompressedLevels - 1, 0));
        node.children << childNode;
    }

    // Eliminate pointless chains of single items
    while (!uncompressedLevels && node.children.size() == 1) {
        node.name = node.name + node.children[0].name;
        node.children = node.children[0].children;
    }

    return node;
}

Node Node::fromCheckList(const QStringList &allChecks)
{
    QStringList sortedChecks = allChecks;
    sortedChecks.sort();

    return groupToNode("", "", sortedChecks, 2);
}

} // namespace ClangTidyPrefixTree

static void buildTree(ProjectExplorer::Tree *parent,
                      ProjectExplorer::Tree *current,
                      const ClangTidyPrefixTree::Node &node)
{
    current->name = node.name;
    current->isDir = node.children.size();
    if (parent) {
        current->fullPath = parent->fullPath + current->name;
        parent->childDirectories.push_back(current);
    } else {
        current->fullPath = Utils::FilePath::fromString(current->name);
    }
    current->parent = parent;
    for (const ClangTidyPrefixTree::Node &nodeChild : node.children)
        buildTree(current, new ProjectExplorer::Tree, nodeChild);
}

static bool needsLink(ProjectExplorer::Tree *node) {
    if (node->fullPath.toString() == "clang-analyzer-")
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
    TidyChecksTreeModel(const QStringList &supportedChecks)
    {
        buildTree(nullptr, m_root, ClangTidyPrefixTree::Node::fromCheckList(supportedChecks));
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
                .split(",", Qt::SkipEmptyParts);

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

            if (role == LinkRole || role == Qt::ToolTipRole) {
                // 'clang-analyzer-' group
                if (node->isDir)
                    return CppTools::Constants::CLANG_STATIC_ANALYZER_DOCUMENTATION_URL;
                return QString::fromUtf8(CppTools::Constants::TIDY_DOCUMENTATION_URL_TEMPLATE)
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
    ClazyCheck check;
    Kind kind = TopLevelNode;
};

class ClazyChecksTreeModel final : public BaseChecksTreeModel
{
    Q_OBJECT

public:
    ClazyChecksTreeModel(const ClazyChecks &supportedClazyChecks)
    {
        // Top level node
        m_root = new ClazyChecksTree("*", ClazyChecksTree::TopLevelNode);

        for (const ClazyCheck &check : supportedClazyChecks) {
            // Level node
            ClazyChecksTree *&levelNode = m_levelNodes[check.level];
            if (!levelNode) {
                levelNode = new ClazyChecksTree(levelDescription(check.level),
                                                ClazyChecksTree::LevelNode);
                levelNode->parent = m_root;
                levelNode->check.level = check.level; // Pass on the level for sorting
                m_root->childDirectories << levelNode;
            }

            // Check node
            auto checkNode = new ClazyChecksTree(check.name, ClazyChecksTree::CheckNode);
            checkNode->parent = levelNode;
            checkNode->check = check;

            levelNode->childDirectories.append(checkNode);

            m_topics.unite(Utils::toSet(check.topics));
        }
    }

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
    QVariant data(const QModelIndex &fullIndex, int role = Qt::DisplayRole) const final
    {
        if (!fullIndex.isValid() || role == Qt::DecorationRole)
            return QVariant();
        const QModelIndex index = this->index(fullIndex.row(), 0, fullIndex.parent());
        const auto *node = ClazyChecksTree::fromIndex(index);

        if (fullIndex.column() == LinkColumn) {
            if (role == LinkRole || role == Qt::ToolTipRole) {
                if (node->check.name.isEmpty())
                    return QVariant();
                return QString::fromUtf8(CppTools::Constants::CLAZY_DOCUMENTATION_URL_TEMPLATE)
                    .arg(node->name);
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
            if (node->kind == ClazyChecksTree::CheckNode && node->check.name == check) {
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
            const QStringList topics = node->check.topics;
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
        const int leftLevel = adaptLevel(ClazyChecksTree::fromIndex(l)->check.level);
        const int rightLevel = adaptLevel(ClazyChecksTree::fromIndex(r)->check.level);

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
            if (node->kind == ClazyChecksTree::LevelNode && node->check.level >= 0) {
                // Rely on the sort order to find the lower level index/node
                const auto previousIndex = this->index(index.row() - 1,
                                                       index.column(),
                                                       index.parent());
                if (previousIndex.isValid()
                    && ClazyChecksTree::fromIndex(mapToSource(previousIndex))->check.level
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

static void setupTreeView(QTreeView *view, QAbstractItemModel *model, int expandToDepth = 0)
{
    view->setModel(model);
    view->expandToDepth(expandToDepth);
    view->header()->setStretchLastSection(false);
    view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    view->setHeaderHidden(true);
}

DiagnosticConfigsWidget::DiagnosticConfigsWidget(const ClangDiagnosticConfigs &configs,
                                                 const Utils::Id &configToSelect,
                                                 const ClangTidyInfo &tidyInfo,
                                                 const ClazyStandaloneInfo &clazyInfo)
    : ClangDiagnosticConfigsWidget(configs, configToSelect)
    , m_tidyTreeModel(new TidyChecksTreeModel(tidyInfo.supportedChecks))
    , m_tidyInfo(tidyInfo)
    , m_clazyTreeModel(new ClazyChecksTreeModel(clazyInfo.supportedChecks))
    , m_clazyInfo(clazyInfo)
{
    m_clazyChecks = std::make_unique<Ui::ClazyChecks>();
    m_clazyChecksWidget = new QWidget();
    m_clazyChecks->setupUi(m_clazyChecksWidget);
    m_clazyChecks->invalidExecutableLabel->setType(Utils::InfoLabel::Warning);
    m_clazyChecks->invalidExecutableLabel->setElideMode(Qt::ElideNone);
    m_clazySortFilterProxyModel = new ClazyChecksSortFilterModel(this);
    m_clazySortFilterProxyModel->setSourceModel(m_clazyTreeModel.get());
    setupTreeView(m_clazyChecks->checksView, m_clazySortFilterProxyModel, 2);
    m_clazyChecks->checksView->setSortingEnabled(true);
    m_clazyChecks->checksView->sortByColumn(0, Qt::AscendingOrder);
    auto topicsModel = new QStringListModel(Utils::toList(m_clazyTreeModel->topics()), this);
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
    connect(m_clazyChecks->checksView,
            &QTreeView::clicked,
            [model = m_clazySortFilterProxyModel](const QModelIndex &index) {
                openUrl(model, index);
            });
    connect(m_clazyChecks->enableLowerLevelsCheckBox, &QCheckBox::stateChanged, [this](int) {
        const bool enable = m_clazyChecks->enableLowerLevelsCheckBox->isChecked();
        m_clazyTreeModel->setEnableLowerLevels(enable);
        codeModelSettings()->setEnableLowerClazyLevels(
            m_clazyChecks->enableLowerLevelsCheckBox->isChecked());
    });
    const Qt::CheckState checkEnableLowerClazyLevels
        = codeModelSettings()->enableLowerClazyLevels() ? Qt::Checked : Qt::Unchecked;
    m_clazyChecks->enableLowerLevelsCheckBox->setCheckState(checkEnableLowerClazyLevels);

    m_tidyChecks = std::make_unique<Ui::TidyChecks>();
    m_tidyChecksWidget = new QWidget();
    m_tidyChecks->setupUi(m_tidyChecksWidget);
    m_tidyChecks->invalidExecutableLabel->setType(Utils::InfoLabel::Warning);
    m_tidyChecks->invalidExecutableLabel->setElideMode(Qt::ElideNone);
    setupTreeView(m_tidyChecks->checksPrefixesTree, m_tidyTreeModel.get());

    connect(m_tidyChecks->checksPrefixesTree,
            &QTreeView::clicked,
            [model = m_tidyTreeModel.get()](const QModelIndex &index) { openUrl(model, index); });

    connect(m_tidyChecks->plainTextEditButton, &QPushButton::clicked, this, [this]() {
        const bool readOnly = currentConfig().isReadOnly();

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

    tabWidget()->addTab(m_tidyChecksWidget, tr("Clang-Tidy Checks"));
    tabWidget()->addTab(m_clazyChecksWidget, tr("Clazy Checks"));
}

DiagnosticConfigsWidget::~DiagnosticConfigsWidget() = default;

void DiagnosticConfigsWidget::syncClangTidyWidgets(const ClangDiagnosticConfig &config)
{
    enum TidyPages { // In sync with m_tidyChecks->stackedWidget pages.
        ChecksPage,
        EmptyPage,
        InvalidExecutablePage,
    };

    disconnectClangTidyItemChanged();

    const ClangDiagnosticConfig::TidyMode tidyMode = config.clangTidyMode();
    switch (tidyMode) {
    case ClangDiagnosticConfig::TidyMode::UseConfigFile:
        m_tidyChecks->tidyMode->setCurrentIndex(1);
        m_tidyChecks->plainTextEditButton->setVisible(false);
        m_tidyChecks->stackedWidget->setCurrentIndex(TidyPages::EmptyPage);
        break;
    case ClangDiagnosticConfig::TidyMode::UseCustomChecks:
    case ClangDiagnosticConfig::TidyMode::UseDefaultChecks:
        m_tidyChecks->tidyMode->setCurrentIndex(0);
        if (m_tidyInfo.supportedChecks.isEmpty()) {
            m_tidyChecks->plainTextEditButton->setVisible(false);
            m_tidyChecks->stackedWidget->setCurrentIndex(TidyPages::InvalidExecutablePage);
        } else {
            m_tidyChecks->plainTextEditButton->setVisible(true);
            m_tidyChecks->stackedWidget->setCurrentIndex(TidyPages::ChecksPage);
            syncTidyChecksToTree(config);
        }
        break;
    }

    const bool enabled = !config.isReadOnly();
    m_tidyChecks->tidyMode->setEnabled(enabled);
    m_tidyChecks->plainTextEditButton->setText(enabled ? tr("Edit Checks as String...")
                                                       : tr("View Checks as String..."));
    m_tidyTreeModel->setEnabled(enabled);
    connectClangTidyItemChanged();
}

void DiagnosticConfigsWidget::syncClazyWidgets(const ClangDiagnosticConfig &config)
{
    enum ClazyPages { // In sync with m_clazyChecks->stackedWidget pages.
        ChecksPage,
        InvalidExecutablePage,
    };

    if (m_clazyInfo.supportedChecks.isEmpty()) {
        m_clazyChecks->stackedWidget->setCurrentIndex(ClazyPages::InvalidExecutablePage);
        return;
    }

    m_clazyChecks->stackedWidget->setCurrentIndex(ClazyPages::ChecksPage);

    disconnectClazyItemChanged();
    const QStringList checkNames = config.clazyMode()
                                           == ClangDiagnosticConfig::ClazyMode::UseDefaultChecks
                                       ? m_clazyInfo.defaultChecks
                                       : config.clazyChecks().split(',', Qt::SkipEmptyParts);
    m_clazyTreeModel->enableChecks(checkNames);

    syncClazyChecksGroupBox();

    const bool enabled = !config.isReadOnly();
    m_clazyChecks->topicsResetButton->setEnabled(enabled);
    m_clazyChecks->enableLowerLevelsCheckBox->setEnabled(enabled);
    selectAll(m_clazyChecks->topicsView);
    m_clazyChecks->topicsView->setEnabled(enabled);
    m_clazyTreeModel->setEnabled(enabled);

    connectClazyItemChanged();
}

void DiagnosticConfigsWidget::syncTidyChecksToTree(const ClangDiagnosticConfig &config)
{
    const QString checks = config.clangTidyMode()
                                   == ClangDiagnosticConfig::TidyMode::UseDefaultChecks
                               ? m_tidyInfo.defaultChecks.join(',')
                               : config.clangTidyChecks();
    m_tidyTreeModel->selectChecks(checks);
}

void DiagnosticConfigsWidget::syncExtraWidgets(const ClangDiagnosticConfig &config)
{
    syncClangTidyWidgets(config);
    syncClazyWidgets(config);
}

void DiagnosticConfigsWidget::connectClangTidyItemChanged()
{
    connect(m_tidyChecks->tidyMode,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DiagnosticConfigsWidget::onClangTidyModeChanged);
    connect(m_tidyTreeModel.get(), &TidyChecksTreeModel::dataChanged,
            this, &DiagnosticConfigsWidget::onClangTidyTreeChanged);
}

void DiagnosticConfigsWidget::disconnectClangTidyItemChanged()
{
    disconnect(m_tidyChecks->tidyMode,
               QOverload<int>::of(&QComboBox::currentIndexChanged),
               this,
               &DiagnosticConfigsWidget::onClangTidyModeChanged);
    disconnect(m_tidyTreeModel.get(), &TidyChecksTreeModel::dataChanged,
               this, &DiagnosticConfigsWidget::onClangTidyTreeChanged);
}

void DiagnosticConfigsWidget::connectClazyItemChanged()
{
    connect(m_clazyTreeModel.get(), &ClazyChecksTreeModel::dataChanged,
            this, &DiagnosticConfigsWidget::onClazyTreeChanged);
}

void DiagnosticConfigsWidget::disconnectClazyItemChanged()
{
    disconnect(m_clazyTreeModel.get(), &ClazyChecksTreeModel::dataChanged,
               this, &DiagnosticConfigsWidget::onClazyTreeChanged);
}

void DiagnosticConfigsWidget::onClangTidyModeChanged(int index)
{
    const ClangDiagnosticConfig::TidyMode tidyMode
        = index == 0 ? ClangDiagnosticConfig::TidyMode::UseCustomChecks
                     : ClangDiagnosticConfig::TidyMode::UseConfigFile;

    ClangDiagnosticConfig config = currentConfig();
    config.setClangTidyMode(tidyMode);
    updateConfig(config);
    syncClangTidyWidgets(config);
}

void DiagnosticConfigsWidget::onClangTidyTreeChanged()
{
    ClangDiagnosticConfig config = currentConfig();
    if (config.clangTidyMode() == ClangDiagnosticConfig::TidyMode::UseDefaultChecks)
        config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
    config.setClangTidyChecks(m_tidyTreeModel->selectedChecks());
    updateConfig(config);
}

void DiagnosticConfigsWidget::onClazyTreeChanged()
{
    syncClazyChecksGroupBox();

    ClangDiagnosticConfig config = currentConfig();
    if (config.clazyMode() == ClangDiagnosticConfig::ClazyMode::UseDefaultChecks)
        config.setClazyMode(ClangDiagnosticConfig::ClazyMode::UseCustomChecks);
    config.setClazyChecks(m_clazyTreeModel->enabledChecks().join(","));
    updateConfig(config);
}

void DiagnosticConfigsWidget::syncClazyChecksGroupBox()
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

} // namespace Internal
} // namespace ClangTools

#include "diagnosticconfigswidget.moc"
