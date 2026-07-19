// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcschangesview.h"

#include "vcsbaseconstants.h"
#include "vcsbaseplugin.h"
#include "vcsbasetr.h"
#include "vcsfilestatus.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/navigationtreeview.h>
#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QApplication>
#include <QItemSelectionModel>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace VcsBase::Internal {

static Q_LOGGING_CATEGORY(log, "qtc.vcs.changesview", QtWarningMsg)

using Section = VcsFileStatus::Section;

class ChangedFileItem final : public TreeItem
{
public:
    ChangedFileItem(VersionControlBase *vc, const FilePath &repository,
                    const VcsFileStatus &status)
        : m_vc(vc), m_repository(repository), m_status(status)
    {}

    VersionControlBase *versionControl() const { return m_vc; }
    FilePath repository() const { return m_repository; }
    VcsFileStatus status() const { return m_status; }
    FilePath absoluteFilePath() const { return m_repository.pathAppended(m_status.relativePath); }

    // A checkable item lets the user pick which files an inline commit includes
    // (used by version controls without a staging area).
    void setCheckable(bool checkable) { m_checkable = checkable; }
    void setChecked(bool checked) { m_checked = checked; }
    bool isCheckable() const { return m_checkable; }
    bool isChecked() const { return m_checked; }

    Qt::ItemFlags flags(int) const final
    {
        Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (m_checkable)
            f |= Qt::ItemIsUserCheckable;
        return f;
    }

    QVariant data(int column, int role) const final
    {
        if (column != 0)
            return {};
        switch (role) {
        case Qt::DisplayRole:
            return m_status.relativePath;
        case Qt::DecorationRole:
            return FileIconProvider::icon(absoluteFilePath());
        case Qt::ForegroundRole:
            return VcsManager::fileStateColor(m_status.state);
        case Qt::CheckStateRole:
            return m_checkable ? QVariant(m_checked ? Qt::Checked : Qt::Unchecked) : QVariant();
        case Qt::ToolTipRole:
            return QString(absoluteFilePath().toUserOutput() + '\n'
                           + VcsManager::fileStateDescription(m_status.state));
        }
        return {};
    }

    bool setData(int column, const QVariant &value, int role) final
    {
        if (column == 0 && role == Qt::CheckStateRole && m_checkable) {
            m_checked = value.toInt() == Qt::Checked;
            return true;
        }
        return TreeItem::setData(column, value, role);
    }

private:
    VersionControlBase *m_vc = nullptr;
    FilePath m_repository;
    VcsFileStatus m_status;
    bool m_checkable = false;
    bool m_checked = true;
};

class SectionItem final : public TypedTreeItem<ChangedFileItem>
{
public:
    explicit SectionItem(Section section)
        : m_section(section)
    {}

    Section section() const { return m_section; }

    QVariant data(int column, int role) const final
    {
        if (column != 0)
            return {};
        if (role == Qt::DisplayRole) {
            QString title;
            switch (m_section) {
            case Section::Staged:
                title = Tr::tr("Staged Changes");
                break;
            case Section::Conflicted:
                title = Tr::tr("Conflicts");
                break;
            case Section::Changed:
                title = Tr::tr("Changes");
                break;
            }
            return QString("%1 (%2)").arg(title).arg(childCount());
        }
        return {};
    }

    Qt::ItemFlags flags(int) const final { return Qt::ItemIsEnabled; }

private:
    Section m_section;
};

class RepositoryItem final : public TreeItem
{
public:
    RepositoryItem(VersionControlBase *vc, const FilePath &repository,
                   const FilePath &parentRepository = {})
        : m_vc(vc), m_repository(repository), m_parentRepository(parentRepository)
    {}

    VersionControlBase *versionControl() const { return m_vc; }
    FilePath repository() const { return m_repository; }
    bool isSubRepository() const { return !m_parentRepository.isEmpty(); }
    void setTopic(const QString &topic) { m_topic = topic; }

    QVariant data(int column, int role) const final
    {
        if (column != 0)
            return {};
        switch (role) {
        case Qt::DisplayRole: {
            QString vcs = m_vc->displayName();
            if (!m_topic.isEmpty())
                vcs += ": " + m_topic;
            const QString name = isSubRepository()
                ? m_repository.relativeChildPath(m_parentRepository).toUserOutput()
                : m_repository.fileName();
            return QString("%1 [%2]").arg(name, vcs);
        }
        case Qt::DecorationRole:
            return FileIconProvider::icon(m_repository);
        case Qt::ToolTipRole:
            return m_repository.toUserOutput();
        }
        return {};
    }

private:
    VersionControlBase *m_vc = nullptr;
    FilePath m_repository;
    FilePath m_parentRepository;
    QString m_topic;
};

class ProjectItem final : public TreeItem
{
public:
    ProjectItem(const QString &displayName, const FilePath &projectPath)
        : m_displayName(displayName), m_projectPath(projectPath)
    {}

    FilePath projectPath() const { return m_projectPath; }

