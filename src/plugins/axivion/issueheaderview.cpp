// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "issueheaderview.h"

#include "axiviontr.h"

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>

#include <QGuiApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QToolButton>

namespace Axivion::Internal {

constexpr int IconSize = 16;
constexpr int InnerMargin = 4;

static QString infoText()
{
    return Tr::tr("Allows for filters combined with & as logical AND, | as logical OR and "
                  "! as logical NOT. The filters may contain * to match sequences of "
                  "arbitrary characters. If a single filter is quoted with double quotes "
                  "it will be matched on the complete string. Some filter characters "
                  "require quoting of the filter expression with double quotes. If inside "
                  "double quotes you need to escape \" and \\ with a backslash.\n"
                  "Some examples:\n\n"
                  "a matches issues where the value contains the letter 'a'\n"
                  "\"abc\" matches issues where the value is exactly 'abc'\n"
                  "!abc matches issues whose value does not contain 'abc'\n"
                  "(ab | cd) & !ef matches issues with values containing 'ab' or 'cd' but not 'ef'\n"
                  "\"\" matches issues having an empty value in this column\n"
                  "!\"\" matches issues having any non-empty value in this column");
}

static void fixGlobalPosOnScreen(QPoint *globalPos, const QSize &size)
{
    QScreen *qscreen = QGuiApplication::screenAt(*globalPos);
    if (!qscreen)
        qscreen = QGuiApplication::primaryScreen();
    const QRect screen = qscreen->availableGeometry();

    if (globalPos->x() + size.width() > screen.width())
        globalPos->setX(screen.width() - size.width());
    if (globalPos->y() + size.height() > screen.height())
        globalPos->setY(screen.height() - size.height());
    if (globalPos->y() < 0)
        globalPos->setY(0);
}

class FilterPopupWidget : public QFrame
{
public:
    FilterPopupWidget(QWidget *parent, const QString &filter)
        : QFrame(parent)
    {
        setWindowFlags(Qt::Popup);
        setAttribute(Qt::WA_DeleteOnClose);
        Qt::FocusPolicy origPolicy = parent->focusPolicy();
        setFocusPolicy(Qt::NoFocus);
        parent->setFocusPolicy(origPolicy);
        setFocusProxy(parent);

        auto infoButton = new QToolButton(this);
        infoButton->setIcon(Utils::Icons::INFO.icon());
        infoButton->setCheckable(true);
        infoButton->setChecked(true);
        m_lineEdit = new Utils::FancyLineEdit(this);
        m_lineEdit->setClearButtonEnabled(true);
        m_lineEdit->setText(filter);
        // TODO add some pre-check for validity of the expression
        // or handle invalid filter exception correctly
        auto apply = new QPushButton(Tr::tr("Apply"), this);
        auto infoLabel = new QLabel(infoText());
        infoLabel->setWordWrap(true);

        using namespace Layouting;
        Column layout {
            Row { infoButton, m_lineEdit, apply},
            infoLabel
        };
        layout.attachTo(this);

        adjustSize();
        setFixedWidth(size().width());

        const auto onApply = [this] {
            QTC_ASSERT(m_lineEdit, return);
            if (m_applyHook)
               m_applyHook(m_lineEdit->text());
            close();
        };
        connect(infoButton, &QToolButton::toggled, this, [this, infoLabel](bool checked){
            QTC_ASSERT(infoLabel, return);
            infoLabel->setVisible(checked);
            adjustSize();
        });
        connect(m_lineEdit, &Utils::FancyLineEdit::editingFinished,
                this, [this, apply, onApply] {
            if (m_lineEdit->hasFocus() || apply->hasFocus()) // avoid triggering for focus lost
                onApply();
            else
                close();
        });
        connect(apply, &QPushButton::clicked, this, onApply);
    }

