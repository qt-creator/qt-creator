// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitgraphview.h"

#include "gitclient.h"
#include "gitconstants.h"
#include "gitgraphmodel.h"
#include "gitplugin.h"
#include "gittr.h"

#include <coreplugin/documentmanager.h>

#include <utils/algorithm.h>
#include <utils/elidinglabel.h>
#include <utils/fancylineedit.h>
#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/store.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Core;
using namespace Utils;

namespace Git::Internal {

const int maxVisibleLanes = 8;

// One lane is as wide as the tree indentation, so that the lanes drawn next to a
// commit's files line up with the ones next to the commit itself.
static int graphLaneWidth(const QFontMetrics &fontMetrics)
{
    return fontMetrics.height() * 3 / 4;
}

// The token palette offers four chromatic families, each with a default and a
// muted shade. The lanes walk the hues first, so that neighboring lanes differ
// as much as possible, and only then repeat a hue in its muted shade.
static QColor laneColor(int colorIndex)
{
    static const Theme::Color colorIds[] = {
        Theme::Token_Accent_Default,
        Theme::Token_Notification_Neutral_Default,
        Theme::Token_Notification_Alert_Default,
        Theme::Token_Notification_Danger_Default,
        Theme::Token_Text_Muted,
        Theme::Token_Notification_Neutral_Muted,
        Theme::Token_Notification_Alert_Muted,
        Theme::Token_Notification_Danger_Muted,
    };
    return Utils::creatorColor(colorIds[colorIndex % int(std::size(colorIds))]);
}

static QColor refColor(GraphRef::Type type)
{
    switch (type) {
    case GraphRef::Head:
        return Utils::creatorColor(Theme::Token_Accent_Default);
    case GraphRef::LocalBranch:
        return Utils::creatorColor(Theme::Token_Notification_Success_Default);
    case GraphRef::RemoteBranch:
        return Utils::creatorColor(Theme::Token_Notification_Neutral_Default);
    case GraphRef::Tag:
        return Utils::creatorColor(Theme::Token_Notification_Alert_Default);
    }
    return Utils::creatorColor(Theme::Token_Text_Default);
}

static QColor statusColor(const QString &status)
{
    if (status.startsWith('A'))
        return Utils::creatorColor(Theme::Token_Notification_Success_Default);
    if (status.startsWith('D'))
        return Utils::creatorColor(Theme::Token_Notification_Danger_Default);
    if (status.startsWith('M'))
        return Utils::creatorColor(Theme::Token_Notification_Alert_Default);
    return Utils::creatorColor(Theme::Token_Text_Default);
}

class GitGraphFilterModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setFilterText(const QString &text)
    {
        m_filterText = text;
        setFilterFixedString(text);
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (sourceParent.isValid()) // file rows follow their commit
            return filterAcceptsRow(sourceParent.row(), {});
        const auto model = static_cast<GitGraphModel *>(sourceModel());
        QTC_ASSERT(model, return false);
        if (QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent))
            return true;
        const QModelIndex index = model->index(sourceRow, 0, sourceParent);
        return index.data(GitGraphModel::HashRole).toString().startsWith(m_filterText,
                                                                         Qt::CaseInsensitive);
    }

private:
    QString m_filterText;
};

class GitGraphDelegate : public QStyledItemDelegate
{
public:
    GitGraphDelegate(GitGraphModel *model, GitGraphFilterModel *filterModel, QObject *parent)
        : QStyledItemDelegate(parent)
        , m_model(model)
        , m_filterModel(filterModel)
    {}

    void setGraphVisible(bool visible) { m_graphVisible = visible; }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(option.fontMetrics.height() + option.fontMetrics.height() / 3);
        return size;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QModelIndex sourceIndex = m_filterModel->mapToSource(index);
        const int row = sourceIndex.row();
        if (!sourceIndex.parent().isValid() && (row < 0 || row >= m_model->commitCount())) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.text.clear();
        QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        if (sourceIndex.parent().isValid()) {
            paintFileRow(painter, opt, sourceIndex);
            return;
        }
        const CommitEntry &entry = m_model->entryAt(row);

        const QRect rect = opt.rect;
        const QFontMetrics fm = opt.fontMetrics;
        const int laneWidth = graphLaneWidth(fm);
        const int penWidth = qMax(laneWidth / 6, 1);
        const int top = rect.top();
        const int bottom = rect.bottom() + 1;
        const int centerY = top + rect.height() / 2;