    QVariant data(int column, int role) const final
    {
        if (column != 0)
            return {};
        switch (role) {
        case Qt::DisplayRole:
            return m_displayName;
        case Qt::ToolTipRole:
            return m_projectPath.toUserOutput();
        }
        return {};
    }

    Qt::ItemFlags flags(int) const final { return Qt::ItemIsEnabled; }

private:
    QString m_displayName;
    FilePath m_projectPath;
};

// Plain text edit that grows with its content up to a maximum number of lines,
// after which it starts to scroll.
class CommitMessageEdit final : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CommitMessageEdit(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
    {
        setPlaceholderText(Tr::tr("Message"));
        setLineWrapMode(QPlainTextEdit::WidgetWidth);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setTabChangesFocus(true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        const auto notify = [this] {
            updateGeometry();
            emit sizeHintChanged();
        };
        connect(this, &QPlainTextEdit::textChanged, this, notify);
        connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
                this, [notify](const QSizeF &) { notify(); });
    }

    QSize sizeHint() const final
    {
        return {QPlainTextEdit::sizeHint().width(), preferredHeight()};
    }
    // Zero minimum width so the editor can shrink and the commit button always
    // fits, even in a narrow panel; only the height is constrained.
    QSize minimumSizeHint() const final { return {0, preferredHeight()}; }

signals:
    void sizeHintChanged();

private:
    int preferredHeight() const
    {
        const int lines = std::clamp(document()->lineCount(), 1, maxLines);
        const int frame = 2 * frameWidth();
        const int docMargin = 2 * qRound(document()->documentMargin());
        const QMargins cm = contentsMargins();
        return lines * fontMetrics().lineSpacing() + docMargin + frame + cm.top() + cm.bottom();
    }

    static constexpr int maxLines = 10;
};

// A message input with a "Commit" button, embedded per repository in the tree.
class CommitBar final : public QWidget
{
    Q_OBJECT

public:
    explicit CommitBar(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_edit = new CommitMessageEdit(this);
        m_commitButton = new QtcButton(Tr::tr("Commit"), QtcButton::SmallPrimary, this);
        m_commitButton->setEnabled(false);
        m_commitButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        using namespace StyleHelper::SpacingTokens;
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(PaddingHM, PaddingVXs, PaddingHM, PaddingVXs);
        layout->setSpacing(GapVXs);
        layout->addWidget(m_edit);
        layout->addWidget(m_commitButton);

        connect(m_edit, &CommitMessageEdit::sizeHintChanged, this, [this] {
            updateGeometry();
            emit heightChanged();
        });
        connect(m_edit, &QPlainTextEdit::textChanged, this, [this] {
            updateCommitButton();
            emit draftChanged(m_edit->toPlainText());
        });
        connect(m_commitButton, &QAbstractButton::clicked, this, [this] {
            emit commitRequested(m_edit->toPlainText());
        });
    }

    // Height needed to stack the (already line-clamped) message editor above the
    // commit button, used to size the hosting tree row. The layout's size hint
    // expands each child to its minimum size hint, so it accounts for the
    // button's real height (QtcButton reports it via minimumSizeHint()).
    int preferredHeight() const { return layout()->sizeHint().height(); }

    // Zero preferred width so the bar never widens the (ResizeToContents) tree
    // column past the viewport; the editor just fills whatever width the cell
    // offers and the commit button always stays visible.
    QSize sizeHint() const final { return {0, preferredHeight()}; }
    QSize minimumSizeHint() const final { return {0, preferredHeight()}; }

    void setDraft(const QString &text)
    {
        if (m_edit->toPlainText() == text)
            return;
        QSignalBlocker blocker(m_edit);
        m_edit->setPlainText(text);
        updateCommitButton();
    }

    void setCanCommit(bool canCommit)
    {
        m_canCommit = canCommit;
        updateCommitButton();
    }

protected:
    void showEvent(QShowEvent *event) final
    {
        QWidget::showEvent(event);
        // Recompute now that child widgets are polished and report their real
        // size hints; the height set at construction is based on unpolished ones.
        emit heightChanged();
    }

signals:
    void heightChanged();
    void draftChanged(const QString &text);
    void commitRequested(const QString &message);

private:
    void updateCommitButton()
    {
        m_commitButton->setEnabled(m_canCommit && !m_edit->toPlainText().trimmed().isEmpty());
    }

    CommitMessageEdit *m_edit = nullptr;
    QtcButton *m_commitButton = nullptr;
    bool m_canCommit = false;
};

// Tree item hosting a CommitBar via QAbstractItemView::setIndexWidget(). Its row
// height is driven by the bar through Qt::SizeHintRole.
class CommitItem final : public TreeItem
{
public:
    CommitItem(VersionControlBase *vc, const FilePath &repository)
        : m_vc(vc), m_repository(repository)
    {}

    VersionControlBase *versionControl() const { return m_vc; }
    FilePath repository() const { return m_repository; }

    void setPreferredHeight(int height)
    {
        if (height > 0 && height != m_height) {
            m_height = height;
            update();
        }
    }

    Qt::ItemFlags flags(int) const final { return Qt::ItemIsEnabled; }

