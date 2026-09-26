// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "branchview.h"

#include "branchadddialog.h"
#include "branchcheckoutdialog.h"
#include "branchmodel.h"
#include "gitclient.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gittr.h"
#include "gitutils.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/inavigationwidgetfactory.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <utils/elidinglabel.h>
#include <utils/fancylineedit.h>
#include <utils/navigationtreeview.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QApplication>
#include <QCursor>
#include <QHeaderView>
#include <QLabel>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPoint>
#include <QSortFilterProxyModel>
#include <QStyle>
#include <QTreeView>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>

#include <algorithm>

using namespace Core;
using namespace QtTaskTree;
using namespace Utils;
using namespace Utils::StyleHelper::SpacingTokens;
using namespace VcsBase;

#ifdef WITH_TESTS
#include <utils/guitest.h>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#endif

namespace Git::Internal {

class BranchFilterModel : public QSortFilterProxyModel
{
public:
    BranchFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        QAbstractItemModel *m = sourceModel();
        // Filter leaves only. The root node and all intermediate nodes should always be visible
        if (!sourceParent.isValid() || m->rowCount(m->index(sourceRow, 0, sourceParent)) > 0)
            return true;
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
};

static QFont branchStatusFont(const QFont &baseFont)
{
    QFont font = StyleHelper::uiFont(StyleHelper::UiElementCaptionStrong);
    font.setResolveMask(QFont::WeightResolved);
    return font.resolve(baseFont);
}

struct BranchStatusBadge
{
    QString label;
    QString compactLabel;
    Theme::Color color;
};

static QList<BranchStatusBadge> statusBadges(const QModelIndex &source)
{
    QList<BranchStatusBadge> badges;
    if (const int ahead = source.data(BranchModel::AheadRole).toInt(); ahead > 0)
        badges.append({QString("\u2191 %1").arg(ahead), QString("\u2191%1").arg(ahead),
                       Theme::Token_Notification_Success_Default});
    if (const int behind = source.data(BranchModel::BehindRole).toInt(); behind > 0)
        badges.append({QString("\u2193 %1").arg(behind), QString("\u2193%1").arg(behind),
                       Theme::Token_Notification_Success_Default});
    if (const int modified = source.data(BranchModel::ModifiedRole).toInt(); modified > 0)
        badges.append({QString("\u00b1%1").arg(modified), QString("\u00b1%1").arg(modified),
                       Theme::Token_Notification_Alert_Default});
    return badges;
}

static int statusBadgeWidth(const BranchStatusBadge &badge, const QFontMetrics &metrics)
{
    return metrics.horizontalAdvance(badge.label) + metrics.height() + GapHXs;
}

static int statusBadgesWidth(const QList<BranchStatusBadge> &badges, const QFontMetrics &metrics)
{
    int width = 0;
    for (const BranchStatusBadge &badge : badges)
        width += statusBadgeWidth(badge, metrics) + GapHXs;
    return width;
}

class BranchVisualDelegate final : public BranchValidationDelegate
{
public:
    BranchVisualDelegate(BranchModel *model, BranchFilterModel *filterModel,
                         QTreeView *view, QWidget *parent)
        : BranchValidationDelegate(parent, model)
        , m_model(model)
        , m_filterModel(filterModel)
        , m_view(view)
    {}

    QSize sizeHint(const QStyleOptionViewItem &viewOption, const QModelIndex &index) const override
    {
        const QStyleOptionViewItem option = setOptions(index, viewOption);
        QSize size = BranchValidationDelegate::sizeHint(option, index);
        if (m_model->isLeaf(m_filterModel->mapToSource(index))) {
            const QModelIndex source = m_filterModel->mapToSource(index);
            const QString tracking = m_model->tracking(source);
            const bool hasTracking = !tracking.isEmpty();
            const bool dateBelow = needsDateLine(option, index);
            const int lineHeight = option.fontMetrics.height() + GapHXs;
            const int lineCount = 1 + hasTracking + dateBelow;
            size.setHeight(qMax(size.height(), lineHeight * lineCount + PaddingVXxs));
            QFont nameFont = option.font;
            const QFontMetrics nameMetrics(nameFont);
            const QFont badgeFont = branchStatusFont(option.font);
            const QFontMetrics badgeMetrics(badgeFont);
            const QString name = source.data(BranchModel::BranchNameRole).toString();
            int firstLineWidth = nameMetrics.horizontalAdvance(name);
            firstLineWidth += statusBadgesWidth(statusBadges(source), badgeMetrics);
            const int trackingWidth = QFontMetrics(option.font).horizontalAdvance(
                QString("\u21b3 %1").arg(tracking));
            size.setWidth(qMax(firstLineWidth, trackingWidth) + PaddingHL + PaddingHS);
        } else {
            size.setHeight(qMax(size.height(), option.fontMetrics.height() + PaddingVS));
        }
        return size;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &viewOption,
               const QModelIndex &index) const override
    {
        const QStyleOptionViewItem option = setOptions(index, viewOption);
        const QModelIndex sourceIndex = m_filterModel->mapToSource(index);
        if (!m_model->isLeaf(sourceIndex)) {
            BranchValidationDelegate::paint(painter, option, index);
            return;
        }

        painter->save();
        painter->setClipRect(option.rect, Qt::IntersectClip);
        const bool selected = option.state & QStyle::State_Selected;
        QStyleOptionViewItem background = option;
        if (selected)
            background.state |= QStyle::State_Selected;
        background.text.clear();
        background.icon = {};
        QStyle *style = option.widget ? option.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &background, painter, option.widget);

        const bool current = sourceIndex == m_model->currentBranch();
        const Theme::Color colorId = m_model->isHead(sourceIndex)
            ? Theme::Token_Accent_Default
            : m_model->isTag(sourceIndex) ? Theme::Token_Notification_Alert_Default
            : m_model->isLocal(sourceIndex) ? Theme::Token_Notification_Success_Default
                                            : Theme::Token_Notification_Neutral_Default;
        const QColor color = Utils::creatorColor(colorId);
        if (current && !selected) {
            QColor fill = color;
            fill.setAlphaF(0.12f);
            QColor outline = color;
            outline.setAlphaF(0.45f);
            StyleHelper::drawCardBg(painter, option.rect.adjusted(PaddingHXxs,
                                            PaddingVXxs / 2,
                                            -PaddingHXxs,
                                            -PaddingVXxs / 2), fill,
                                    QPen(outline, 1), RadiusS);
        }

