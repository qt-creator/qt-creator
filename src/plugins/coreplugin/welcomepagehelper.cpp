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

#include "welcomepagehelper.h"

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QHeaderView>
#include <QHoverEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmapCache>
#include <QTimer>

namespace Core {

using namespace Utils;

static QColor themeColor(Theme::Color role)
{
    return creatorTheme()->color(role);
}

static QFont sizedFont(int size, const QWidget *widget)
{
    QFont f = widget->font();
    f.setPixelSize(size);
    return f;
}

SearchBox::SearchBox(QWidget *parent)
    : WelcomePageFrame(parent)
{
    QPalette pal;
    pal.setColor(QPalette::Base, themeColor(Theme::Welcome_BackgroundColor));

    m_lineEdit = new FancyLineEdit;
    m_lineEdit->setFiltering(true);
    m_lineEdit->setFrame(false);
    m_lineEdit->setFont(sizedFont(14, this));
    m_lineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_lineEdit->setPalette(pal);

    auto box = new QHBoxLayout(this);
    box->setContentsMargins(10, 3, 3, 3);
    box->addWidget(m_lineEdit);
}

GridView::GridView(QWidget *parent)
    : QTableView(parent)
{
    setVerticalScrollMode(ScrollPerPixel);
    horizontalHeader()->hide();
    horizontalHeader()->setDefaultSectionSize(GridProxyModel::GridItemWidth);
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize(GridProxyModel::GridItemHeight);
    setMouseTracking(true); // To enable hover.
    setSelectionMode(QAbstractItemView::NoSelection);
    setFrameShape(QFrame::NoFrame);
    setGridStyle(Qt::NoPen);

    QPalette pal;
    pal.setColor(QPalette::Base, themeColor(Theme::Welcome_BackgroundColor));
    setPalette(pal); // Makes a difference on Mac.
}

void GridView::leaveEvent(QEvent *)
{
    QHoverEvent hev(QEvent::HoverLeave, QPointF(), QPointF());
    viewportEvent(&hev); // Seemingly needed to kill the hover paint.
}

void GridProxyModel::setSourceModel(QAbstractItemModel *newModel)
{
    if (m_sourceModel == newModel)
        return;
    if (m_sourceModel)
        disconnect(m_sourceModel, nullptr, this, nullptr);
    m_sourceModel = newModel;
    if (newModel) {
        connect(newModel, &QAbstractItemModel::layoutAboutToBeChanged, this, [this] {
            emit layoutAboutToBeChanged();
        });
        connect(newModel, &QAbstractItemModel::layoutChanged, this, [this] {
            emit layoutChanged();
        });
        connect(newModel, &QAbstractItemModel::modelAboutToBeReset, this, [this] {
            beginResetModel();
        });
        connect(newModel, &QAbstractItemModel::modelReset, this, [this] { endResetModel(); });
        connect(newModel, &QAbstractItemModel::rowsAboutToBeInserted, this, [this] {
            beginResetModel();
        });
        connect(newModel, &QAbstractItemModel::rowsInserted, this, [this] { endResetModel(); });
        connect(newModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, [this] {
            beginResetModel();
        });
        connect(newModel, &QAbstractItemModel::rowsRemoved, this, [this] { endResetModel(); });
        connect(newModel,
                &QAbstractItemModel::dataChanged,
                this,
                [this] (const QModelIndex &topLeft,
                        const QModelIndex &bottomRight,
                        const QVector<int> &roles) {
            QAbstractItemModel::dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles);
        });
    }
}

QAbstractItemModel *GridProxyModel::sourceModel() const
{
    return m_sourceModel;
}

QVariant GridProxyModel::data(const QModelIndex &index, int role) const
{
    const OptModelIndex sourceIndex = mapToSource(index);
    if (sourceIndex)
        return sourceModel()->data(*sourceIndex, role);
    return QVariant();
}

Qt::ItemFlags GridProxyModel::flags(const QModelIndex &index) const
{
    const OptModelIndex sourceIndex = mapToSource(index);
    if (sourceIndex)
        return sourceModel()->flags(*sourceIndex);
    return Qt::ItemFlags();
}

bool GridProxyModel::hasChildren(const QModelIndex &parent) const
{
    const OptModelIndex sourceParent = mapToSource(parent);
    if (sourceParent)
        return sourceModel()->hasChildren(*sourceParent);
    return false;
}

void GridProxyModel::setColumnCount(int columnCount)
{
    if (columnCount == m_columnCount)
        return;
    QTC_ASSERT(columnCount >= 1, columnCount = 1);
    m_columnCount = columnCount;
    emit layoutChanged();
}

int GridProxyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    int rows = sourceModel()->rowCount(QModelIndex());
    return (rows + m_columnCount - 1) / m_columnCount;
}

int GridProxyModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_columnCount;
}

QModelIndex GridProxyModel::index(int row, int column, const QModelIndex &) const
{
    return createIndex(row, column, nullptr);
}

QModelIndex GridProxyModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

// The items at the lower right of the grid might not correspond to source items, if
// source's row count is not N*columnCount
OptModelIndex GridProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
        return QModelIndex();
    int sourceRow = proxyIndex.row() * m_columnCount + proxyIndex.column();
    if (sourceRow < sourceModel()->rowCount())
        return sourceModel()->index(sourceRow, 0);
    return OptModelIndex();
}

QModelIndex GridProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid())
        return QModelIndex();
    QTC_CHECK(sourceIndex.column() == 0);
    int proxyRow = sourceIndex.row() / m_columnCount;
    int proxyColumn = sourceIndex.row() % m_columnCount;
    return index(proxyRow, proxyColumn, QModelIndex());
}

const QSize ListModel::defaultImageSize(188, 145);

ListModel::ListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ListModel::~ListModel()
{
    qDeleteAll(m_items);
    m_items.clear();
}

int ListModel::rowCount(const QModelIndex &) const
{
    return m_items.size();
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.count())
        return QVariant();

    ListItem *item = m_items.at(index.row());
    switch (role) {
    case Qt::DisplayRole: // for search only
        return QString(item->name + ' ' + item->tags.join(' '));
    case ItemRole:
        return QVariant::fromValue(item);
    case ItemImageRole: {
        QPixmap pixmap;
        if (QPixmapCache::find(item->imageUrl, &pixmap))
            return pixmap;
        if (pixmap.isNull())
            pixmap = fetchPixmapAndUpdatePixmapCache(item->imageUrl);
        return pixmap;
    }
    case ItemTagsRole:
        return item->tags;
    default:
        return QVariant();
    }
}

ListModelFilter::ListModelFilter(ListModel *sourceModel, QObject *parent) :
    QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    sort(0);
}

bool ListModelFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const ListItem *item = sourceModel()->index(sourceRow, 0, sourceParent).data(
                ListModel::ItemRole).value<Core::ListItem *>();

    if (!item)
        return false;

    bool earlyExitResult;
    if (leaveFilterAcceptsRowBeforeFiltering(item, &earlyExitResult))
        return earlyExitResult;

    if (!m_filterTags.isEmpty()) {
        return Utils::allOf(m_filterTags, [&item](const QString &filterTag) {
            return item->tags.contains(filterTag);
        });
    }

    if (!m_filterStrings.isEmpty()) {
        for (const QString &subString : m_filterStrings) {
            bool wordMatch = false;
            wordMatch |= bool(item->name.contains(subString, Qt::CaseInsensitive));
            if (wordMatch)
                continue;
            const auto subMatch = [&subString](const QString &elem) { return elem.contains(subString); };
            wordMatch |= Utils::contains(item->tags, subMatch);
            if (wordMatch)
                continue;
            wordMatch |= bool(item->description.contains(subString, Qt::CaseInsensitive));
            if (!wordMatch)
                return false;
        }
    }

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void ListModelFilter::delayedUpdateFilter()
{
    if (m_timerId != 0)
        killTimer(m_timerId);

    m_timerId = startTimer(320);
}

void ListModelFilter::timerEvent(QTimerEvent *timerEvent)
{
    if (m_timerId == timerEvent->timerId()) {
        invalidateFilter();
        emit layoutChanged();
        killTimer(m_timerId);
        m_timerId = 0;
    }
}

struct SearchStringLexer
{
    QString code;
    const QChar *codePtr;
    QChar yychar;
    QString yytext;

    enum TokenKind {
        END_OF_STRING = 0,
        TAG,
        STRING_LITERAL,
        UNKNOWN
    };

    inline void yyinp() { yychar = *codePtr++; }

    SearchStringLexer(const QString &code)
        : code(code)
        , codePtr(code.unicode())
        , yychar(QLatin1Char(' ')) { }

    int operator()() { return yylex(); }