    QVariant data(int column, int role) const final
    {
        if (column == 0 && role == Qt::SizeHintRole && m_height > 0)
            return QSize(0, m_height);
        return {};
    }

private:
    VersionControlBase *m_vc = nullptr;
    FilePath m_repository;
    int m_height = 0;
};

// Relays a CommitItem's size change to the view so its row gets re-laid out;
// the row height itself comes from CommitItem's Qt::SizeHintRole.
class CommitDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void notifySizeChanged(const QModelIndex &index) { emit sizeHintChanged(index); }
};

class ChangesDelegate : public QStyledItemDelegate
{
public:
    explicit ChangesDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const final
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->palette.setColor(QPalette::HighlightedText, option->palette.color(QPalette::Text));
    }
};

class ChangesView final : public QWidget
{
public:
    ChangesView();

    QList<QToolButton *> createToolButtons();

protected:
    void showEvent(QShowEvent *event) final;

private:
    void rebuildProjectTree();
    void updateRepositoryItem(RepositoryItem *item);
    void ensureCommitBar(CommitItem *item);
    void commitRepository(VersionControlBase *vc, const FilePath &repository,
                          const QString &message);
    QStringList checkedFiles(VersionControlBase *vc, const FilePath &repository) const;
    void fileCheckStateChanged(ChangedFileItem *item);
    void repositoryStatusChanged(const FilePath &repository);
    void repositoryChanged(const FilePath &repository);
    void requestRefresh(bool onlyDirty);
    void requestAutomaticRefresh(const FilePath &changedFile = {});
    void activate(const QModelIndex &index);
    void openContextMenu(const QPoint &point);
    QString itemKey(TreeItem *item) const;
    void restoreExpandedState(TreeItem *item);
    void forEachRepositoryItem(const std::function<void(RepositoryItem *)> &callback,
                               const FilePath &repository = {});

    QAction *m_refreshAction = nullptr;
    NavigationTreeView *m_treeView = nullptr;
    TreeModel<> *m_model = nullptr;

    struct RepositoryEntry
    {
        VersionControlBase *vc = nullptr;
        FilePath repository;
        FilePath parent; // Empty for top-level (project) repositories.
        friend bool operator==(const RepositoryEntry &, const RepositoryEntry &) = default;
    };
    bool hasChanges(const RepositoryEntry &entry) const;

    QList<RepositoryEntry> m_repositories;
    QHash<FilePath, FilePath> m_subToParent; // Sub-repository -> direct parent.
    QSet<VersionControlBase *> m_connectedVcs;
    QSet<FilePath> m_dirtyRepositories;
    QSet<QString> m_collapsedKeys;
    CommitDelegate *m_commitDelegate = nullptr;
    QHash<FilePath, QString> m_commitDrafts;              // Unsent messages per repository.
    QHash<FilePath, QPointer<CommitBar>> m_commitBars;   // Live commit bars per repository.
    // Files (relative paths) the user unchecked per repository; anything else is
    // considered checked. Only used by version controls without staging.
    QHash<FilePath, QSet<QString>> m_uncheckedFiles;
};

ChangesView::ChangesView()
    : m_refreshAction(new QAction(this))
    , m_treeView(new NavigationTreeView(this))
    , m_model(new TreeModel<>(this))
{
    m_refreshAction->setIcon(Utils::Icons::RELOAD_TOOLBAR.icon());
    m_refreshAction->setToolTip(Tr::tr("Refresh"));
    connect(m_refreshAction, &QAction::triggered, this, [this] {
        // Re-resolve the version control for all projects: a repository may have
        // been created after a project was opened, and the VcsManager caches
        // negative lookups.
        for (Project *project : ProjectManager::projects())
            VcsManager::resetVersionControlForDirectory(project->projectDirectory());
        rebuildProjectTree();
        requestRefresh(false);
    });

    m_treeView->setItemDelegate(new ChangesDelegate(m_treeView));
    m_treeView->setHeaderHidden(true);
    m_treeView->setModel(m_model);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    // NavigationTreeView defaults to uniform row heights, which ignores the
    // per-row size hint the commit bar needs to grow.
    m_treeView->setUniformRowHeights(false);
    m_commitDelegate = new CommitDelegate(m_treeView);
    m_treeView->setItemDelegate(m_commitDelegate);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_model, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &topLeft, const QModelIndex &, const QList<int> &roles) {
        if (!roles.isEmpty() && !roles.contains(Qt::CheckStateRole))
            return;
        if (const auto item = dynamic_cast<ChangedFileItem *>(m_model->itemForIndex(topLeft)))
            fileCheckStateChanged(item);
    });
    connect(m_treeView, &QAbstractItemView::activated, this, &ChangesView::activate);
    connect(m_treeView, &QWidget::customContextMenuRequested,
            this, &ChangesView::openContextMenu);
    connect(m_treeView, &QTreeView::collapsed, this, [this](const QModelIndex &index) {
        const QString key = itemKey(m_model->itemForIndex(index));
        if (!key.isEmpty())
            m_collapsedKeys.insert(key);
    });
    connect(m_treeView, &QTreeView::expanded, this, [this](const QModelIndex &index) {
        m_collapsedKeys.remove(itemKey(m_model->itemForIndex(index)));
    });

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_treeView);
    layout->setContentsMargins(0, 2, 0, 0);
    setLayout(layout);

    connect(ProjectManager::instance(), &ProjectManager::projectAdded,
            this, &ChangesView::rebuildProjectTree);
    connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
            this, &ChangesView::rebuildProjectTree);
    connect(VcsManager::instance(), &VcsManager::repositoryChanged,
            this, &ChangesView::repositoryChanged);
    connect(EditorManager::instance(), &EditorManager::saved,
            this, [this](IDocument *document, IDocument::SaveOption option) {
        if (option != IDocument::SaveOption::AutoSave)
            requestAutomaticRefresh(document->filePath());
    });
    connect(qApp, &QApplication::applicationStateChanged,
            this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive)
            requestAutomaticRefresh();
    });

    rebuildProjectTree();
}