        const QColor textColor = option.palette.color(selected ? QPalette::HighlightedText
                                                               : QPalette::Text);
        const QFont badgeFont = branchStatusFont(option.font);
        const QFontMetrics badgeMetrics(badgeFont);
        const int badgeHeight = qMin(badgeMetrics.height() + GapHXs, option.rect.height() - PaddingVXxs);
        painter->setRenderHint(QPainter::Antialiasing);
        const QString tracking = m_model->tracking(sourceIndex);
        const bool hasTracking = !tracking.isEmpty();
        const QString date = index.siblingAtColumn(1).data().toString();
        const bool dateBelow = needsDateLine(option, index);
        const int firstLineHeight = option.fontMetrics.height() + GapHXs;
        const int firstLineCenter = hasTracking || dateBelow
                                        ? option.rect.top() + firstLineHeight / 2
                                        : option.rect.center().y();
        painter->setPen(QPen(color, 1.5));
        painter->setBrush(current ? QBrush(color) : QBrush(Qt::NoBrush));
        painter->drawEllipse(QPointF(option.rect.left() + PaddingHS + GapHXxs / 2, firstLineCenter), PrimitiveS / 2,
                             PrimitiveS / 2);

        const int textLeft = option.rect.left() + PaddingHL + GapHXxs;
        QFont nameFont = option.font;
        QList<BranchStatusBadge> badges = statusBadges(sourceIndex);

        const QFontMetrics nameMetrics(nameFont);
        const int rowRight = qMin(option.rect.right(), m_view->viewport()->rect().right());
        const int dateWidth = option.fontMetrics.horizontalAdvance(date);
        const int dateLeft = rowRight - PaddingHXs - dateWidth;
        const int availableWidth = dateBelow || date.isEmpty()
                                       ? rowRight - textLeft - PaddingHXs
                                       : dateLeft - option.fontMetrics.horizontalAdvance(' ') - textLeft;
        int totalBadgeWidth = statusBadgesWidth(badges, badgeMetrics);
        if (totalBadgeWidth > availableWidth) {
            totalBadgeWidth = 0;
            for (BranchStatusBadge &badge : badges) {
                badge.label = badge.compactLabel;
                totalBadgeWidth += statusBadgeWidth(badge, badgeMetrics) + GapHXs;
            }
        }
        while (totalBadgeWidth > availableWidth && badges.size() > 1)
            totalBadgeWidth -= statusBadgeWidth(badges.takeLast(), badgeMetrics) + GapHXs;
        if (totalBadgeWidth > availableWidth) {
            badges.clear();
            totalBadgeWidth = 0;
        }

        const int nameWidth = qMax(0, availableWidth - totalBadgeWidth);
        const QString name = sourceIndex.data(BranchModel::BranchNameRole).toString();
        const QString visibleName = nameMetrics.horizontalAdvance(name) <= nameWidth
                                        ? name : nameMetrics.elidedText(name, Qt::ElideRight,
                                                                       nameWidth);
        painter->setFont(nameFont);
        painter->setPen(textColor);
        const QRect nameRect(textLeft, option.rect.top(), nameWidth,
                             hasTracking || dateBelow ? firstLineHeight : option.rect.height());
        painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, visibleName);
        int badgeX = textLeft + nameMetrics.horizontalAdvance(visibleName) + GapHXs;
        painter->setFont(badgeFont);
        for (const BranchStatusBadge &badgeInfo : std::as_const(badges)) {
            const int width = statusBadgeWidth(badgeInfo, badgeMetrics);
            const QRect badge(badgeX, firstLineCenter - badgeHeight / 2, width, badgeHeight);
            QColor border = Utils::creatorColor(badgeInfo.color);
            QColor fill = border;
            const QColor filledText = Utils::creatorColor(Theme::Token_Basic_White);
            if (current) {
                fill = StyleHelper::ensureReadableOn(filledText, fill);
                border = fill;
            } else {
                border.setAlphaF(0.6f);
                fill.setAlphaF(0.16f);
            }
            StyleHelper::drawCardBg(painter, badge, fill, QPen(border, 1), badgeHeight / 2);
            painter->setPen(current ? filledText : selected ? textColor
                                               : Utils::creatorColor(Theme::Token_Text_Muted));
            painter->drawText(badge, Qt::AlignCenter, badgeInfo.label);
            badgeX += width + GapHXs;
        }
        if (hasTracking) {
            painter->setFont(option.font);
            painter->setPen(selected ? textColor
                                     : Utils::creatorColor(Theme::Token_Text_Muted));
            const QRect trackingRect(textLeft, option.rect.top() + firstLineHeight,
                                     qMax(0, rowRight - textLeft - PaddingHXs),
                                     firstLineHeight);
            painter->drawText(trackingRect, Qt::AlignVCenter | Qt::AlignLeft,
                              QFontMetrics(option.font).elidedText(QString("\u21b3 %1").arg(tracking),
                                                                   Qt::ElideMiddle,
                                                                   trackingRect.width()));
        }
        if (dateBelow) {
            painter->setFont(option.font);
            painter->setPen(selected ? textColor
                                     : Utils::creatorColor(Theme::Token_Text_Muted));
            const int dateLine = hasTracking ? 2 : 1;
            const QRect dateRect(textLeft, option.rect.top() + firstLineHeight * dateLine,
                                 qMax(0, rowRight - textLeft - PaddingHXs), firstLineHeight);
            painter->drawText(dateRect, Qt::AlignVCenter | Qt::AlignLeft, date);
        } else if (!date.isEmpty()) {
            painter->setFont(option.font);
            painter->setPen(selected ? textColor
                                     : Utils::creatorColor(Theme::Token_Text_Muted));
            const QRect dateRect(dateLeft, option.rect.top(), dateWidth,
                                 hasTracking ? firstLineHeight : option.rect.height());
            painter->drawText(dateRect, Qt::AlignVCenter | Qt::AlignRight, date);
        }
        painter->restore();
    }

