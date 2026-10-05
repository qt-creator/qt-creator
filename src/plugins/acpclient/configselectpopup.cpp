// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "configselectpopup.h"

#include "acpclienttr.h"

#include <utils/fuzzymatcher.h>
#include <utils/icon.h>
#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QKeyEvent>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QScreen>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <utility>

using namespace Utils;
using namespace Utils::StyleHelper;
using namespace Utils::StyleHelper::SpacingTokens;

namespace AcpClient::Internal {

enum ItemRole {
    ValueRole = Qt::UserRole + 1,
    DescriptionRole,
    FavoriteRole,
    CurrentRole,
    HeaderRole,
};

enum {
    IconSize = 16,
    MinPopupWidth = 320,
    MaxPopupWidth = 560,
    MinListHeight = 80,
    MaxListHeight = 360,
};

constexpr TextFormat headerTf
    {Theme::Token_Text_Muted, UiElementH6Capital,
     Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip};
constexpr TextFormat nameTf
    {Theme::Token_Text_Default, UiElementLabelMedium,
     Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip};
constexpr TextFormat currentNameTf
    {Theme::Token_Text_Accent, nameTf.uiElement, nameTf.drawTextFlags};
constexpr TextFormat descriptionTf
    {Theme::Token_Text_Muted, UiElementCaption, nameTf.drawTextFlags};

static QIcon favoriteIcon(bool on)
{
    static const QIcon onIcon
        = Icon({{":/utils/images/pinned_small.png", Theme::Token_Text_Accent}}, Icon::Tint).icon();
    static const QIcon offIcon
        = Icon({{":/utils/images/pinned_small.png", Theme::Token_Text_Muted}}, Icon::Tint).icon();
    return on ? onIcon : offIcon;
}

static QRect favoriteRect(const QRect &itemRect)
{
    return QRect(itemRect.right() - PaddingHS - IconSize,
                 itemRect.top() + (itemRect.height() - IconSize) / 2,
                 IconSize, IconSize);
}

class ConfigSelectDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QString text = index.data(Qt::DisplayRole).toString();

        if (index.data(HeaderRole).toBool()) {
            painter->setFont(headerTf.font());
            painter->setPen(headerTf.color());
            painter->drawText(option.rect.adjusted(PaddingHS, 0, -PaddingHS, 0),
                              headerTf.drawTextFlags, text);
            return;
        }

        const bool selected = option.state & QStyle::State_Selected;
        const bool hovered = option.state & QStyle::State_MouseOver;
        if (selected || hovered) {
            StyleHelper::drawCardBg(painter,
                                    option.rect.adjusted(PaddingHXxs, 0, -PaddingHXxs, 0),
                                    Utils::creatorColor(selected ? Theme::Token_Foreground_Muted
                                                                 : Theme::Token_Foreground_Subtle),
                                    QPen(Qt::NoPen),
                                    RadiusS);
        }

        const QRect favRect = favoriteRect(option.rect);
        const bool isFavorite = index.data(FavoriteRole).toBool();
        if (isFavorite || selected || hovered)
            favoriteIcon(isFavorite).paint(painter, favRect);

        const TextFormat &tf = index.data(CurrentRole).toBool() ? currentNameTf : nameTf;
        const int textLeft = option.rect.left() + PaddingHS;
        const int textRight = favRect.left() - GapHS;
        const QFontMetrics nameFm(tf.font());
        // Elide against the room there is, and take the width from the result.
        // Measuring the name and eliding it to its own width again elides text
        // that fits wherever the advance is fractional, as it is on macOS.
        const QString name = nameFm.elidedText(text, Qt::ElideRight, textRight - textLeft);
        const int nameWidth = qMin(nameFm.horizontalAdvance(name), textRight - textLeft);
        const QRect nameRect(textLeft, option.rect.top(), nameWidth, option.rect.height());
        painter->setFont(tf.font());
        painter->setPen(tf.color());
        painter->drawText(nameRect, tf.drawTextFlags, name);

        const QString description = index.data(DescriptionRole).toString();
        const int descriptionLeft = nameRect.right() + GapHS;
        if (description.isEmpty() || text == description || descriptionLeft >= textRight)
            return;