QList<QToolButton *> ChangesView::createToolButtons()
{
    auto refreshButton = new QToolButton;
    refreshButton->setDefaultAction(m_refreshAction);
    refreshButton->setProperty(StyleHelper::C_NO_ARROW, true);
    return {refreshButton};
}

void ChangesView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    requestRefresh(true);
}

QString ChangesView::itemKey(TreeItem *item) const
{
    if (const auto projectItem = dynamic_cast<ProjectItem *>(item))
        return "p|" + projectItem->projectPath().toUrlishString();
    if (const auto repositoryItem = dynamic_cast<RepositoryItem *>(item))
        return "r|" + repositoryItem->repository().toUrlishString();
    if (const auto sectionItem = dynamic_cast<SectionItem *>(item)) {
        const auto repositoryItem = static_cast<RepositoryItem *>(sectionItem->parent());
        return "s|" + repositoryItem->repository().toUrlishString() + '|'
               + QString::number(int(sectionItem->section()));
    }
    return {};
}

void ChangesView::restoreExpandedState(TreeItem *item)
{
    item->forAllChildren([this](TreeItem *child) {
        if (child->childCount() == 0)
            return;
        const QString key = itemKey(child);
        if (!key.isEmpty() && !m_collapsedKeys.contains(key))
            m_treeView->expand(m_model->indexForItem(child));
    });
}

void ChangesView::forEachRepositoryItem(const std::function<void(RepositoryItem *)> &callback,
                                        const FilePath &repository)
{
    QList<RepositoryItem *> items;
    m_model->rootItem()->forAllChildren([&items, &repository](TreeItem *item) {
        const auto repositoryItem = dynamic_cast<RepositoryItem *>(item);
        if (repositoryItem && (repository.isEmpty() || repositoryItem->repository() == repository))
            items.append(repositoryItem);
    });
    for (RepositoryItem *item : std::as_const(items))
        callback(item);
}