private:
    bool needsDateLine(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        const QString date = index.siblingAtColumn(1).data().toString();
        if (date.isEmpty())
            return false;

        const QModelIndex source = m_filterModel->mapToSource(index);
        QFont nameFont = option.font;
        const QFont badgeFont = branchStatusFont(option.font);
        const QFontMetrics badgeMetrics(badgeFont);
        const int branchWidth = QFontMetrics(nameFont).horizontalAdvance(
                                    source.data(BranchModel::BranchNameRole).toString())
                                + statusBadgesWidth(statusBadges(source), badgeMetrics);

        int depth = 0;
        for (QModelIndex parent = index; parent.isValid(); parent = parent.parent())
            ++depth;
        const int textLeft = depth * m_view->indentation() + PaddingHL + GapHXxs;
        const int dateWidth = option.fontMetrics.horizontalAdvance(date);
        const int spaceWidth = option.fontMetrics.horizontalAdvance(' ');
        return textLeft + branchWidth + spaceWidth + dateWidth + GapHXs
               > m_view->viewport()->width();
    }

    BranchModel *m_model;
    BranchFilterModel *m_filterModel;
    QTreeView *m_view;
};

struct SetInContext
{
    explicit SetInContext(BranchModel *model) : m_model(model)
    {
        m_model->setRefreshBlocked(true);
    }
    SetInContext(const SetInContext &) = delete;
    SetInContext &operator=(const SetInContext &) = delete;
    SetInContext(SetInContext &&) = delete;
    SetInContext &operator=(SetInContext &&) = delete;
    ~SetInContext() { if (m_model) m_model->setRefreshBlocked(false); }
    QPointer<BranchModel> m_model;
};

static QModelIndex findBranchIndex(BranchModel *model, const QString &ref, const QString &hash)
{
    QList<QModelIndex> parents{QModelIndex{}};
    while (!parents.isEmpty()) {
        const QModelIndex parent = parents.takeLast();
        for (int row = 0; row < model->rowCount(parent); ++row) {
            const QModelIndex index = model->index(row, 0, parent);
            if (model->fullName(index, true) == ref && model->hash(index) == hash)
                return index;
            parents.append(index);
        }
    }
    return {};
}

static BranchModel *globalModel = nullptr;

static BranchModel *branchModel()
{
    if (!globalModel)
        globalModel = new BranchModel;
    return globalModel;
}

BranchView::BranchView(BranchViewFactory *factory)
    : m_includeOldEntriesAction(new QAction(Tr::tr("Include Old Entries"), this))
    , m_includeTagsAction(new QAction(Tr::tr("Include Tags"), this))
    , m_addAction(new QAction(this))
    , m_refreshAction(new QAction(this))
    , m_repositoryLabel(new ElidingLabel(this))
    , m_filterEdit(new FancyLineEdit(this))
    , m_branchView(new NavigationTreeView(this))
    , m_model(branchModel())
    , m_filterModel(new BranchFilterModel(this))
    , m_factory(factory)
{
    m_treeStateRepository = m_model->repository();
    m_addAction->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
    connect(m_addAction, &QAction::triggered, this, &BranchView::add);

    m_refreshAction->setIcon(Utils::Icons::RELOAD_TOOLBAR.icon());
    m_refreshAction->setToolTip(Tr::tr("Refresh"));
    connect(m_refreshAction, &QAction::triggered, m_model, &BranchModel::refreshCurrentRepository);

    m_branchView->setHeaderHidden(true);
    m_branchView->setUniformRowHeights(false);
    m_branchView->setIndentation(m_branchView->fontMetrics().height());
    m_branchView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_branchView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocus();

    m_repositoryLabel->setElideMode(Qt::ElideLeft);

    connect(m_model, &QAbstractItemModel::modelAboutToBeReset,
            this, &BranchView::saveTreeState);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterRole(Qt::EditRole);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_branchView->setModel(m_filterModel);
    m_branchView->setColumnHidden(1, true);
    m_branchView->header()->setStretchLastSection(false);
    m_branchView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    connect(m_branchView->header(), &QHeaderView::sectionResized,
            this, &BranchView::updateRowLayout);
    m_filterEdit->setFiltering(true);
    m_filterEdit->setPlaceholderText(Tr::tr("Filter branches and tags"));
    connect(m_filterEdit, &FancyLineEdit::textChanged,
            m_filterModel, QOverload<const QString &>::of(&BranchFilterModel::setFilterRegularExpression));
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_filterEdit);
    layout->addWidget(m_repositoryLabel);
    layout->addWidget(m_branchView);
    layout->setContentsMargins(0, 2, 0, 0);
    setLayout(layout);

    m_includeOldEntriesAction->setCheckable(true);
    m_includeOldEntriesAction->setChecked(m_model->oldBranchesIncluded());
    m_includeOldEntriesAction->setToolTip(
                Tr::tr("Include branches and tags that have not been active for %n days.", nullptr,
                   Constants::OBSOLETE_COMMIT_AGE_IN_DAYS));
    connect(m_includeOldEntriesAction, &QAction::toggled,
            this, &BranchView::setIncludeOldEntries);
    connect(m_model, &BranchModel::oldBranchesIncludedChanged,
            m_includeOldEntriesAction, &QAction::setChecked);
    m_includeTagsAction->setCheckable(true);
    m_includeTagsAction->setChecked(settings().showTags());
    connect(m_includeTagsAction, &QAction::toggled,
            this, &BranchView::setIncludeTags);
    connect(m_model, &BranchModel::tagsIncludedChanged,
            m_includeTagsAction, &QAction::setChecked);

    m_branchView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_branchView->setEditTriggers(QAbstractItemView::SelectedClicked
                                  | QAbstractItemView::EditKeyPressed);
    m_branchView->setItemDelegate(new BranchVisualDelegate(m_model, m_filterModel, m_branchView, this));
    connect(m_branchView, &QAbstractItemView::doubleClicked,
            this, [this](const QModelIndex &idx) { log(m_filterModel->mapToSource(idx)); });
    connect(m_branchView, &QWidget::customContextMenuRequested,
            this, &BranchView::slotCustomContextMenu);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &BranchView::restoreTreeState);

    m_branchView->selectionModel()->clear();
    expandAndResize();
    connect(m_model, &BranchModel::repositoryChanged, this, &BranchView::refresh);
    connect(m_model, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight,
                   const QList<int> &roles) {
        if (roles.isEmpty() || roles.contains(Qt::DisplayRole) || roles.contains(Qt::FontRole))
            updateRowLayout();
        if (!QToolTip::isVisible() || (!roles.isEmpty() && !roles.contains(Qt::ToolTipRole)))
            return;
        QWidget *viewport = m_branchView->viewport();
        const QPoint globalPosition = QCursor::pos();
        const QPoint viewportPosition = viewport->mapFromGlobal(globalPosition);
        if (!viewport->rect().contains(viewportPosition))
            return;
        const QModelIndex proxyIndex = m_branchView->indexAt(viewportPosition);
        const QModelIndex sourceIndex = m_filterModel->mapToSource(proxyIndex);
        if (!sourceIndex.isValid() || sourceIndex.parent() != topLeft.parent()
            || sourceIndex.row() < topLeft.row() || sourceIndex.row() > bottomRight.row()
            || sourceIndex.column() < topLeft.column() || sourceIndex.column() > bottomRight.column()) {
            return;
        }
        const QString toolTip = proxyIndex.data(Qt::ToolTipRole).toString();
        if (!toolTip.isEmpty()) {
            QToolTip::showText(globalPosition, toolTip, viewport,
                               m_branchView->visualRect(proxyIndex));
        }
    });
    refresh();
}