        const QRect descriptionRect(descriptionLeft, option.rect.top(),
                                    textRight - descriptionLeft, option.rect.height());
        const QFontMetrics descriptionFm(descriptionTf.font());
        painter->setFont(descriptionTf.font());
        painter->setPen(descriptionTf.color());
        painter->drawText(descriptionRect, descriptionTf.drawTextFlags,
                          descriptionFm.elidedText(description, Qt::ElideRight,
                                                   descriptionRect.width()));
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const bool isHeader = index.data(HeaderRole).toBool();
        const int lineHeight = isHeader ? headerTf.lineHeight() : nameTf.lineHeight();
        const int height = qMax(lineHeight, isHeader ? 0 : int(IconSize)) + 2 * PaddingVXs;
        return QSize(option.rect.width(), height);
    }
};

// List view reporting clicks on an item's favorite icon separately from clicks
// selecting the item.
class ConfigSelectListView final : public QListView
{
public:
    explicit ConfigSelectListView(QWidget *parent = nullptr)
        : QListView(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setMouseTracking(true);
        // Keystrokes belong to the filter, also after a click in the list.
        setFocusPolicy(Qt::NoFocus);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setItemDelegate(new ConfigSelectDelegate(this));
    }

    std::function<void(const QModelIndex &)> onFavoriteClicked;

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        const QPoint pos = event->position().toPoint();
        const QModelIndex index = indexAt(pos);
        if (event->button() == Qt::LeftButton && index.isValid()
            && !index.data(HeaderRole).toBool() && favoriteRect(visualRect(index)).contains(pos)) {
            if (onFavoriteClicked)
                onFavoriteClicked(index);
            return;
        }
        QListView::mousePressEvent(event);
    }
};

ConfigSelectPopup::ConfigSelectPopup(QWidget *parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    // The card painted in paintEvent has rounded corners; a plain popup window
    // would fill the corners with the palette and shadow its rectangle.
    setAttribute(Qt::WA_TranslucentBackground);

    m_filterEdit = new QtcSearchBox;
    m_filterEdit->setFiltering(true);
    m_filterEdit->setPlaceholderText(Tr::tr("Filter"));
    m_filterEdit->installEventFilter(this);

    m_model = new QStandardItemModel(this);

    m_view = new ConfigSelectListView;
    m_view->setModel(m_model);
    m_view->onFavoriteClicked = [this](const QModelIndex &index) { toggleFavorite(index); };

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(PaddingHS, PaddingVS, PaddingHS, PaddingVS);
    layout->setSpacing(GapVXs);
    layout->addWidget(m_filterEdit);
    layout->addWidget(m_view);

    connect(m_filterEdit, &QLineEdit::textChanged, this, &ConfigSelectPopup::setFilter);
    connect(m_view, &QAbstractItemView::clicked, this, &ConfigSelectPopup::activate);
}

void ConfigSelectPopup::setEntries(const QList<ConfigSelectEntry> &entries,
                                   const QString &currentValue)
{
    m_entries = entries;
    m_currentValue = currentValue;
    rebuild();
}

void ConfigSelectPopup::setFavorites(const QStringList &favoriteValues)
{
    m_favorites = favoriteValues;
    rebuild();
}

void ConfigSelectPopup::setFilter(const QString &filter)
{
    if (m_filter == filter)
        return;
    m_filter = filter;
    if (m_filterEdit->text() != filter)
        m_filterEdit->setText(filter);
    rebuild();
}

QStringList ConfigSelectPopup::visibleValues() const
{
    QStringList result;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QModelIndex index = m_model->index(row, 0);
        if (!index.data(HeaderRole).toBool())
            result.append(index.data(ValueRole).toString());
    }
    return result;
}