        painter->save();
        painter->setClipRect(rect);
        painter->setRenderHint(QPainter::Antialiasing);

        int x = rect.left() + laneWidth / 4;
        if (m_graphVisible) {
            const auto laneX = [&](int lane) {
                return rect.left() + laneWidth / 4 + lane * laneWidth + laneWidth / 2;
            };
            // Lanes beyond the limit are left out rather than folded onto the
            // last one, which would connect commits that have nothing to do
            // with each other. The marker below says that something is missing.
            bool overflow = entry.lane >= maxVisibleLanes;
            painter->setBrush(Qt::NoBrush);
            for (const GraphEdge &edge : entry.edges) {
                if (edge.fromLane >= maxVisibleLanes || edge.toLane >= maxVisibleLanes) {
                    overflow = true;
                    continue;
                }
                painter->setPen(QPen(laneColor(edge.colorIndex), penWidth));
                const int x1 = laneX(edge.fromLane);
                const int x2 = laneX(edge.toLane);
                switch (edge.type) {
                case GraphEdge::Pass:
                    if (x1 == x2) {
                        painter->drawLine(QPoint(x1, top), QPoint(x1, bottom));
                    } else {
                        // The lane moved into one that closed above, so it
                        // crosses over to its new place within this row.
                        QPainterPath path(QPoint(x1, top));
                        path.cubicTo(QPoint(x1, centerY), QPoint(x2, centerY),
                                     QPoint(x2, bottom));
                        painter->drawPath(path);
                    }
                    break;
                case GraphEdge::ToCommit:
                    if (x1 == x2) {
                        painter->drawLine(QPoint(x1, top), QPoint(x1, centerY));
                    } else {
                        QPainterPath path(QPoint(x1, top));
                        path.cubicTo(QPoint(x1, centerY), QPoint(x1, centerY),
                                     QPoint(x2, centerY));
                        painter->drawPath(path);
                    }
                    break;
                case GraphEdge::FromCommit:
                    if (x1 == x2) {
                        painter->drawLine(QPoint(x1, centerY), QPoint(x1, bottom));
                    } else {
                        QPainterPath path(QPoint(x1, centerY));
                        path.cubicTo(QPoint(x2, centerY), QPoint(x2, centerY),
                                     QPoint(x2, bottom));
                        painter->drawPath(path);
                    }
                    break;
                }
            }

            const int radius = laneWidth * 3 / 10;
            if (entry.lane < maxVisibleLanes) {
                const QPoint center(laneX(entry.lane), centerY);
                const QColor nodeColor = laneColor(entry.colorIndex);
                painter->setPen(Qt::NoPen);
                painter->setBrush(nodeColor);
                painter->drawEllipse(center, radius, radius);
                const bool isHead = Utils::anyOf(entry.refs, [](const GraphRef &ref) {
                    return ref.type == GraphRef::Head;
                });
                if (isHead) {
                    painter->setBrush(Qt::NoBrush);
                    painter->setPen(nodeColor);
                    painter->drawEllipse(center.toPointF(), radius + 2.5, radius + 2.5);
                }
            }

            const int columns = qMin(entry.laneCount, maxVisibleLanes) + (overflow ? 1 : 0);
            if (overflow) {
                painter->setFont(opt.font);
                painter->setPen(Utils::creatorColor(Theme::Token_Text_Subtle));
                painter->setBrush(Qt::NoBrush);
                painter->drawText(QRect(laneX(columns - 1) - laneWidth / 2, top,
                                        laneWidth, rect.height()),
                                  Qt::AlignCenter, "…");
            }
            x = rect.left() + laneWidth / 4 + columns * laneWidth + laneWidth / 2;
        }