void ChangesView::rebuildProjectTree()
{
    struct Entry
    {
        Project *project = nullptr;
        VersionControlBase *vc = nullptr;
        FilePath repository;
    };
    QList<Entry> entries;
    for (Project *project : ProjectManager::projects()) {
        // Ensure the project directory is monitored. This is idempotent, and
        // for directories without version control it arms the detection of
        // repositories created after the project was opened.
        VcsManager::monitorDirectory(project->projectDirectory(), true);

        FilePath topLevel;
        IVersionControl *vc
            = VcsManager::findVersionControlForDirectory(project->projectDirectory(), &topLevel);
        const auto vcb = qobject_cast<VersionControlBase *>(vc);
        qCDebug(log) << "rebuildProjectTree:" << project->displayName()
                     << project->projectDirectory() << "vc:" << (vc ? vc->displayName() : QString("-"))
                     << "topLevel:" << topLevel
                     << "supported:" << (vcb && vcb->supportsChangesView());
        if (!vcb || !vcb->supportsChangesView())
            continue;
        entries.append({project, vcb, topLevel});
    }
    qCDebug(log) << "rebuildProjectTree: projects:" << ProjectManager::projects().size()
                 << "entries:" << entries.size();

    // Track the listed repositories. Monitoring is not needed here: the project
    // tree already monitors all project directories.
    QList<RepositoryEntry> repositories;
    for (const Entry &entry : std::as_const(entries)) {
        const RepositoryEntry repo{entry.vc, entry.repository, {}};
        if (!repositories.contains(repo))
            repositories.append(repo);
    }

    // Recursively collect sub-repositories (submodules, externals, ...). A
    // sub-repository may use a different version control system than its
    // parent, so each path is resolved independently. A repository that is
    // also a project's top-level repository stays top-level.
    QSet<FilePath> visited
        = Utils::transform<QSet>(repositories,
                                 [](const RepositoryEntry &entry) { return entry.repository; });
    QHash<FilePath, int> depth;
    m_subToParent.clear();
    for (int i = 0; i < repositories.size(); ++i) {
        const RepositoryEntry entry = repositories.at(i); // Copy: the list grows.
        if (depth.value(entry.repository) >= 8)
            continue;
        const FilePaths subPaths = entry.vc->subRepositories(entry.repository);
        for (const FilePath &subPath : subPaths) {
            if (visited.contains(subPath))
                continue;
            visited.insert(subPath);
            // Idempotent; for git this also monitors the sub-repository's own
            // submodules, enabling recursive discovery.
            VcsManager::monitorDirectory(subPath, true);
            FilePath topLevel;
            IVersionControl *vc = VcsManager::findVersionControlForDirectory(subPath, &topLevel);
            const auto vcb = qobject_cast<VersionControlBase *>(vc);
            // topLevel != subPath: an uninitialized sub-repository resolves to
            // the containing repository.
            if (!vcb || !vcb->supportsChangesView() || topLevel != subPath)
                continue;
            qCDebug(log) << "rebuildProjectTree: sub-repository" << subPath << "of"
                         << entry.repository << "vc:" << vcb->displayName();
            repositories.append({vcb, subPath, entry.repository});
            m_subToParent.insert(subPath, entry.repository);
            depth.insert(subPath, depth.value(entry.repository) + 1);
        }
    }

    for (const RepositoryEntry &old : std::as_const(m_repositories)) {
        if (!repositories.contains(old))
            m_dirtyRepositories.remove(old.repository);
    }
    for (const RepositoryEntry &repo : std::as_const(repositories)) {
        if (!m_repositories.contains(repo))
            m_dirtyRepositories.insert(repo.repository);
        if (!m_connectedVcs.contains(repo.vc)) {
            connect(repo.vc, &VersionControlBase::repositoryStatusChanged,
                    this, &ChangesView::repositoryStatusChanged);
            m_connectedVcs.insert(repo.vc);
        }
    }
    m_repositories = repositories;

    // Rebuild the tree. The project level is omitted if only one project is loaded.
    const QSet<Project *> projects
        = Utils::transform<QSet>(entries, [](const Entry &entry) { return entry.project; });
    // clear() removes every row, so the view destroys the embedded commit bars.
    m_commitBars.clear();
    m_model->clear();
    TreeItem *root = m_model->rootItem();
    if (projects.size() > 1) {
        QHash<Project *, ProjectItem *> projectItems;
        for (const Entry &entry : std::as_const(entries)) {
            ProjectItem *&projectItem = projectItems[entry.project];
            if (!projectItem) {
                projectItem = new ProjectItem(entry.project->displayName(),
                                              entry.project->projectDirectory());
                root->appendChild(projectItem);
            }
            projectItem->appendChild(new RepositoryItem(entry.vc, entry.repository));
        }
    } else {
        for (const Entry &entry : std::as_const(entries))
            root->appendChild(new RepositoryItem(entry.vc, entry.repository));
    }

    forEachRepositoryItem([this](RepositoryItem *item) { updateRepositoryItem(item); });
    restoreExpandedState(m_model->rootItem());

    if (isVisible())
        requestRefresh(true);
}

void ChangesView::updateRepositoryItem(RepositoryItem *item)
{
    VersionControlBase *vc = item->versionControl();
    const FilePath repository = item->repository();
    VcsFileStatusList status = vc->repositoryStatus(repository);

    const auto lessThan = [](const VcsFileStatus &a, const VcsFileStatus &b) {
        return a.relativePath.compare(b.relativePath, Qt::CaseInsensitive) < 0;
    };
    std::sort(status.begin(), status.end(), lessThan);

    const bool staging = vc->supportsStaging();
    const bool hasStaged = Utils::anyOf(status, [](const VcsFileStatus &file) {
        return file.section == Section::Staged;
    });
    // A version control with a staging area commits its index; others commit the
    // files the user checked, so the bar appears whenever there is anything to
    // commit.
    const bool wantCommitBar = staging ? hasStaged : !status.isEmpty();

    // Drop remembered unchecks for files that are no longer reported.
    if (!staging) {
        QSet<QString> &unchecked = m_uncheckedFiles[repository];
        if (!unchecked.isEmpty()) {
            const QSet<QString> present = Utils::transform<QSet>(status,
                [](const VcsFileStatus &f) { return f.relativePath; });
            unchecked.intersect(present);
        }
    }

    // Preserve an existing commit bar (kept as the first child) across in-place
    // refreshes so the message being typed and the editor focus survive.
    CommitItem *commitItem = item->childCount() > 0
        ? dynamic_cast<CommitItem *>(item->childAt(0)) : nullptr;
    if (wantCommitBar && !commitItem) {
        commitItem = new CommitItem(vc, repository);
        item->insertChild(0, commitItem);
        ensureCommitBar(commitItem);
    } else if (!wantCommitBar && commitItem) {
        item->removeChildAt(0);
        m_commitBars.remove(repository);
        commitItem = nullptr;
    }

    // Rebuild every child except the preserved commit bar at index 0.
    const int keep = commitItem ? 1 : 0;
    while (item->childCount() > keep)
        item->removeChildAt(keep);

    if (commitItem) {
        if (CommitBar *bar = m_commitBars.value(repository))
            bar->setCanCommit(staging ? hasStaged : !checkedFiles(vc, repository).isEmpty());
    }

    if (staging) {
        for (const Section section : {Section::Conflicted, Section::Staged, Section::Changed}) {
            const VcsFileStatusList files = Utils::filtered(status,
                [section](const VcsFileStatus &file) { return file.section == section; });
            if (files.isEmpty())
                continue;
            auto sectionItem = new SectionItem(section);
            for (const VcsFileStatus &file : files)
                sectionItem->appendChild(new ChangedFileItem(vc, repository, file));
            item->appendChild(sectionItem);
        }
    } else {
        // Without a staging area, each file gets a checkbox so the user can pick
        // what the inline commit includes.
        const QSet<QString> unchecked = m_uncheckedFiles.value(repository);
        for (const VcsFileStatus &file : std::as_const(status)) {
            auto fileItem = new ChangedFileItem(vc, repository, file);
            fileItem->setCheckable(true);
            fileItem->setChecked(!unchecked.contains(file.relativePath));
            item->appendChild(fileItem);
        }
    }

    // Nested sub-repositories, shown only while they (or their own nested
    // sub-repositories) have changes.
    QList<RepositoryEntry> subs = Utils::filtered(m_repositories,
        [&repository](const RepositoryEntry &entry) { return entry.parent == repository; });
    Utils::sort(subs, [](const RepositoryEntry &a, const RepositoryEntry &b) {
        return a.repository < b.repository;
    });
    for (const RepositoryEntry &sub : std::as_const(subs)) {
        if (!hasChanges(sub))
            continue;
        auto subItem = new RepositoryItem(sub.vc, sub.repository, repository);
        item->appendChild(subItem);
        updateRepositoryItem(subItem);
    }

    if (item->childCount() == keep && !item->isSubRepository())
        item->appendChild(new StaticTreeItem(Tr::tr("No changes")));

    item->setTopic(vc->vcsTopic(repository));
    item->update();
    restoreExpandedState(item);
    const QString key = itemKey(item);
    if (!m_collapsedKeys.contains(key))
        m_treeView->expand(m_model->indexForItem(item));
}