    void setOnApply(const std::function<void(const QString &)> &hook) { m_applyHook = hook; }

protected:
    void showEvent(QShowEvent *event) override
    {
        QWidget::showEvent(event);
        if (!event->spontaneous())
            m_lineEdit->setFocus(Qt::PopupFocusReason);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        if (m_handleResizeEvent) { // ignore the first resize event (first layout)
            const int oldHeight = event->oldSize().height();
            const int newHeight = event->size().height();
            const QPoint position = pos();
            move(position.x(), position.y() + oldHeight - newHeight);
        }
        m_handleResizeEvent = true;
    }

private:
    bool m_handleResizeEvent = false;
    Utils::FancyLineEdit *m_lineEdit = nullptr;
    std::function<void(const QString &)> m_applyHook;
};

static QIcon iconForSorted(std::optional<Qt::SortOrder> order)
{
    static const Utils::Icon UNSORTED(
                {{":/axivion/images/sortAsc.png", Utils::Theme::IconsDisabledColor},
                 {":/axivion/images/sortDesc.png", Utils::Theme::IconsDisabledColor}},
                Utils::Icon::MenuTintedStyle);
    static const Utils::Icon SORT_ASC(
                {{":/axivion/images/sortAsc.png", Utils::Theme::PaletteText},
                 {":/axivion/images/sortDesc.png", Utils::Theme::IconsDisabledColor}},
                Utils::Icon::MenuTintedStyle);
    static const Utils::Icon SORT_DESC(
                {{":/axivion/images/sortAsc.png", Utils::Theme::IconsDisabledColor},
                 {":/axivion/images/sortDesc.png", Utils::Theme::PaletteText}},
                Utils::Icon::MenuTintedStyle);
    static const QIcon unsorted = UNSORTED.icon();
    static const QIcon sortedAsc = SORT_ASC.icon();
    static const QIcon sortedDesc = SORT_DESC.icon();

    if (!order)
        return unsorted;
    return *order == Qt::AscendingOrder ? sortedAsc : sortedDesc;
}

static QIcon iconForFilter(bool isActive)
{
    static const Utils::Icon INACTIVE(
                {{":/utils/images/filtericon.png", Utils::Theme::IconsDisabledColor}},
                Utils::Icon::MenuTintedStyle);
    static const Utils::Icon ACTIVE(
                {{":/utils/images/filtericon.png", Utils::Theme::PaletteText}},
                Utils::Icon::MenuTintedStyle);
    static const QIcon inactive = INACTIVE.icon();
    static const QIcon active = ACTIVE.icon();
    return isActive ? active : inactive;
}

void IssueHeaderView::setColumnInfoList(const QList<ColumnInfo> &infos)
{
    m_columnInfoList = infos;
    const QList<int> oldIndexes = m_currentSortIndexes;
    m_currentSortIndexes.clear();
    for (int i = 0; i < infos.size(); ++i)
        m_columnInfoList[i].sortOrder.reset();
    for (int oldIndex : oldIndexes)
        headerDataChanged(Qt::Horizontal, oldIndex, oldIndex);
}

const QString IssueHeaderView::currentSortString() const
{
    QString sort;
    for (int i : m_currentSortIndexes) {
        QTC_ASSERT(i >= 0 && i < m_columnInfoList.size(), return {});
        if (!sort.isEmpty())
            sort.append(',');
        const ColumnInfo info = m_columnInfoList.at(i);
        sort.append(info.key + (info.sortOrder == Qt::AscendingOrder ? " asc" : " desc"));
    }
    return sort;
}

const QMap<QString, QString> IssueHeaderView::currentFilterMapping() const
{
    QMap<QString, QString> filter;
    for (int i = 0, end = m_columnInfoList.size(); i < end; ++i) {
        const ColumnInfo ci = m_columnInfoList.at(i);
        if (ci.filter.has_value())
            filter.insert("filter_" + ci.key, QString::fromUtf8(QUrl::toPercentEncoding(*ci.filter)));
    }
    return filter;
}

void IssueHeaderView::updateExistingColumnInfos(
        const std::map<QString, QString> &filters,
        const std::optional<std::vector<Dto::SortInfoDto>> &sorters)
{
    // update filters..
    for (int i = 0, end = m_columnInfoList.size(); i < end; ++i) {
        ColumnInfo &info = m_columnInfoList[i];
        const auto filterItem = filters.find(info.key);
        if (filterItem == filters.end())
            info.filter.reset();
        else
            info.filter.emplace(filterItem->second);

        if (sorters) { // ..and sorters if needed
            bool found = false;
            for (const Dto::SortInfoDto &dto : *sorters) {
                if (dto.key != info.key)
                    continue;
                info.sortOrder = dto.getDirectionEnum() == Dto::SortDirection::asc
                        ? Qt::AscendingOrder : Qt::DescendingOrder;
                found = true;
            }
            if (!found)
                info.sortOrder.reset();
        } else { // or clear them
            info.sortOrder.reset();
        }
    }

    // update sort order
    m_currentSortIndexes.clear();
    if (sorters) {
        for (const Dto::SortInfoDto &dto : *sorters) {
            int index = Utils::indexOf(m_columnInfoList, [key = dto.key](const ColumnInfo &ci) {
                return ci.key == key;
            });
            if (index == -1) // legit
                continue;
            m_currentSortIndexes << index;
        }
    }
    // inform UI
    emit filterChanged();
}

void IssueHeaderView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPoint position = event->position().toPoint();
        const int y = position.y();
        if (y > 1 && y < height() - 2) { // TODO improve
            const int pos = position.x();
            const int logical = logicalIndexAt(pos);
            QTC_ASSERT(logical > -1 && logical < m_columnInfoList.size(),
                       QHeaderView::mousePressEvent(event); return);
            m_lastToggleLogicalPos = logical;
            const int margin = style()->pixelMetric(QStyle::PM_HeaderGripMargin, nullptr, this);
            const int lastIconEnd = sectionViewportPosition(logical) + sectionSize(logical) - margin;
            const int lastIconStart = lastIconEnd - IconSize;
            const int firstIconStart = lastIconStart - InnerMargin - IconSize;

            const ColumnInfo info = m_columnInfoList.at(logical);
            if (info.sortable && info.filterable) {
                if (firstIconStart < pos && firstIconStart + IconSize > pos)
                    m_maybeToggle.emplace(Sort);
                else if (lastIconStart < pos && lastIconEnd > pos)
                    m_maybeToggle.emplace(Filter);
                else
                    m_maybeToggle.reset();
            } else {
                if (lastIconStart < pos && lastIconEnd > pos)
                    m_maybeToggle.emplace(info.sortable ? Sort : Filter);
            }
            m_withShift = event->modifiers() == Qt::ShiftModifier;
        }
    }
    QHeaderView::mousePressEvent(event);
}