        const QColor textColor = opt.palette.color(opt.state & QStyle::State_Selected
                                                       ? QPalette::HighlightedText
                                                       : QPalette::Text);
        const QFont refFont = StyleHelper::uiFont(StyleHelper::UiElementCaption);
        const QFontMetrics refFm(refFont);
        const int pillHeight = qMin(refFm.height() + 2, rect.height());
        painter->setFont(refFont);
        for (const GraphRef &ref : entry.refs) {
            const QColor color = refColor(ref.type);
            const QString name = refFm.elidedText(ref.name, Qt::ElideMiddle, laneWidth * 12);
            const QRect pill(x, centerY - pillHeight / 2,
                             refFm.horizontalAdvance(name) + pillHeight, pillHeight);
            QColor fill = color;
            fill.setAlphaF(0.3f);
            StyleHelper::drawCardBg(painter, pill, fill, QPen(color, 1), pillHeight / 2);
            painter->setPen(textColor);
            painter->drawText(pill, Qt::AlignCenter, name);
            x = pill.right() + laneWidth / 2;
            if (x > rect.right())
                break;
        }

        painter->setFont(opt.font);
        painter->setPen(textColor);
        const QString subject = fm.elidedText(entry.subject, Qt::ElideRight, rect.right() - x);
        painter->drawText(QRect(x, top, rect.right() - x, rect.height()),
                          Qt::AlignVCenter | Qt::AlignLeft, subject);
        painter->restore();
    }

private:
    void paintFileRow(QPainter *painter, const QStyleOptionViewItem &opt,
                      const QModelIndex &sourceIndex) const
    {
        const int commitRow = sourceIndex.parent().row();
        const CommitEntry &entry = m_model->entryAt(commitRow);
        const QList<FileChange> &files = m_model->filesAt(commitRow);
        if (sourceIndex.row() < 0 || sourceIndex.row() >= files.size())
            return;
        const FileChange &file = files.at(sourceIndex.row());

        const QRect rect = opt.rect;
        const QFontMetrics fm = opt.fontMetrics;
        const int laneWidth = graphLaneWidth(fm);
        // Half of the commit rows' width, so that the lanes passing the files
        // stay recognizable without competing with the commits' graph.
        const int penWidth = qMax(laneWidth / 12, 1);
        // File rows are indented one tree level further than their commit.
        int indentation = 0;
        if (auto tree = qobject_cast<const QTreeView *>(opt.widget))
            indentation = tree->indentation();

        painter->save();
        // The lanes live left of the item rect, in the indentation area.
        painter->setClipRect(rect.adjusted(-indentation, 0, 0, 0));
        painter->setRenderHint(QPainter::Antialiasing);

        int x = rect.left() + laneWidth / 4;
        if (m_graphVisible) {
            const int left = rect.left() - indentation;
            // Continue the commit's downward lanes through the expanded rows.
            // Lanes ending at the commit have nothing left to continue, and the
            // ones beyond the limit are left out as in the commit's row.
            bool overflow = false;
            for (const GraphEdge &edge : entry.edges) {
                if (edge.type == GraphEdge::ToCommit)
                    continue;
                if (edge.toLane >= maxVisibleLanes) {
                    overflow = true;
                    continue;
                }
                const int laneX = left + laneWidth / 4 + edge.toLane * laneWidth + laneWidth / 2;
                painter->setPen(QPen(laneColor(edge.colorIndex), penWidth));
                painter->drawLine(QPoint(laneX, rect.top()), QPoint(laneX, rect.bottom() + 1));
            }
            const int columns = qMin(entry.laneCount, maxVisibleLanes) + (overflow ? 1 : 0);
            x = qMax(x, left + laneWidth / 4 + columns * laneWidth + laneWidth / 2);
        }

        painter->setFont(opt.font);
        const QString status = file.status.left(1);
        painter->setPen(statusColor(status));
        painter->drawText(QRect(x, rect.top(), rect.right() - x, rect.height()),
                          Qt::AlignVCenter | Qt::AlignLeft, status);
        x += fm.horizontalAdvance(status) + laneWidth / 2;

        const QFont statFont = StyleHelper::uiFont(StyleHelper::UiElementCaption);
        const QFontMetrics statFm(statFont);
        const QString added = file.added < 0 ? QString() : QString("+%1").arg(file.added);
        const QString deleted = file.added < 0 ? QString() : QString("-%1").arg(file.deleted);
        const int pillHeight = qMin(statFm.height() + 2, rect.height());
        const int pillWidth = added.isEmpty()
                                  ? 0
                                  : statFm.horizontalAdvance(added + " " + deleted) + pillHeight;

        painter->setPen(opt.palette.color(opt.state & QStyle::State_Selected
                                              ? QPalette::HighlightedText : QPalette::Text));
        const int pathWidth = rect.right() - x - (pillWidth > 0 ? pillWidth + laneWidth / 2 : 0);
        const QString path = fm.elidedText(file.path, Qt::ElideLeft, pathWidth);
        painter->drawText(QRect(x, rect.top(), pathWidth, rect.height()),
                          Qt::AlignVCenter | Qt::AlignLeft, path);

        if (pillWidth > 0) {
            x += fm.horizontalAdvance(path) + laneWidth / 2;
            const QColor color = Utils::creatorColor(Theme::Token_Text_Subtle);
            QColor fill = color;
            fill.setAlphaF(0.3f);
            const QRect pill(x, rect.center().y() - pillHeight / 2, pillWidth, pillHeight);
            StyleHelper::drawCardBg(painter, pill, fill, QPen(color, 1), pillHeight / 2);

            painter->setFont(statFont);
            int textX = pill.left() + pillHeight / 2;
            const auto drawCount = [&](const QString &text, const QColor &textColor) {
                painter->setPen(textColor);
                painter->drawText(QRect(textX, pill.top(), pill.right() - textX, pill.height()),
                                  Qt::AlignVCenter | Qt::AlignLeft, text);
                textX += statFm.horizontalAdvance(text);
            };
            drawCount(added, statusColor("A"));
            drawCount(" ", Utils::creatorColor(Theme::Token_Text_Default));
            drawCount(deleted, statusColor("D"));
        }
        painter->restore();
    }

    GitGraphModel *m_model = nullptr;
    GitGraphFilterModel *m_filterModel = nullptr;
    bool m_graphVisible = true;
};