void ChangesView::ensureCommitBar(CommitItem *item)
{
    VersionControlBase *vc = item->versionControl();
    const FilePath repository = item->repository();

    auto bar = new CommitBar;
    bar->setDraft(m_commitDrafts.value(repository));
    connect(bar, &CommitBar::draftChanged, this, [this, repository](const QString &text) {
        if (text.isEmpty())
            m_commitDrafts.remove(repository);
        else
            m_commitDrafts.insert(repository, text);
    });
    connect(bar, &CommitBar::commitRequested, this, [this, vc, repository](const QString &message) {
        commitRepository(vc, repository, message);
    });
    connect(bar, &CommitBar::heightChanged, this, [this, item, bar] {
        item->setPreferredHeight(bar->preferredHeight());
        const QModelIndex index = m_model->indexForItem(item);
        if (index.isValid())
            m_commitDelegate->notifySizeChanged(index);
    });

    const QModelIndex index = m_model->indexForItem(item);
    m_treeView->setIndexWidget(index, bar);
    // Size the row to the bar right away; the row was laid out before the widget
    // existed, so its height would otherwise stay at the default.
    item->setPreferredHeight(bar->preferredHeight());
    if (index.isValid())
        m_commitDelegate->notifySizeChanged(index);
    m_commitBars.insert(repository, bar);
}

QStringList ChangesView::checkedFiles(VersionControlBase *vc, const FilePath &repository) const
{
    const QSet<QString> unchecked = m_uncheckedFiles.value(repository);
    QStringList files;
    for (const VcsFileStatus &file : vc->repositoryStatus(repository)) {
        if (!unchecked.contains(file.relativePath))
            files.append(file.relativePath);
    }
    return files;
}

void ChangesView::fileCheckStateChanged(ChangedFileItem *item)
{
    const FilePath repository = item->repository();
    QSet<QString> &unchecked = m_uncheckedFiles[repository];
    if (item->isChecked())
        unchecked.remove(item->status().relativePath);
    else
        unchecked.insert(item->status().relativePath);
    if (CommitBar *bar = m_commitBars.value(repository))
        bar->setCanCommit(!checkedFiles(item->versionControl(), repository).isEmpty());
}

void ChangesView::commitRepository(VersionControlBase *vc, const FilePath &repository,
                                   const QString &message)
{
    if (message.trimmed().isEmpty())
        return;
    // Staging version controls (git) commit their index; others commit the
    // files the user left checked.
    const QStringList files = vc->supportsStaging() ? QStringList() : checkedFiles(vc, repository);
    if (!vc->supportsStaging() && files.isEmpty())
        return;
    if (vc->commitFiles(repository, files, message)) {
        m_commitDrafts.remove(repository);
        m_uncheckedFiles.remove(repository);
        vc->requestRepositoryStatus(repository);
    }
}