void ConfigSelectPopup::rebuild()
{
    const QModelIndex previous = m_view->currentIndex();
    const QString selectedValue = previous.isValid() ? previous.data(ValueRole).toString()
                                                     : m_currentValue;

    m_model->clear();

    const QRegularExpression matcher = FuzzyMatcher::createRegExp(m_filter);
    const auto matches = [this, &matcher](const ConfigSelectEntry &entry) {
        if (m_filter.isEmpty())
            return true;
        return matcher.match(entry.name).hasMatch() || matcher.match(entry.value).hasMatch()
               || matcher.match(entry.description).hasMatch();
    };

    const auto addHeader = [this](const QString &text) {
        auto *item = new QStandardItem(text);
        item->setFlags({});
        item->setData(true, HeaderRole);
        m_model->appendRow(item);
    };
    const auto addEntry = [this](const ConfigSelectEntry &entry) {
        const QString name = entry.name.isEmpty() ? entry.value : entry.name;
        auto *item = new QStandardItem(name);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setData(entry.value, ValueRole);
        item->setData(entry.description, DescriptionRole);
        item->setData(m_favorites.contains(entry.value), FavoriteRole);
        item->setData(entry.value == m_currentValue, CurrentRole);
        if (!entry.description.isEmpty() && entry.description != name)
            item->setToolTip(entry.description);
        m_model->appendRow(item);
    };

    QList<ConfigSelectEntry> favorites;
    for (const QString &value : std::as_const(m_favorites)) {
        const auto it = std::find_if(m_entries.cbegin(), m_entries.cend(),
                                     [&value](const ConfigSelectEntry &entry) {
            return entry.value == value;
        });
        if (it != m_entries.cend() && matches(*it))
            favorites.append(*it);
    }

    if (!favorites.isEmpty()) {
        addHeader(Tr::tr("Favorites"));
        for (const ConfigSelectEntry &entry : std::as_const(favorites))
            addEntry(entry);
    }

    QString group;
    bool anyEntry = false;
    for (const ConfigSelectEntry &entry : std::as_const(m_entries)) {
        if (!matches(entry))
            continue;
        if (!anyEntry || entry.group != group) {
            group = entry.group;
            if (!group.isEmpty())
                addHeader(group);
            else if (!favorites.isEmpty() && !anyEntry)
                addHeader(Tr::tr("All"));
        }
        addEntry(entry);
        anyEntry = true;
    }

    if (!anyEntry) {
        auto *item = new QStandardItem(Tr::tr("No matching entries"));
        item->setFlags({});
        item->setData(true, HeaderRole);
        m_model->appendRow(item);
    }

    // Keep the value selected that was selected before, so that typing a filter
    // does not move the keyboard selection away from it.
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QModelIndex index = m_model->index(row, 0);
        if ((index.flags() & Qt::ItemIsSelectable)
            && index.data(ValueRole).toString() == selectedValue) {
            m_view->setCurrentIndex(index);
            break;
        }
    }
    if (!m_view->currentIndex().isValid())
        moveSelection(1);

    updateListHeight();
}

void ConfigSelectPopup::updateListHeight()
{
    int listHeight = 2 * PaddingVXs;
    for (int row = 0; row < m_model->rowCount(); ++row)
        listHeight += m_view->sizeHintForRow(row);
    const int viewHeight = qBound(int(MinListHeight), listHeight, int(MaxListHeight));
    if (m_view->height() == viewHeight)
        return;

    m_view->setFixedHeight(viewHeight);
    if (!isVisible())
        return;

    // The window keeps the size constraints of the layout as it was when the
    // popup was shown until the layout is activated again, and a stale minimum
    // height would clamp the resize below.
    layout()->activate();

    // Filtering shrinks the list while the popup is open. Keep the edge the
    // popup was opened from in place, so it stays attached to its button.
    const QRect previous = geometry();
    const int height = preferredHeight();
    const int top = m_openedUpwards ? previous.bottom() - height + 1 : previous.top();
    setGeometry(previous.left(), top, previous.width(), height);
}

// The layout's own size hint lags behind the list height set above until the
// layout is activated, which only happens once the event loop runs again.
int ConfigSelectPopup::preferredHeight() const
{
    const QMargins margins = layout()->contentsMargins();
    return margins.top() + m_filterEdit->sizeHint().height() + layout()->spacing()
           + m_view->height() + margins.bottom();
}