struct SetInContext
{
    SetInContext(bool &block) : m_block(block)
    {
        m_origValue = m_block;
        m_block = true;
    }
    ~SetInContext() { m_block = m_origValue; }
    bool &m_block;
    bool m_origValue;
};

GitGraphView::GitGraphView()
    : m_refreshAction(new QAction(this))
    , m_allBranchesAction(new QAction(Tr::tr("All Branches"), this))
    , m_repositoryLabel(new ElidingLabel(this))
    , m_graphView(new NavigationTreeView(this))
    , m_model(new GitGraphModel(this))
    , m_filterModel(new GitGraphFilterModel(this))
{
    m_refreshAction->setIcon(Utils::Icons::RELOAD_TOOLBAR.icon());
    m_refreshAction->setToolTip(Tr::tr("Refresh"));
    connect(m_refreshAction, &QAction::triggered, this, &GitGraphView::refreshCurrentRepository);

    m_allBranchesAction->setCheckable(true);
    m_allBranchesAction->setChecked(m_model->allBranches());
    m_allBranchesAction->setToolTip(Tr::tr("Show commits from all branches."));
    connect(m_allBranchesAction, &QAction::toggled, this, [this](bool checked) {
        if (checked == m_model->allBranches())
            return;
        m_model->setAllBranches(checked);
        refreshCurrentRepository();
    });

    m_repositoryLabel->setElideMode(Qt::ElideLeft);

    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_graphView->setHeaderHidden(true);
    m_graphView->setUniformRowHeights(true);
    m_graphView->setExpandsOnDoubleClick(false); // activating a row shows it
    // The delegate derives the lane width from the item font metrics, so the
    // indentation has to come from the same font.
    m_graphView->setIndentation(graphLaneWidth(m_graphView->fontMetrics()));
    m_graphView->setModel(m_filterModel);
    auto delegate = new GitGraphDelegate(m_model, m_filterModel, this);
    m_graphView->setItemDelegate(delegate);

    // Reaching the end of the loaded history continues it, and a view that is
    // not filled yet keeps asking until it is.
    QScrollBar *scrollBar = m_graphView->verticalScrollBar();
    connect(scrollBar, &QAbstractSlider::valueChanged, this, &GitGraphView::loadMoreIfAtEnd);
    connect(scrollBar, &QAbstractSlider::rangeChanged, this, &GitGraphView::loadMoreIfAtEnd);

    // Show the changed files of whatever commit the user navigates to.
    connect(m_graphView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [this](const QModelIndex &current) {
        if (current.isValid() && !current.parent().isValid())
            m_graphView->expand(current);
    });

    auto filterEdit = new FancyLineEdit(this);
    filterEdit->setFiltering(true);
    filterEdit->setPlaceholderText(Tr::tr("Filter by subject or hash"));
    connect(filterEdit, &FancyLineEdit::textChanged, this, [this, delegate](const QString &text) {
        delegate->setGraphVisible(text.isEmpty());
        m_filterModel->setFilterText(text);
    });

    auto layout = new QVBoxLayout(this);
    layout->addWidget(filterEdit);
    layout->addWidget(m_repositoryLabel);
    layout->addWidget(m_graphView);
    layout->setContentsMargins(0, StyleHelper::SpacingTokens::PaddingVXxs, 0, 0);
    setLayout(layout);

    m_graphView->setContextMenuPolicy(Qt::CustomContextMenu);
    // activated, so that the panel can be driven from the keyboard and honors
    // the user's single or double click setting.
    connect(m_graphView, &QAbstractItemView::activated, this, &GitGraphView::showCommit);
    connect(m_graphView, &QWidget::customContextMenuRequested,
            this, &GitGraphView::slotCustomContextMenu);

    m_repository = currentState().topLevel();
}