void BranchView::refresh()
{
    updateRowLayout();
    const FilePath repository = m_model->repository();

    if (repository.isEmpty()) {
        m_repositoryLabel->setText(Tr::tr("<No repository>"));
        m_addAction->setToolTip(Tr::tr("Create Git Repository..."));
        m_branchView->setEnabled(false);
    } else {
        m_repositoryLabel->setText(repository.toUserOutput());
        m_repositoryLabel->setToolTip(msgRepositoryLabel(repository));
        m_addAction->setToolTip(Tr::tr("Add Branch..."));
        m_branchView->setEnabled(true);
    }
}

void BranchView::showEvent(QShowEvent *)
{
    updateRowLayout();
    if (m_factory)
        m_factory->viewShown(this);
    else
        m_model->refreshCurrentRepository();
}

QList<QToolButton *> BranchView::createToolButtons()
{
    auto filter = new QToolButton;
    filter->setIcon(Utils::Icons::FILTER.icon());
    filter->setToolTip(Tr::tr("Filter"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty(StyleHelper::C_NO_ARROW, true);

    auto filterMenu = new QMenu(filter);
    filterMenu->addAction(m_includeOldEntriesAction);
    filterMenu->addAction(m_includeTagsAction);
    filter->setMenu(filterMenu);

    auto addButton = new QToolButton;
    addButton->setDefaultAction(m_addAction);
    addButton->setProperty(StyleHelper::C_NO_ARROW, true);

    auto refreshButton = new QToolButton;
    refreshButton->setDefaultAction(m_refreshAction);
    refreshButton->setProperty(StyleHelper::C_NO_ARROW, true);

    return {filter, addButton, refreshButton};
}

void BranchView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateRowLayout();
}

void BranchView::updateRowLayout()
{
    if (m_rowLayoutPending)
        return;
    m_rowLayoutPending = true;
    QMetaObject::invokeMethod(this, [this] {
        m_rowLayoutPending = false;
        m_branchView->doItemsLayout();
    }, Qt::QueuedConnection);
}

void BranchView::slotCustomContextMenu(const QPoint &point)
{
    const QModelIndex filteredIndex = m_branchView->indexAt(point);
    if (!filteredIndex.isValid())
        return;

    const QModelIndex index = m_filterModel->mapToSource(filteredIndex);
    const QString indexName = m_model->fullName(index);
    const QModelIndex currentBranch = m_model->currentBranch();
    const QString currentName = m_model->fullName(currentBranch);
    const QString trackingName = m_model->tracking(index);
    const bool currentSelected = index.sibling(index.row(), 0) == currentBranch;
    const bool isLocal = m_model->isLocal(index);
    const bool isTag = m_model->isTag(index);
    const bool hasActions = m_model->isLeaf(index);
    const bool currentLocal = m_model->isLocal(currentBranch);
    QSingleTaskTreeRunner taskTreeRunner;
    QAction *mergeAction = nullptr;

    SetInContext block(m_model);
    QMenu contextMenu;
    if (isLocal || hasActions)
        contextMenu.addAction(Tr::tr("&Add..."), this, &BranchView::add);

    auto addMergeActions = [&, this](const QString &target) {
        mergeAction = contextMenu.addAction(Tr::tr("&Merge \"%1\" into \"%2\"")
                                                .arg(target, currentName),
                                                this,
                                                [this, target] { merge(target, false); });
        taskTreeRunner.start(fastForwardMergeRecipe([&] {
            auto ffMerge = new QAction(
                Tr::tr("&Merge \"%1\" into \"%2\" (Fast-Forward)").arg(target, currentName));
            connect(ffMerge, &QAction::triggered, this, [this, target] { merge(target, true); });
            contextMenu.insertAction(mergeAction, ffMerge);
            mergeAction->setText(Tr::tr("Merge \"%1\" into \"%2\" (No &Fast-Forward)")
                                     .arg(target, currentName));
        }));

        contextMenu.addAction(Tr::tr("&Rebase \"%1\" on \"%2\"")
                                  .arg(currentName, target),
                              this, [this, target] { rebase(target); });
        contextMenu.addSeparator();
    };

    const std::optional<QString> remote = m_model->remoteName(index);
    if (remote.has_value()) {
        contextMenu.addAction(Tr::tr("&Fetch"), this, [this, &remote] {
            gitClient().fetch(m_model->repository(), *remote);
        });
        contextMenu.addSeparator();
        if (!remote->isEmpty()) {
            contextMenu.addAction(Tr::tr("Remove &Stale Branches"), this, [this, &remote] {
                gitClient().removeStaleRemoteBranches(m_model->repository(), *remote);
            });
            contextMenu.addSeparator();
        }
        contextMenu.addAction(Tr::tr("Manage &Remotes..."), this, &manageRemotes);
    }
    if (hasActions) {
        if (!currentSelected && (isLocal || isTag))
            contextMenu.addAction(Tr::tr("Rem&ove..."), this, &BranchView::remove);
        if (isLocal || isTag)
            contextMenu.addAction(Tr::tr("Re&name..."), this, &BranchView::rename);
        if (!currentSelected)
            contextMenu.addAction(Tr::tr("&Checkout"), this, &BranchView::checkout);
        if (isLocal && currentSelected && !trackingName.isEmpty()) {
            contextMenu.addAction(Tr::tr("Stop tracking \"%1\"").arg(trackingName),
                                  this, [this] { m_model->setRemoteTracking({}); });
        }
        contextMenu.addSeparator();
        contextMenu.addAction(Tr::tr("&Diff"), this, [this] {
            const QString fullName = m_model->fullName(selectedIndex(), true);
            if (!fullName.isEmpty())
                gitClient().diffBranch(m_model->repository(), fullName);
        });
        contextMenu.addAction(Tr::tr("&Log"), this, [this] { log(selectedIndex()); });
        contextMenu.addAction(Tr::tr("Reflo&g"), this, [this] { reflog(selectedIndex()); });
        contextMenu.addSeparator();

        if (currentSelected && !trackingName.isEmpty())
            addMergeActions(trackingName);

        if (!currentSelected) {
            auto resetMenu = new QMenu(Tr::tr("Re&set"), &contextMenu);
            resetMenu->addAction(Tr::tr("&Hard"), this, [this] { reset("hard"); });
            resetMenu->addAction(Tr::tr("&Mixed"), this, [this] { reset("mixed"); });
            resetMenu->addAction(Tr::tr("&Soft"), this, [this] { reset("soft"); });
            contextMenu.addMenu(resetMenu);

            addMergeActions(indexName);

            contextMenu.addAction(Tr::tr("Cherry-&Pick..."), this, &BranchView::cherryPick);
        }
        if (!currentSelected && !isTag) {
            if (currentLocal) {
                contextMenu.addAction(Tr::tr("&Track"), this, [this] {
                    m_model->setRemoteTracking(selectedIndex());
                });
            }
            if (!isLocal) {
                contextMenu.addSeparator();
                contextMenu.addAction(Tr::tr("&Push"), this, &BranchView::push);
            }
        }
    }
    contextMenu.exec(m_branchView->viewport()->mapToGlobal(point));
}

void BranchView::expandAndResize()
{
    m_branchView->expandAll();
    updateRowLayout();
}

QString BranchView::pathElementForIndex(const QModelIndex &index) const
{
    if (m_model->isLeaf(index))
        return "L" + m_model->fullName(index, true);
    return "G" + index.data(Qt::DisplayRole).toString();
}

QString BranchView::pathForIndex(const QModelIndex &index) const
{
    QStringList path;
    for (QModelIndex current = index; current.isValid(); current = current.parent())
        path.prepend(pathElementForIndex(current));
    return path.join(QChar(0x1f));
}

QModelIndex BranchView::sourceIndexForPath(const QString &path) const
{
    const QStringList parts = path.split(QChar(0x1f), Qt::SkipEmptyParts);
    QModelIndex parent;
    QModelIndex current;
    for (const QString &part : parts) {
        current = {};
        for (int row = 0; row < m_model->rowCount(parent); ++row) {
            const QModelIndex candidate = m_model->index(row, 0, parent);
            if (pathElementForIndex(candidate) == part) {
                current = candidate;
                break;
            }
        }
        if (!current.isValid())
            return {};
        parent = current;
    }
    return current;
}

void BranchView::saveTreeState()
{
    m_treeStateRepository = m_model->repository();
    m_expandedPaths.clear();
    std::function<void(const QModelIndex &)> saveChildren = [&](const QModelIndex &parent) {
        for (int row = 0; row < m_model->rowCount(parent); ++row) {
            const QModelIndex source = m_model->index(row, 0, parent);
            const QModelIndex proxy = m_filterModel->mapFromSource(source);
            if (m_model->rowCount(source) > 0)
                m_expandedPaths.insert(pathForIndex(source), m_branchView->isExpanded(proxy));
            saveChildren(source);
        }
    };
    saveChildren({});
    m_currentPath = pathForIndex(m_filterModel->mapToSource(m_branchView->currentIndex()));
    m_hasSavedTreeState = true;
}

void BranchView::restoreTreeState()
{
    if (m_treeStateRepository != m_model->repository()) {
        m_treeStateRepository = m_model->repository();
        m_expandedPaths.clear();
        m_currentPath.clear();
        m_hasSavedTreeState = false;
    }
    if (!m_hasSavedTreeState) {
        expandAndResize();
        return;
    }

    m_branchView->expandAll();
    const QModelIndex source = sourceIndexForPath(m_currentPath);
    const QModelIndex proxy = m_filterModel->mapFromSource(source);
    if (proxy.isValid()) {
        m_branchView->setCurrentIndex(proxy);
        m_branchView->selectionModel()->select(
            proxy, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    QStringList paths = m_expandedPaths.keys();
    std::sort(paths.begin(), paths.end(), [](const QString &left, const QString &right) {
        return left.count(QChar(0x1f)) < right.count(QChar(0x1f));
    });
    for (const QString &path : std::as_const(paths)) {
        const QModelIndex proxy = m_filterModel->mapFromSource(sourceIndexForPath(path));
        if (proxy.isValid())
            m_branchView->setExpanded(proxy, m_expandedPaths.value(path));
    }
    updateRowLayout();
}

void BranchView::setIncludeOldEntries(bool filter)
{
    m_model->setOldBranchesIncluded(filter);
}

void BranchView::setIncludeTags(bool includeTags)
{
    m_model->setTagsIncluded(includeTags);
}

QModelIndex BranchView::selectedIndex()
{
    QModelIndexList selected = m_branchView->selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return {};
    return m_filterModel->mapToSource(selected.at(0));
}

bool BranchView::add()
{
    if (m_model->repository().isEmpty()) {
        initRepository();
        return true;
    }

    SetInContext block(m_model);
    QModelIndex trackedIndex = selectedIndex();
    QString trackedBranch = m_model->fullName(trackedIndex);
    if (trackedBranch.isEmpty()) {
        trackedIndex = m_model->currentBranch();
        trackedBranch = m_model->fullName(trackedIndex);
    }
    const bool isLocal = m_model->isLocal(trackedIndex);
    const bool isTracked = !m_model->isHead(trackedIndex) && !m_model->isTag(trackedIndex);

    const QStringList localNames = m_model->localBranchNames();

    BranchAddDialog branchAddDialog(localNames, BranchAddDialog::Type::AddBranch, this);

    const QString suggestedName = GitClient::suggestedLocalBranchName(
                m_model->repository(), localNames, trackedBranch,
                isTracked ? GitClient::BranchTargetType::Remote : GitClient::BranchTargetType::Commit);
    branchAddDialog.setBranchName(suggestedName);
    branchAddDialog.setTrackedBranchName(isTracked ? trackedBranch : QString(), !isLocal);
    branchAddDialog.setCheckoutVisible(true);

    if (branchAddDialog.exec() == QDialog::Accepted) {
        const QString name = branchAddDialog.branchName();
        const bool track = branchAddDialog.track();
        const bool checkoutAfter = branchAddDialog.checkout();
        const QPointer<BranchView> self(this);
        m_model->addBranch(name, track, trackedIndex,
                           [self, checkoutAfter](const QModelIndex &idx) {
            if (!self || !idx.isValid() || !self->m_branchView)
                return;
            const FilePath repository = self->m_model->repository();
            const QString ref = self->m_model->fullName(idx, true);
            const QString hash = self->m_model->hash(idx);
            QMetaObject::invokeMethod(self, [self, repository, ref, hash, checkoutAfter] {
                if (!self || self->m_model->repository() != repository)
                    return;
                SetInContext block(self->m_model);
                const QModelIndex index = findBranchIndex(self->m_model, ref, hash);
                if (!index.isValid())
                    return;
                QModelIndex mapped = self->m_filterModel->mapFromSource(index);
                if (!mapped.isValid()) {
                    self->m_filterEdit->clear();
                    mapped = self->m_filterModel->mapFromSource(index);
                }
                if (!mapped.isValid())
                    return;
                self->m_branchView->setCurrentIndex(mapped);
                self->m_branchView->scrollTo(mapped);
                if (checkoutAfter)
                    self->checkout();
            }, Qt::QueuedConnection);
        });
        return true;
    }

    return false;
}

bool BranchView::checkout()
{
    SetInContext block(m_model);
    const FilePath workingDirectory = m_model->repository();
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return false;

    const QModelIndex selected = selectedIndex();
    const QString currentBranch = m_model->fullName(m_model->currentBranch());
    const QString nextBranch = m_model->fullName(selected);
    const QString popMessageStart = QCoreApplication::applicationName() +
            ' ' + nextBranch + "-AutoStash ";


    BranchCheckoutDialog branchCheckoutDialog(this, currentBranch, nextBranch);

    if (gitClient().gitStatus(workingDirectory, StatusModes(NoUntracked | NoSubmodules)) != StatusResult::Changed)
        branchCheckoutDialog.foundNoLocalChanges();

    const QList<Stash> stashes = gitClient().synchronousStashList(workingDirectory);
    for (const Stash &stash : stashes) {
        if (stash.message.startsWith(popMessageStart)) {
            branchCheckoutDialog.foundStashForNextBranch();
            break;
        }
    }

    if (!branchCheckoutDialog.hasLocalChanges() &&
        !branchCheckoutDialog.hasStashForNextBranch()) {
        // No local changes and no Auto Stash - no need to open dialog
        m_model->checkoutBranch(selected);
    } else if (branchCheckoutDialog.exec() == QDialog::Accepted) {

        if (branchCheckoutDialog.makeStashOfCurrentBranch()) {
            if (gitClient().synchronousStash(workingDirectory, currentBranch + "-AutoStash").isEmpty())
                return false;
        } else if (branchCheckoutDialog.moveLocalChangesToNextBranch()) {
            if (!gitClient().beginStashScope(workingDirectory, "Checkout", NoPrompt))
                return false;
        } else if (branchCheckoutDialog.discardLocalChanges()) {
            if (!gitClient().synchronousReset(workingDirectory))
                return false;
        }

        const auto commandHandler = [workingDirectory,
                                     moveChanges = branchCheckoutDialog.moveLocalChangesToNextBranch(),
                                     popStash = branchCheckoutDialog.popStashOfNextBranch(),
                                     popMessageStart
                                    ](const CommandResult &result) {
            if (moveChanges) {
                gitClient().endStashScope(workingDirectory);
            } else if (popStash
                       && result.result() == ProcessResult::FinishedWithSuccess) {
                QString stashName;
                const QList<Stash> stashes = gitClient().synchronousStashList(workingDirectory);
                for (const Stash &stash : stashes) {
                    if (stash.message.startsWith(popMessageStart)) {
                        stashName = stash.name;
                        break;
                    }
                }
                if (!stashName.isEmpty())
                    gitClient().synchronousStashRestore(workingDirectory, stashName, true);
            }
        };
        m_model->checkoutBranch(selected, commandHandler);
    } else if (branchCheckoutDialog.diffRequested()) {
        gitClient().diffRepository(workingDirectory, {}, {});
    }

    if (QTC_GUARD(m_branchView))
        m_branchView->selectionModel()->clear();
    return true;
}

// Prompt to delete a local branch and do so.
bool BranchView::remove()
{
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branchName = m_model->fullName(selected);
    if (branchName.isEmpty())
        return false;

    const bool isTag = m_model->isTag(selected);
    const FilePath repository = m_model->repository();
    const QString refName = m_model->fullName(selected, true);
    const QString branchHash = m_model->hash(selected);
    const QPointer<BranchView> self(this);
    const auto confirmDelete = [self, repository, branchName, refName, branchHash,
                                isTag](bool wasMerged) {
        if (!self || self->m_model->repository() != repository)
            return;
        const auto currentIndex = [self, refName, branchHash] {
            return findBranchIndex(self->m_model, refName, branchHash);
        };
        if (!currentIndex().isValid())
            return;
        QString message;
        if (isTag)
            message = Tr::tr("Would you like to delete the tag \"%1\"?").arg(branchName);
        else if (wasMerged)
            message = Tr::tr("Would you like to delete the branch \"%1\"?").arg(branchName);
        else
            message = Tr::tr("Would you like to delete the <b>unmerged</b> branch \"%1\"?")
                          .arg(branchName);

        if (QMessageBox::question(self, isTag ? Tr::tr("Delete Tag") : Tr::tr("Delete Branch"),
                                  message, QMessageBox::Yes | QMessageBox::No,
                                  wasMerged ? QMessageBox::Yes : QMessageBox::No)
            != QMessageBox::Yes) {
            return;
        }
        if (!self || self->m_model->repository() != repository)
            return;
        const QModelIndex index = currentIndex();
        if (!index.isValid() || index == self->m_model->currentBranch())
            return;
        if (isTag)
            self->m_model->removeTag(index);
        else
            self->m_model->removeBranch(index);
    };

    const auto queueConfirmation = [self, confirmDelete](bool merged) {
        if (!self)
            return;
        QMetaObject::invokeMethod(self, [self, confirmDelete, merged] {
            if (!self)
                return;
            SetInContext block(self->m_model);
            confirmDelete(merged);
        }, Qt::QueuedConnection);
    };
    if (isTag)
        queueConfirmation(true);
    else
        m_model->branchIsMerged(selected, queueConfirmation);

    return true;
}

bool BranchView::rename()
{
    const QModelIndex selected = selectedIndex();
    const bool isTag = m_model->isTag(selected);
    QTC_CHECK(m_model->isLocal(selected) || isTag);

    const QString oldName = m_model->fullName(selected);
    QStringList localNames;
    if (!isTag)
        localNames = m_model->localBranchNames();

    const BranchAddDialog::Type type = isTag ? BranchAddDialog::Type::RenameTag
                                             : BranchAddDialog::Type::RenameBranch;
    BranchAddDialog branchAddDialog(localNames, type, this);
    branchAddDialog.setBranchName(oldName);

    branchAddDialog.exec();

    if (branchAddDialog.result() == QDialog::Accepted) {
        if (branchAddDialog.branchName() == oldName)
            return false;
        if (isTag)
            m_model->renameTag(oldName, branchAddDialog.branchName());
        else
            m_model->renameBranch(oldName, branchAddDialog.branchName());
        return true;
    }

    if (QTC_GUARD(m_branchView))
        m_branchView->selectionModel()->clear();
    return false;
}

void BranchView::reset(const QByteArray &resetType)
{
    const QString currentName = m_model->fullName(m_model->currentBranch());
    const QString branchName = m_model->fullName(selectedIndex());
    if (currentName.isEmpty() || branchName.isEmpty())
        return;

    if (QMessageBox::question(this, Tr::tr("Git Reset"), Tr::tr("Reset branch \"%1\" to \"%2\"?")
                              .arg(currentName, branchName),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        gitClient().reset(m_model->repository(), QLatin1String("--" + resetType), branchName);
    }
}

Group BranchView::fastForwardMergeRecipe(const std::function<void()> &callback)
{
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branch = m_model->fullName(selected, true);

    struct FastForwardStorage
    {
        QString mergeBase;
        QString topRevision;
    };

    const Storage<FastForwardStorage> storage;

    const auto onMergeBaseSetup = [repository = m_model->repository(), branch](Process &process) {
        gitClient().setupCommand(process, repository, {"merge-base", "HEAD", branch});
    };
    const auto onMergeBaseDone = [storage](const Process &process) {
        storage->mergeBase = process.cleanedStdOut().trimmed();
    };

    const GroupItem topRevisionProc = gitClient().topRevision(
        m_model->repository(),
        [storage](const QString &revision, const QDateTime &) {
            storage->topRevision = revision;
        });

    const Group root {
        storage,
        parallel,
        ProcessTask(onMergeBaseSetup, onMergeBaseDone, CallDoneFlag::OnSuccess),
        topRevisionProc,
        onGroupDone([storage, callback] {
            if (storage->mergeBase == storage->topRevision)
                callback();
        }, CallDoneFlag::OnSuccess)
    };
    return root;
}

void BranchView::merge(const QString &branch, bool allowFastForward)
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return;
    QTC_ASSERT(!branch.isEmpty(), return);

    if (gitClient().beginStashScope(m_model->repository(), "merge", AllowUnstashed))
        gitClient().synchronousMerge(m_model->repository(), branch, allowFastForward);
}

void BranchView::rebase(const QString &branch)
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return;
    QTC_ASSERT(!branch.isEmpty(), return);

    if (gitClient().beginStashScope(m_model->repository(), "rebase"))
        gitClient().rebase(m_model->repository(), branch);
}

void BranchView::cherryPick()
{
    if (!Core::DocumentManager::saveAllModifiedDocuments())
        return;
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString branch = m_model->fullName(selected, true);
    cherryPickCommits(branch);
}

void BranchView::log(const QModelIndex &idx)
{
    const QString branchName = m_model->fullName(idx, true);
    if (branchName.isEmpty())
        return;
    SetInContext block(m_model);
    gitClient().log(m_model->repository(), QString(), false, {branchName});
}

void BranchView::reflog(const QModelIndex &idx)
{
    const QString branchName = m_model->fullName(idx, true);
    if (branchName.isEmpty())
        return;
    SetInContext block(m_model);
    gitClient().reflog(m_model->repository(), branchName);
}

void BranchView::push()
{
    const QModelIndex selected = selectedIndex();
    QTC_CHECK(selected != m_model->currentBranch());

    const QString fullTargetName = m_model->fullName(selected);
    const int pos = fullTargetName.indexOf('/');

    const QString localBranch = m_model->fullName(m_model->currentBranch());
    const QString remoteName = fullTargetName.left(pos);
    const QString remoteBranch = fullTargetName.mid(pos + 1);
    const QStringList pushArgs = {remoteName, localBranch + ':' + remoteBranch};

    gitClient().push(m_model->repository(), pushArgs);
}

BranchViewFactory::BranchViewFactory()
{
    setDisplayName(Tr::tr("Git Branches"));
    setPriority(500);
    setId(Constants::GIT_BRANCH_VIEW_ID);
    BranchModel *model = branchModel();
    model->setCheckoutRefreshEnabled(false);
    connect(model, &BranchModel::refreshFinished, this, [this, model](bool success) {
        if (model->repository() != m_repository)
            return;
        m_needsRefresh = !success;
    });
}

BranchViewFactory::~BranchViewFactory()
{
    delete globalModel;
    globalModel = nullptr;
}

NavigationView BranchViewFactory::createWidget()
{
    auto view = new BranchView(this);
    m_views.append(view);
    connect(view, &QObject::destroyed, this, [this] {
        m_views.removeIf([](const QPointer<BranchView> &view) { return view.isNull(); });
    });
    return {view, view->createToolButtons()};
}

void BranchViewFactory::refresh(const FilePath &repository)
{
    const BranchModel::ShowError showError = branchModel()->repository() == repository
        ? BranchModel::ShowError::No
        : BranchModel::ShowError::Yes;
    m_repository = repository;
    m_needsRefresh = true;
    if (hasVisibleView())
        branchModel()->refresh(repository, showError, BranchModel::RefreshMode::IfChanged);
}

void BranchViewFactory::refreshIfSame(const FilePath &repository)
{
    if (m_repository != repository)
        return;
    m_needsRefresh = true;
    if (hasVisibleView())
        branchModel()->refreshIfSame(repository);
}

void BranchViewFactory::refreshCurrentBranch()
{
    if (hasVisibleView())
        branchModel()->refreshCurrentBranch();
}

void BranchViewFactory::viewShown(BranchView *view)
{
    if (m_repository.isEmpty())
        m_repository = currentState().topLevel();
    const bool otherViewVisible = std::any_of(m_views.cbegin(), m_views.cend(),
                                             [view](const QPointer<BranchView> &other) {
        return other && other != view && other->isVisible();
    });
    if (m_needsRefresh || !otherViewVisible) {
        branchModel()->refresh(m_repository, BranchModel::ShowError::Yes);
    }
}

bool BranchViewFactory::hasVisibleView() const
{
    return std::any_of(m_views.cbegin(), m_views.cend(), [](const QPointer<BranchView> &view) {
        return view && view->isVisible();
    });
}

#ifdef WITH_TESTS
class BranchViewTest final : public QObject
{
    Q_OBJECT

private slots:
    void testModalRefresh_data();
    void testModalRefresh();
};

void BranchViewTest::testModalRefresh_data()
{
    QTest::addColumn<QString>("operation");
    QTest::newRow("delete confirmation") << QString("remove");
    QTest::newRow("checkout dialog") << QString("checkout");
    QTest::newRow("checkout after creating branch") << QString("add");
}

void BranchViewTest::testModalRefresh()
{
    QFETCH(QString, operation);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const FilePath repository = FilePath::fromString(directory.path());
    const auto run = [repository](const QStringList &args) {
        return gitClient().vcsSynchronousExec(repository, args).result()
               == ProcessResult::FinishedWithSuccess;
    };
    QVERIFY(run({"init", "-b", "main", "."}));
    QVERIFY(run({"config", "user.name", "test"}));
    QVERIFY(run({"config", "user.email", "test@example.invalid"}));
    QVERIFY((repository / "file.txt").writeFileContents("initial\n"));
    QVERIFY(run({"add", "file.txt"}));
    QVERIFY(run({"-c", "commit.gpgsign=false", "commit", "-m", "initial"}));
    QVERIFY(run({"branch", "topic"}));
    if (operation != "remove")
        QVERIFY((repository / "file.txt").writeFileContents("modified\n"));
    BranchView view;
    BranchModel *model = view.m_model;
    const QScopeGuard clearRepository([model] {
        model->refresh({}, BranchModel::ShowError::No, BranchModel::RefreshMode::Force);
    });
    view.show();
    model->refresh(repository, BranchModel::ShowError::No, BranchModel::RefreshMode::Force);
    QTRY_VERIFY(model->currentBranch().isValid());
    const QModelIndex topic = findBranchIndex(model, "refs/heads/topic",
                                             model->hash(model->currentBranch()));
    QVERIFY(topic.isValid());
    view.m_branchView->setCurrentIndex(view.m_filterModel->mapFromSource(topic));
    if (operation == "add") {
        const auto added = GuiTest::onNextDialog([](QWidget *widget) {
            auto edit = widget->findChild<QLineEdit *>();
            QVERIFY(edit);
            edit->setText("created");
            auto buttons = widget->findChild<QDialogButtonBox *>();
            QVERIFY(buttons);
            buttons->button(QDialogButtonBox::Ok)->click();
        });
        QVERIFY(view.add());
        QVERIFY(*added);
    }
    QSignalSpy started(model, &BranchModel::refreshStarted);
    bool blocked = false;
    const auto handled = GuiTest::onNextDialog([&](QWidget *widget) {
        auto dialog = qobject_cast<QDialog *>(widget);
        QVERIFY(dialog);
        if (operation == "remove")
            QCOMPARE(dialog->windowTitle(), QString("Delete Branch"));
        else
            QCOMPARE(dialog->windowTitle(), QString("Checkout branch \"%1\"")
                                               .arg(operation == "add" ? QString("created")
                                                                         : QString("topic")));
        const int before = started.count();
        model->refresh(repository, BranchModel::ShowError::No, BranchModel::RefreshMode::Force);
        blocked = started.count() == before;
        dialog->reject();
    });
    if (operation == "remove")
        QVERIFY(view.remove());
    else if (operation == "checkout")
        view.checkout();
    QTRY_VERIFY(*handled);
    QVERIFY(blocked);
    QTRY_VERIFY(started.count() > 0);
    QTRY_VERIFY(model->currentBranch().isValid());
    QCOMPARE(model->fullName(model->currentBranch()), QString("main"));
    if (operation == "add")
        QVERIFY(model->localBranchNames().contains("created"));
}

QObject *createBranchViewTest()
{
    return new BranchViewTest;
}
#endif

} // Git::Internal

#ifdef WITH_TESTS
#include "branchview.moc"
#endif