void ConfigSelectPopup::moveSelection(int delta)
{
    const int rowCount = m_model->rowCount();
    if (rowCount == 0 || delta == 0)
        return;

    const int step = delta > 0 ? 1 : -1;
    int row = m_view->currentIndex().isValid() ? m_view->currentIndex().row()
                                               : (delta > 0 ? -1 : rowCount);
    for (int remaining = qAbs(delta); remaining > 0; --remaining) {
        int candidate = row + step;
        while (candidate >= 0 && candidate < rowCount
               && !(m_model->index(candidate, 0).flags() & Qt::ItemIsSelectable)) {
            candidate += step;
        }
        if (candidate < 0 || candidate >= rowCount)
            break;
        row = candidate;
    }

    if (row < 0 || row >= rowCount)
        return;
    const QModelIndex index = m_model->index(row, 0);
    if (index.flags() & Qt::ItemIsSelectable) {
        m_view->setCurrentIndex(index);
        m_view->scrollTo(index);
    }
}

// An empty string is a valid value, so a header is told apart by its role.
static bool isEntry(const QModelIndex &index)
{
    return index.isValid() && !index.data(HeaderRole).toBool();
}

void ConfigSelectPopup::activate(const QModelIndex &index)
{
    if (!isEntry(index))
        return;
    const QString value = index.data(ValueRole).toString();
    close();
    emit valueSelected(value);
}

void ConfigSelectPopup::toggleFavorite(const QModelIndex &index)
{
    if (!isEntry(index))
        return;
    const QString value = index.data(ValueRole).toString();
    if (m_favorites.removeAll(value) == 0)
        m_favorites.append(value);
    rebuild();
    emit favoritesChanged(m_favorites);
}

bool ConfigSelectPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_filterEdit)
        return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::ShortcutOverride) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape && keyEvent->modifiers() == Qt::NoModifier) {
            event->accept();
            return true;
        }
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() != QEvent::KeyPress)
        return QWidget::eventFilter(watched, event);

    const int pageRows = qMax(1, m_view->height() / qMax(1, nameTf.lineHeight() + 2 * PaddingVXs));
    auto *keyEvent = static_cast<QKeyEvent *>(event);
    switch (keyEvent->key()) {
    case Qt::Key_Down:
        moveSelection(1);
        return true;
    case Qt::Key_Up:
        moveSelection(-1);
        return true;
    case Qt::Key_PageDown:
        moveSelection(pageRows);
        return true;
    case Qt::Key_PageUp:
        moveSelection(-pageRows);
        return true;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        activate(m_view->currentIndex());
        return true;
    case Qt::Key_Escape:
        close();
        return true;
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

void ConfigSelectPopup::showRelativeTo(QWidget *anchor)
{
    updateListHeight();
    const int width = qBound(int(MinPopupWidth), anchor ? anchor->width() : int(MinPopupWidth),
                             int(MaxPopupWidth));
    resize(width, preferredHeight());

    if (anchor) {
        const QPoint anchorTopLeft = anchor->mapToGlobal(QPoint(0, 0));
        const QScreen *screen = anchor->screen();
        const QRect available = screen ? screen->availableGeometry() : QRect();
        // The input area sits at the bottom of the chat panel, so the popup
        // opens upwards unless it does not fit there.
        int y = anchorTopLeft.y() - height() - GapVXs;
        m_openedUpwards = true;
        if (available.isValid() && y < available.top()) {
            y = anchorTopLeft.y() + anchor->height() + GapVXs;
            m_openedUpwards = false;
        }
        int x = anchorTopLeft.x();
        if (available.isValid()) {
            x = qBound(available.left(), x, qMax(available.left(), available.right() - width));
            y = qMin(y, available.bottom() - height());
        }
        move(x, y);
    }

    show();
    m_filterEdit->selectAll();
    m_filterEdit->setFocus();
}

void ConfigSelectPopup::mousePressEvent(QMouseEvent *event)
{
    // A Qt::Popup grabs the mouse, so a click anywhere outside ends up here.
    if (!rect().contains(event->position().toPoint())) {
        close();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ConfigSelectPopup::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    StyleHelper::drawCardBg(&painter,
                            rect().adjusted(0, 0, -1, -1),
                            Utils::creatorColor(Theme::Token_Background_Default),
                            Utils::creatorColor(Theme::Token_Stroke_Subtle),
                            RadiusM);
}

} // namespace AcpClient::Internal