void GitGraphView::refreshIfSame(const FilePath &repository)
{
    if (m_repository != repository)
        return;
    if (m_blockRefresh)
        m_postponedRefresh = true;
    else
        refreshCurrentRepository();
}

void GitGraphView::refresh(const FilePath &repository, bool force)
{
    if (m_blockRefresh || (m_repository == repository && !force))
        return;
    // Editors without repository context, e.g. the diffs opened from this
    // view, must not drop the shown history.
    if (repository.isEmpty() && !force && !m_repository.isEmpty())
        return;

    m_repository = repository;
    if (m_repository.isEmpty()) {
        m_repositoryLabel->setText(Tr::tr("<No repository>"));
        m_graphView->setEnabled(false);
    } else {
        m_repositoryLabel->setText(m_repository.toUserOutput());
        m_repositoryLabel->setToolTip(msgRepositoryLabel(m_repository));
        m_graphView->setEnabled(true);
    }

    // Do not refresh the model when the view is hidden
    if (!isVisible())
        return;

    m_model->refresh(m_repository);
}

void GitGraphView::refreshCurrentBranch()
{
    if (m_blockRefresh)
        m_postponedRefresh = true;
    else
        refreshCurrentRepository();
}

void GitGraphView::setAllBranches(bool allBranches)
{
    m_allBranchesAction->setChecked(allBranches);
    m_model->setAllBranches(allBranches);
}

bool GitGraphView::allBranches() const
{
    return m_model->allBranches();
}