bool ChangesView::hasChanges(const RepositoryEntry &entry) const
{
    if (!entry.vc->repositoryStatus(entry.repository).isEmpty())
        return true;
    return Utils::anyOf(m_repositories, [this, &entry](const RepositoryEntry &sub) {
        return sub.parent == entry.repository && hasChanges(sub);
    });
}

void ChangesView::repositoryStatusChanged(const FilePath &repository)
{
    // Update from the top-level ancestor: nested repository items exist only
    // while they have changes, so the changed sub-repository may have no item
    // yet, or its item may have to disappear.
    FilePath root = repository;
    QSet<FilePath> seen;
    while (m_subToParent.contains(root) && !seen.contains(root)) {
        seen.insert(root);
        root = m_subToParent.value(root);
    }
    forEachRepositoryItem([this](RepositoryItem *item) { updateRepositoryItem(item); },
                          root);
    if (root != repository) {
        // The path may additionally be listed as a top-level repository of
        // another project.
        forEachRepositoryItem([this](RepositoryItem *item) { updateRepositoryItem(item); },
                              repository);
    }
}

void ChangesView::repositoryChanged(const FilePath &repository)
{
    const bool listed = Utils::anyOf(m_repositories, [&repository](const RepositoryEntry &entry) {
        return entry.repository == repository;
    });
    if (!listed) {
        // A repository may have appeared for a project that had no version
        // control so far, e.g. "git init" after opening the project.
        const bool affectsProject = Utils::anyOf(ProjectManager::projects(),
            [&repository](Project *project) {
                const FilePath dir = project->projectDirectory();
                return dir == repository || dir.isChildOf(repository)
                       || repository.isChildOf(dir);
            });
        if (affectsProject)
            rebuildProjectTree();
        return;
    }
    if (isVisible()) {
        for (const RepositoryEntry &entry : std::as_const(m_repositories)) {
            if (entry.repository == repository)
                entry.vc->requestRepositoryStatus(repository);
        }
        forEachRepositoryItem([&repository](RepositoryItem *item) {
            if (item->repository() == repository) {
                item->setTopic(item->versionControl()->vcsTopic(repository));
                item->update();
            }
        });
    } else {
        m_dirtyRepositories.insert(repository);
    }
}

void ChangesView::requestRefresh(bool onlyDirty)
{
    for (const RepositoryEntry &entry : std::as_const(m_repositories)) {
        if (!onlyDirty || m_dirtyRepositories.contains(entry.repository))
            entry.vc->requestRepositoryStatus(entry.repository);
    }
    m_dirtyRepositories.clear();
}

void ChangesView::requestAutomaticRefresh(const FilePath &changedFile)
{
    for (const RepositoryEntry &entry : std::as_const(m_repositories)) {
        if (!entry.vc->supportsAutomaticStatusUpdates())
            continue;
        if (!changedFile.isEmpty() && !changedFile.isChildOf(entry.repository))
            continue;
        if (isVisible())
            entry.vc->requestRepositoryStatus(entry.repository);
        else
            m_dirtyRepositories.insert(entry.repository);
    }
}

void ChangesView::activate(const QModelIndex &index)
{
    const auto item = dynamic_cast<ChangedFileItem *>(m_model->itemForIndex(index));
    if (!item)
        return;
    const VcsFileStatus status = item->status();
    if (status.state == VcsFileState::Untracked)
        EditorManager::openEditor(item->absoluteFilePath());
    else
        item->versionControl()->diffChangedFile(item->repository(), status.relativePath,
                                                status.section);
}