void IssueHeaderView::mouseReleaseEvent(QMouseEvent *event)
{
    bool dontSkip = !m_dragging && m_maybeToggle;
    const int toggleMode = m_maybeToggle ? *m_maybeToggle : -1;
    bool withShift = m_withShift && event->modifiers() == Qt::ShiftModifier;
    m_dragging = false;
    m_maybeToggle.reset();
    m_withShift = false;

    if (dontSkip) {
        const QPoint position = event->position().toPoint();
        const int y = position.y();
        const int logical = logicalIndexAt(position.x());
        if (logical == m_lastToggleLogicalPos
                && logical > -1 && logical < m_columnInfoList.size()) {
            if (toggleMode == Sort && m_columnInfoList.at(logical).sortable) { // ignore non-sortable
                if (y < height() / 2) // TODO improve
                    onToggleSort(logical, Qt::AscendingOrder, withShift);
                else
                    onToggleSort(logical, Qt::DescendingOrder, withShift);
            } else if (toggleMode == Filter && m_columnInfoList.at(logical).filterable) {
                // TODO we need some popup for text input (entering filter expression)
                // apply them to the columninfo, and use them for the search..
                const auto onApply = [this, logical](const QString &txt) {
                    if (txt.isEmpty())
                        m_columnInfoList[logical].filter.reset();
                    else
                        m_columnInfoList[logical].filter.emplace(txt);
                    headerDataChanged(Qt::Horizontal, logical, logical);
                    emit filterChanged();
                };
                auto popup = new FilterPopupWidget(this, m_columnInfoList.at(logical).filter.value_or(""));
                popup->setOnApply(onApply);
                const int right = sectionViewportPosition(logical) + sectionSize(logical);
                const QSize size = popup->sizeHint();
                QPoint globalPos = mapToGlobal(QPoint{std::max(0, x() + right - size.width()),
                                                      this->y() - size.height()});
                fixGlobalPosOnScreen(&globalPos, size);
                popup->move(globalPos);
                popup->show();
            }
        }
    }
    m_lastToggleLogicalPos = -1;
    QHeaderView::mouseReleaseEvent(event);
}