QList<QToolButton *> GitGraphView::createToolButtons()
{
    auto filter = new QToolButton;
    filter->setIcon(Utils::Icons::FILTER.icon());
    filter->setToolTip(Tr::tr("Filter"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty(StyleHelper::C_NO_ARROW, true);

    auto filterMenu = new QMenu(filter);
    filterMenu->addAction(m_allBranchesAction);
    filter->setMenu(filterMenu);

    auto refreshButton = new QToolButton;
    refreshButton->setDefaultAction(m_refreshAction);
    refreshButton->setProperty(StyleHelper::C_NO_ARROW, true);

    return {filter, refreshButton};
}

void GitGraphView::showEvent(QShowEvent *)
{
    refreshCurrentRepository();
}

void GitGraphView::refreshCurrentRepository()
{
    refresh(m_repository, true);
}

void GitGraphView::loadMoreIfAtEnd()
{
    // A hidden view has no meaningful scroll range, and would read the whole
    // history in one go.
    if (!m_graphView->isVisible())
        return;
    const QScrollBar *scrollBar = m_graphView->verticalScrollBar();
    if (scrollBar->value() >= scrollBar->maximum())
        m_model->loadMore();
}

void GitGraphView::showCommit(const QModelIndex &index)
{
    const QModelIndex sourceIndex = m_filterModel->mapToSource(index);
    if (!sourceIndex.isValid())
        return;
    {
        SetInContext block(m_blockRefresh);
        const QModelIndex commitIndex
            = sourceIndex.parent().isValid() ? sourceIndex.parent() : sourceIndex;
        const CommitEntry &entry = m_model->entryAt(commitIndex.row());
        // Show the whole commit for commit rows and for files of parentless
        // commits, whose changes cannot be diffed against "<hash>^".
        if (sourceIndex.parent().isValid() && !entry.parents.isEmpty()) {
            const FileChange &file
                = m_model->filesAt(commitIndex.row()).at(sourceIndex.row());
            showFileChanges(entry.hash, file.path, file.oldPath);
        } else {
            gitClient().show(m_repository, entry.hash);
        }
    }
    if (m_postponedRefresh) {
        m_postponedRefresh = false;
        refreshCurrentRepository();
    }
}

void GitGraphView::showFileChanges(const QString &hash, const QString &file,
                                   const QString &oldFile)
{
    const QString parentFile = oldFile.isEmpty() ? file : oldFile;
    gitClient().inlineDiffRevisions(m_repository, m_repository.pathAppended(file),
                                    hash, file, hash + "^", parentFile);
}

void GitGraphView::slotCustomContextMenu(const QPoint &point)
{
    const QModelIndex index = m_graphView->indexAt(point);
    const QString hash = hashAt(index);
    if (hash.isEmpty())
        return;

    {
        // Scoped, so that a refresh postponed by one of the actions is not
        // blocked again when it is carried out below.
        SetInContext block(m_blockRefresh);
        QMenu contextMenu;
        const QString file = index.data(GitGraphModel::FilePathRole).toString();
        if (!file.isEmpty()) {
            const QString oldFile = index.data(GitGraphModel::OldFilePathRole).toString();
            contextMenu.addAction(Tr::tr("Show C&hanges"), this, [this, hash, file, oldFile] {
                showFileChanges(hash, file, oldFile);
            });
            contextMenu.addAction(Tr::tr("Copy File &Path"), this, [file] {
                Utils::setClipboardAndSelection(file);
            });
            contextMenu.addSeparator();
        }
        contextMenu.addAction(Tr::tr("&Show Commit"), this, [this, hash] {
            gitClient().show(m_repository, hash);
        });
        contextMenu.addAction(Tr::tr("&Copy Commit Hash"), this, [hash] {
            Utils::setClipboardAndSelection(hash);
        });
        contextMenu.addSeparator();
        contextMenu.addAction(Tr::tr("Cherry-&Pick Commit"), this, [this, hash] {
            if (Core::DocumentManager::saveAllModifiedDocuments())
                gitClient().cherryPick(m_repository, hash);
        });
        contextMenu.addAction(Tr::tr("Chec&kout Commit"), this, [this, hash] {
            if (Core::DocumentManager::saveAllModifiedDocuments())
                gitClient().checkout(m_repository, hash);
        });
        contextMenu.exec(m_graphView->viewport()->mapToGlobal(point));
    }
    if (m_postponedRefresh) {
        m_postponedRefresh = false;
        refreshCurrentRepository();
    }
}

QString GitGraphView::hashAt(const QModelIndex &filteredIndex) const
{
    if (!filteredIndex.isValid())
        return {};
    return filteredIndex.data(GitGraphModel::HashRole).toString();
}

GitGraphViewFactory::GitGraphViewFactory()
{
    setDisplayName(Tr::tr("Git Log"));
    setPriority(490);
    setId(Constants::GIT_GRAPH_VIEW_ID);
}

GitGraphView *GitGraphViewFactory::view() const
{
    return m_view;
}

NavigationView GitGraphViewFactory::createWidget()
{
    m_view = new GitGraphView;
    return {m_view, m_view->createToolButtons()};
}

const char kBaseKey[] = "GitGraphView.";
const char kAllBranchesKey[] = ".AllBranches";

void GitGraphViewFactory::saveSettings(QtcSettings *settings, int position, QWidget *widget)
{
    QTC_ASSERT(widget, return);
    const auto view = static_cast<GitGraphView *>(widget);
    settings->setValueWithDefault(Utils::numberedKey(kBaseKey, position) + kAllBranchesKey,
                                  view->allBranches(), false);
}

void GitGraphViewFactory::restoreSettings(QtcSettings *settings, int position, QWidget *widget)
{
    QTC_ASSERT(widget, return);
    const auto view = static_cast<GitGraphView *>(widget);
    view->setAllBranches(settings->value(Utils::numberedKey(kBaseKey, position) + kAllBranchesKey,
                                         false).toBool());
}

} // Git::Internal