void ChangesView::openContextMenu(const QPoint &point)
{
    const QModelIndex index = m_treeView->indexAt(point);
    if (!index.isValid())
        return;

    QMenu menu;
    if (const auto repositoryItem = dynamic_cast<RepositoryItem *>(m_model->itemForIndex(index))) {
        VersionControlBase *vc = repositoryItem->versionControl();
        const FilePath repository = repositoryItem->repository();
        menu.addAction(Tr::tr("Refresh"), this, [vc, repository] {
            vc->requestRepositoryStatus(repository);
        });
        menu.exec(m_treeView->mapToGlobal(point));
        return;
    }

    if (!dynamic_cast<ChangedFileItem *>(m_model->itemForIndex(index)))
        return;

    // Right-clicking a row outside the current selection selects just that row;
    // right-clicking inside the selection keeps it, so actions apply to every
    // selected file.
    QItemSelectionModel *selectionModel = m_treeView->selectionModel();
    if (!selectionModel->isSelected(index)) {
        selectionModel->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        m_treeView->setCurrentIndex(index);
    }

    // Snapshot the selected files by value: an action may trigger an
    // asynchronous status update that rebuilds the tree and invalidates the
    // item pointers.
    struct Target
    {
        VersionControlBase *vc = nullptr;
        FilePath repository;
        VcsFileStatus status;
        FilePath absolutePath;
    };
    QList<Target> targets;
    for (const QModelIndex &selected : selectionModel->selectedIndexes()) {
        if (const auto item = dynamic_cast<ChangedFileItem *>(m_model->itemForIndex(selected)))
            targets.append({item->versionControl(), item->repository(), item->status(),
                            item->absoluteFilePath()});
    }
    if (targets.isEmpty())
        return;

    const bool single = targets.size() == 1;
    const auto inState = [](VcsFileState state) {
        return [state](const Target &t) { return t.status.state == state; };
    };
    const bool anyDeleted = Utils::anyOf(targets, inState(VcsFileState::Deleted));
    const bool allUntracked = Utils::allOf(targets, inState(VcsFileState::Untracked));
    const bool noneUntracked = !Utils::anyOf(targets, inState(VcsFileState::Untracked));
    const bool noneUnmerged = !Utils::anyOf(targets, inState(VcsFileState::Unmerged));
    const bool allStaged = Utils::allOf(targets,
        [](const Target &t) { return t.status.section == Section::Staged; });
    const bool noneStaged = !Utils::anyOf(targets,
        [](const Target &t) { return t.status.section == Section::Staged; });
    const bool allSupportStaging = Utils::allOf(targets,
        [](const Target &t) { return t.vc->supportsStaging(); });
    const bool allSupportAdd = Utils::allOf(targets, [](const Target &t) {
        return t.vc->supportsOperation(IVersionControl::AddOperation);
    });

    if (!anyDeleted) {
        menu.addAction(Tr::tr("Open File"), this, [targets] {
            for (const Target &t : targets)
                EditorManager::openEditor(t.absolutePath);
        });
    }
    // Diff, Log and Annotate inspect a single file.
    if (single && !allUntracked) {
        const Target t = targets.first();
        menu.addAction(Tr::tr("Diff"), this, [t] {
            t.vc->diffChangedFile(t.repository, t.status.relativePath, t.status.section);
        });
    }
    menu.addSeparator();

    if (allSupportStaging) {
        if (allStaged) {
            menu.addAction(Tr::tr("Unstage"), this, [targets] {
                for (const Target &t : targets)
                    t.vc->unstageFile(t.repository, t.status.relativePath);
            });
        } else if (noneStaged && noneUntracked && noneUnmerged) {
            // Untracked and unmerged files are handled by vcsFillFileActionMenu()
            // below, which offers "Add"/"Stage" and "Mark Conflicts Resolved".
            menu.addAction(Tr::tr("Stage"), this, [targets] {
                for (const Target &t : targets)
                    t.vc->stageFile(t.repository, t.status.relativePath);
            });
        }
    } else if (allUntracked && allSupportAdd) {
        menu.addAction(Tr::tr("Add"), this, [targets] {
            for (const Target &t : targets)
                t.vc->vcsAdd(t.absolutePath);
        });
    }
    // Hidden for rows in the Staged section: revertChangedFile() only reverts the
    // working tree changes, so unstaging first is the natural flow there.
    if (noneUntracked && noneStaged) {
        menu.addAction(Tr::tr("Revert..."), this, [this, targets] {
            const QString question = targets.size() == 1
                ? Tr::tr("Discard the changes to \"%1\"?").arg(targets.first().status.relativePath)
                : Tr::tr("Discard the changes to %n selected files?", nullptr, targets.size());
            const QMessageBox::StandardButton answer = QMessageBox::question(
                this, Tr::tr("Revert"), question,
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return;
            for (const Target &t : targets)
                t.vc->revertChangedFile(t.repository, t.status.relativePath);
        });
    }
    if (single) {
        const Target t = targets.first();
        const FilePath relativePath = FilePath::fromString(t.status.relativePath);
        if (t.status.state != VcsFileState::Untracked) {
            menu.addAction(Tr::tr("Log"), this, [t, relativePath] {
                t.vc->vcsLog(t.repository, relativePath);
            });
            if (t.status.state != VcsFileState::Deleted
                && t.vc->supportsOperation(IVersionControl::AnnotateOperation)) {
                menu.addAction(Tr::tr("Annotate"), this, [t] {
                    t.vc->vcsAnnotate(t.absolutePath, 1);
                });
            }
        }
        // Untracked and unmerged files additionally get the state-specific
        // actions of the version control (add to ignore file, merge tool, ...) -
        // the staged/unstaged distinction is irrelevant for those states.
        if (t.status.state == VcsFileState::Untracked
            || t.status.state == VcsFileState::Unmerged) {
            menu.addSeparator();
            t.vc->vcsFillFileActionMenu(&menu, t.repository, relativePath, t.status.state);
        }
    }

    // Refresh the affected repositories after any action that may have altered
    // their state.
    if (menu.exec(m_treeView->mapToGlobal(point))) {
        FilePaths refreshed;
        for (const Target &t : targets) {
            if (!refreshed.contains(t.repository)) {
                refreshed.append(t.repository);
                t.vc->requestRepositoryStatus(t.repository);
            }
        }
    }
}

ChangesViewFactory::ChangesViewFactory()
{
    setDisplayName(Tr::tr("Changes"));
    setPriority(450);
    setId(Constants::VCS_CHANGES_VIEW_ID);
}

NavigationView ChangesViewFactory::createWidget()
{
    auto view = new ChangesView;
    return {view, view->createToolButtons()};
}

} // namespace VcsBase::Internal

#include "vcschangesview.moc"