    int yylex() {
        while (yychar.isSpace())
            yyinp(); // skip all the spaces

        yytext.clear();

        if (yychar.isNull())
            return END_OF_STRING;

        QChar ch = yychar;
        yyinp();

        switch (ch.unicode()) {
        case '"':
        case '\'':
        {
            const QChar quote = ch;
            yytext.clear();
            while (!yychar.isNull()) {
                if (yychar == quote) {
                    yyinp();
                    break;
                }
                if (yychar == QLatin1Char('\\')) {
                    yyinp();
                    switch (yychar.unicode()) {
                    case '"': yytext += QLatin1Char('"'); yyinp(); break;
                    case '\'': yytext += QLatin1Char('\''); yyinp(); break;
                    case '\\': yytext += QLatin1Char('\\'); yyinp(); break;
                    }
                } else {
                    yytext += yychar;
                    yyinp();
                }
            }
            return STRING_LITERAL;
        }

        default:
            if (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
                yytext.clear();
                yytext += ch;
                while (yychar.isLetterOrNumber() || yychar == QLatin1Char('_')) {
                    yytext += yychar;
                    yyinp();
                }
                if (yychar == QLatin1Char(':') && yytext == QLatin1String("tag")) {
                    yyinp();
                    return TAG;
                }
                return STRING_LITERAL;
            }
        }

        yytext += ch;
        return UNKNOWN;
    }
};

void ListModelFilter::setSearchString(const QString &arg)
{
    if (m_searchString == arg)
        return;

    m_searchString = arg;
    m_filterTags.clear();
    m_filterStrings.clear();

    // parse and update
    SearchStringLexer lex(arg);
    bool isTag = false;
    while (int tk = lex()) {
        if (tk == SearchStringLexer::TAG) {
            isTag = true;
            m_filterStrings.append(lex.yytext);
        }

        if (tk == SearchStringLexer::STRING_LITERAL) {
            if (isTag) {
                m_filterStrings.pop_back();
                m_filterTags.append(lex.yytext);
                isTag = false;
            } else {
                m_filterStrings.append(lex.yytext);
            }
        }
    }

    delayedUpdateFilter();
}

bool ListModelFilter::leaveFilterAcceptsRowBeforeFiltering(const ListItem *, bool *) const
{
    return false;
}

ListItemDelegate::ListItemDelegate()
{
    lightColor = QColor(221, 220, 220); // color: "#dddcdc"
    backgroundColor = themeColor(Theme::Welcome_BackgroundColor);
    foregroundColor1 = themeColor(Theme::Welcome_ForegroundPrimaryColor); // light-ish.
    foregroundColor2 = themeColor(Theme::Welcome_ForegroundSecondaryColor); // blacker.
}

void ListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    const ListItem *item = index.data(ListModel::ItemRole).value<Core::ListItem *>();

    // Quick hack for empty items in the last row.
    if (!item)
        return;

    const QRect rc = option.rect;

    const int d = 10;
    const int x = rc.x() + d;
    const int y = rc.y() + d;
    const int w = rc.width() - 2 * d - GridProxyModel::GridItemGap;
    const int h = rc.height() - 2 * d;
    const bool hovered = option.state & QStyle::State_MouseOver;

    const int tagsBase = GridProxyModel::TagsSeparatorY + 10;
    const int shiftY = GridProxyModel::TagsSeparatorY - 20;
    const int nameY = GridProxyModel::TagsSeparatorY - 20;

    const QRect textRect = QRect(x, y + nameY, w, h);

    QTextOption wrapped;
    wrapped.setWrapMode(QTextOption::WordWrap);
    int offset = 0;
    if (hovered) {
        if (index != m_previousIndex) {
            m_previousIndex = index;
            m_startTime.start();
            m_currentArea = rc;
            m_currentWidget = qobject_cast<QAbstractItemView *>(
                const_cast<QWidget *>(option.widget));
        }
        offset = m_startTime.elapsed() * GridProxyModel::GridItemHeight / 200; // Duration 200 ms.
        if (offset < shiftY)
            QTimer::singleShot(5, this, &ListItemDelegate::goon);
        else if (offset > shiftY)
            offset = shiftY;
    } else {
        m_previousIndex = QModelIndex();
    }

    const QFontMetrics fm(option.widget->font());
    const QRect shiftedTextRect = textRect.adjusted(0, -offset, 0, -offset);

    // The pixmap.
    if (offset == 0) {
        QPixmap pm = index.data(ListModel::ItemImageRole).value<QPixmap>();
        QRect inner(x + 11, y - offset, ListModel::defaultImageSize.width(),
                    ListModel::defaultImageSize.height());
        QRect pixmapRect = inner;
        if (!pm.isNull()) {
            painter->setPen(foregroundColor2);

            adjustPixmapRect(&pixmapRect);

            QPoint pixmapPos = pixmapRect.center();
            pixmapPos.rx() -= pm.width() / pm.devicePixelRatio() / 2;
            pixmapPos.ry() -= pm.height() / pm.devicePixelRatio() / 2;
            painter->drawPixmap(pixmapPos, pm);

            drawPixmapOverlay(item, painter, option, pixmapRect);

        } else {
            // The description text as fallback.
            painter->setPen(foregroundColor2);
            painter->setFont(sizedFont(11, option.widget));
            painter->drawText(pixmapRect.adjusted(6, 10, -6, -10), item->description, wrapped);
        }
        painter->setPen(foregroundColor1);
        painter->drawRect(pixmapRect.adjusted(-1, -1, -1, -1));
    }

    // The title of the example.
    painter->setPen(foregroundColor1);
    painter->setFont(sizedFont(13, option.widget));
    QRectF nameRect;
    if (offset) {
        nameRect = painter->boundingRect(shiftedTextRect, item->name, wrapped);
        painter->drawText(nameRect, item->name, wrapped);
    } else {
        nameRect = QRect(x, y + nameY, x + w, y + nameY + 20);
        QString elidedName = fm.elidedText(item->name, Qt::ElideRight, w - 20);
        painter->drawText(nameRect, elidedName);
    }

    // The separator line below the example title.
    if (offset) {
        int ll = nameRect.bottom() + 5;
        painter->setPen(lightColor);
        painter->drawLine(x, ll, x + w, ll);
    }

    // The description text.
    if (offset) {
        int dd = nameRect.height() + 10;
        QRect descRect = shiftedTextRect.adjusted(0, dd, 0, dd);
        painter->setPen(foregroundColor2);
        painter->setFont(sizedFont(11, option.widget));
        painter->drawText(descRect, item->description, wrapped);
    }

    // Separator line between text and 'Tags:' section
    painter->setPen(lightColor);
    painter->drawLine(x, y + GridProxyModel::TagsSeparatorY,
                      x + w, y + GridProxyModel::TagsSeparatorY);

    // The 'Tags:' section
    const int tagsHeight = h - tagsBase;
    const QFont tagsFont = sizedFont(10, option.widget);
    const QFontMetrics tagsFontMetrics(tagsFont);
    QRect tagsLabelRect = QRect(x, y + tagsBase, 30, tagsHeight - 2);
    painter->setPen(foregroundColor2);
    painter->setFont(tagsFont);
    painter->drawText(tagsLabelRect, tr("Tags:"));

    painter->setPen(themeColor(Theme::Welcome_LinkColor));
    m_currentTagRects.clear();
    int xx = 0;
    int yy = y + tagsBase;
    for (const QString &tag : item->tags) {
        const int ww = tagsFontMetrics.horizontalAdvance(tag) + 5;
        if (xx + ww > w - 30) {
            yy += 15;
            xx = 0;
        }
        const QRect tagRect(xx + x + 30, yy, ww, 15);
        painter->drawText(tagRect, tag);
        m_currentTagRects.append({ tag, tagRect });
        xx += ww;
    }

    // Box it when hovered.
    if (hovered) {
        painter->setPen(lightColor);
        painter->drawRect(rc.adjusted(0, 0, -1, -1));
    }

}

bool ListItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                   const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        const ListItem *item = index.data(ListModel::ItemRole).value<ListItem *>();
        if (!item)
            return false;
        auto mev = static_cast<QMouseEvent *>(event);

        if (mev->button() != Qt::LeftButton) // do not react on right click
            return false;

        if (index.isValid()) {
            const QPoint pos = mev->pos();
            if (pos.y() > option.rect.y() + GridProxyModel::TagsSeparatorY) {
                //const QStringList tags = idx.data(Tags).toStringList();
                for (const auto &it : m_currentTagRects) {
                    if (it.second.contains(pos))
                        emit tagClicked(it.first);
                }
            } else {
                clickAction(item);
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void ListItemDelegate::drawPixmapOverlay(const ListItem *, QPainter *,
                                         const QStyleOptionViewItem &, const QRect &) const
{
}

void ListItemDelegate::clickAction(const ListItem *) const
{
}

void ListItemDelegate::adjustPixmapRect(QRect *) const
{
}

void ListItemDelegate::goon()
{
    if (m_currentWidget)
        m_currentWidget->viewport()->update(m_currentArea);
}

} // namespace Core