void IssueHeaderView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragging = true;
    QHeaderView::mouseMoveEvent(event);
}

void IssueHeaderView::onToggleSort(int index, Qt::SortOrder order, bool multi)
{
    QTC_ASSERT(index > -1 && index < m_columnInfoList.size(), return);
    const QList<int> oldSortIndexes = m_currentSortIndexes;
    std::optional<Qt::SortOrder> oldSortOrder = m_columnInfoList.at(index).sortOrder;
    int pos = m_currentSortIndexes.indexOf(index);

    if (oldSortOrder == order)
        m_columnInfoList[index].sortOrder.reset();
    else
        m_columnInfoList[index].sortOrder = order;
    if (multi) {
        if (pos == -1)
            m_currentSortIndexes.append(index);
        else if (oldSortOrder == order)
            m_currentSortIndexes.remove(pos);
    } else {
        m_currentSortIndexes.clear();
        if (pos == -1 || oldSortOrder != order)
            m_currentSortIndexes.append(index);
        for (int oldIndex : oldSortIndexes) {
            if (oldIndex == index)
                continue;
            m_columnInfoList[oldIndex].sortOrder.reset();
        }
    }

    for (int oldIndex : oldSortIndexes)
        headerDataChanged(Qt::Horizontal, oldIndex, oldIndex);
    headerDataChanged(Qt::Horizontal, index, index);
    emit sortTriggered();
}

QSize IssueHeaderView::sectionSizeFromContents(int logicalIndex) const
{
    const QSize oldSize = QHeaderView::sectionSizeFromContents(logicalIndex);
    QTC_ASSERT(logicalIndex > -1 && logicalIndex < m_columnInfoList.size(), return oldSize);
    const ColumnInfo ci = m_columnInfoList.at(logicalIndex);
    const QSize newSize = QSize(qMax(ci.width, oldSize.width()), oldSize.height());

    if (!ci.filterable && !ci.sortable)
        return newSize;

    const int margin = style()->pixelMetric(QStyle::PM_HeaderGripMargin, nullptr, this);
    // compute additional space for needed icon(s)
    int additionalWidth = IconSize + margin;
    if (ci.filterable && ci.sortable)
        additionalWidth += IconSize + InnerMargin;

    return QSize{newSize.width() + additionalWidth, qMax(newSize.height(), IconSize)};
}

void IssueHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    painter->save();
    QHeaderView::paintSection(painter, rect, logicalIndex);
    painter->restore();
    if (logicalIndex < 0 || logicalIndex >= m_columnInfoList.size())
        return;
    const ColumnInfo info = m_columnInfoList.at(logicalIndex);
    if (!info.sortable && !info.filterable)
        return;

    const int offset = qMax((rect.height() - IconSize), 0) / 2;
    const int margin = style()->pixelMetric(QStyle::PM_HeaderGripMargin, nullptr, this);
    const int lastIconLeft = rect.left() + rect.width() - IconSize - margin;
    const int firstIconLeft = lastIconLeft - InnerMargin - IconSize;

    const bool bothIcons = info.sortable && info.filterable;
    const QRect clearRect(bothIcons ? firstIconLeft : lastIconLeft, 0,
                          bothIcons ? IconSize + InnerMargin + IconSize + margin
                                    : IconSize + margin, rect.height());
    painter->save();
    QStyleOptionHeader opt;
    initStyleOption(&opt);
    opt.rect = clearRect;
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
    painter->restore();
    QRect iconRect(lastIconLeft, offset, IconSize, IconSize);
    if (info.filterable) {
        const QIcon icon = iconForFilter(info.filter.has_value());
        icon.paint(painter, iconRect);
        iconRect.setRect(firstIconLeft, offset, IconSize, IconSize);
    }
    if (info.sortable) {
        const QIcon icon = iconForSorted(m_currentSortIndexes.contains(logicalIndex) ? info.sortOrder
                                                                                     : std::nullopt);
        icon.paint(painter, iconRect);
    }
}

} // namespace Axivion::Internal
