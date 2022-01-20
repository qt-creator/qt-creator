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
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QEasingCurve>
#include <QFontDatabase>
#include <QHeaderView>
#include <QHoverEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmapCache>
#include <QTimer>

#include <qdrawutil.h>

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

namespace WelcomePageHelpers {

QFont brandFont()
{
    const static QFont f = []{
        const int id = QFontDatabase::addApplicationFont(":/studiofonts/TitilliumWeb-Regular.ttf");
        QFont result;
        result.setPixelSize(16);
        if (id >= 0) {
            const QStringList fontFamilies = QFontDatabase::applicationFontFamilies(id);
            result.setFamilies(fontFamilies);
        }
        return result;
    }();
    return f;
}

QWidget *panelBar(QWidget *parent)
{
    auto frame = new QWidget(parent);
    frame->setAutoFillBackground(true);
    frame->setMinimumWidth(WelcomePageHelpers::HSpacing);
    QPalette pal = frame->palette();
    pal.setColor(QPalette::Window, themeColor(Theme::Welcome_BackgroundPrimaryColor));
    frame->setPalette(pal);
    return frame;
}

} // namespace WelcomePageHelpers

SearchBox::SearchBox(QWidget *parent)
    : WelcomePageFrame(parent)
{
    setAutoFillBackground(true);

    m_lineEdit = new FancyLineEdit;
    m_lineEdit->setFiltering(true);
    m_lineEdit->setFrame(false);
    m_lineEdit->setFont(WelcomePageHelpers::brandFont());
    m_lineEdit->setMinimumHeight(33);
    m_lineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    QPalette pal = buttonPalette(false, false, true);
    // for the margins
    pal.setColor(QPalette::Window, m_lineEdit->palette().color(QPalette::Base));
    // for macOS dark mode
    pal.setColor(QPalette::WindowText, themeColor(Theme::Welcome_ForegroundPrimaryColor));
    pal.setColor(QPalette::Text, themeColor(Theme::Welcome_TextColor));
    setPalette(pal);

    auto box = new QHBoxLayout(this);
    box->setContentsMargins(10, 0, 1, 0);
    box->addWidget(m_lineEdit);
}

GridView::GridView(QWidget *parent)
    : QListView(parent)
{
    setResizeMode(QListView::Adjust);
    setMouseTracking(true); // To enable hover.
    setSelectionMode(QAbstractItemView::NoSelection);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewMode(IconMode);
    setUniformItemSizes(true);

    QPalette pal;
    pal.setColor(QPalette::Base, themeColor(Theme::Welcome_BackgroundSecondaryColor));
    setPalette(pal); // Makes a difference on Mac.
}

void GridView::leaveEvent(QEvent *)
{
    QHoverEvent hev(QEvent::HoverLeave, QPointF(), QPointF());
    viewportEvent(&hev); // Seemingly needed to kill the hover paint.
}

const QSize ListModel::defaultImageSize(214, 160);

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

    explicit SearchStringLexer(const QString &code)
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
    : backgroundPrimaryColor(themeColor(Theme::Welcome_BackgroundPrimaryColor))
    , backgroundSecondaryColor(themeColor(Theme::Welcome_BackgroundSecondaryColor))
    , foregroundPrimaryColor(themeColor(Theme::Welcome_ForegroundPrimaryColor))
    , hoverColor(themeColor(Theme::Welcome_HoverColor))
    , textColor(themeColor(Theme::Welcome_TextColor))
{
}

void ListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    const ListItem *item = index.data(ListModel::ItemRole).value<Core::ListItem *>();

    const QRect rc = option.rect;
    const QRect tileRect(0, 0, rc.width() - GridItemGap, rc.height() - GridItemGap);
    const QSize thumbnailBgSize = ListModel::defaultImageSize.grownBy(QMargins(1, 1, 1, 1));
    const QRect thumbnailBgRect((tileRect.width() - thumbnailBgSize.width()) / 2, GridItemGap,
                                thumbnailBgSize.width(), thumbnailBgSize.height());
    const QRect textArea = tileRect.adjusted(GridItemGap, GridItemGap, -GridItemGap, -GridItemGap);

    const bool hovered = option.state & QStyle::State_MouseOver;

    constexpr int tagsBase = TagsSeparatorY + 17;
    constexpr int shiftY = TagsSeparatorY - 16;
    constexpr int nameY = TagsSeparatorY - 20;

    const QRect textRect = textArea.translated(0, nameY);

    painter->save();
    painter->translate(rc.topLeft());

    painter->fillRect(tileRect, hovered ? hoverColor : backgroundPrimaryColor);

    QTextOption wrapped;
    wrapped.setWrapMode(QTextOption::WordWrap);
    int offset = 0;
    float animationProgress = 0; // Linear increase from 0.0 to 1.0 during hover animation
    if (hovered) {
        if (index != m_previousIndex) {
            m_previousIndex = index;
            m_blurredThumbnail = QPixmap();
            m_startTime.start();
            m_currentWidget = qobject_cast<QAbstractItemView *>(
                const_cast<QWidget *>(option.widget));
        }
        constexpr float hoverAnimationDuration = 260;
        animationProgress = m_startTime.elapsed() / hoverAnimationDuration;
        static const QEasingCurve animationCurve(QEasingCurve::OutCubic);
        offset = animationCurve.valueForProgress(animationProgress) * shiftY;
        if (offset < shiftY)
            QTimer::singleShot(10, this, &ListItemDelegate::goon);
    } else if (index == m_previousIndex) {
        m_previousIndex = QModelIndex();
    }

    const QRect shiftedTextRect = textRect.adjusted(0, -offset, 0, -offset);

    // The pixmap.
    const QPixmap pm = index.data(ListModel::ItemImageRole).value<QPixmap>();
    QPoint thumbnailPos = thumbnailBgRect.center();
    if (!pm.isNull()) {
        painter->fillRect(thumbnailBgRect, backgroundSecondaryColor);

        thumbnailPos.rx() -= pm.width() / pm.devicePixelRatio() / 2 - 1;
        thumbnailPos.ry() -= pm.height() / pm.devicePixelRatio() / 2 - 1;
        painter->drawPixmap(thumbnailPos, pm);

        painter->setPen(foregroundPrimaryColor);
        drawPixmapOverlay(item, painter, option, thumbnailBgRect);
    } else {
        // The description text as fallback.
        painter->setPen(textColor);
        painter->setFont(sizedFont(11, option.widget));
        painter->drawText(thumbnailBgRect.adjusted(6, 10, -6, -10), item->description, wrapped);
    }

    // The description background
    if (offset && !pm.isNull()) {
        if (m_blurredThumbnail.isNull()) {
            constexpr int blurRadius = 50;
            QImage thumbnail(tileRect.size() + QSize(blurRadius, blurRadius) * 2,
                             QImage::Format_ARGB32_Premultiplied);
            thumbnail.fill(hoverColor);
            QPainter thumbnailPainter(&thumbnail);
            thumbnailPainter.translate(blurRadius, blurRadius);
            thumbnailPainter.fillRect(thumbnailBgRect, backgroundSecondaryColor);
            thumbnailPainter.drawPixmap(thumbnailPos, pm);
            thumbnailPainter.setPen(foregroundPrimaryColor);
            drawPixmapOverlay(item, &thumbnailPainter, option, thumbnailBgRect);
            thumbnailPainter.end();

            m_blurredThumbnail = QPixmap(tileRect.size());
            QPainter blurredThumbnailPainter(&m_blurredThumbnail);
            blurredThumbnailPainter.translate(-blurRadius, -blurRadius);
            qt_blurImage(&blurredThumbnailPainter, thumbnail, blurRadius, false, false);
            blurredThumbnailPainter.setOpacity(0.825);
            blurredThumbnailPainter.fillRect(tileRect, hoverColor);
        }
        QRect thumbnailPortionRect = tileRect;
        thumbnailPortionRect.setTop(shiftY - offset);
        const QPixmap thumbnailPortionPM = m_blurredThumbnail.copy(thumbnailPortionRect);
        painter->drawPixmap(thumbnailPortionRect.topLeft(), thumbnailPortionPM);
    }

    // The description Text (unhovered or hovered)
    painter->setPen(textColor);
    painter->setFont(sizedFont(13, option.widget)); // Title font
    if (offset) {
        // The title of the example
        const QRectF nameRect = painter->boundingRect(shiftedTextRect, item->name, wrapped);
        painter->drawText(nameRect, item->name, wrapped);

        // The separator line below the example title.
        const int ll = nameRect.height() + 3;
        const QLine line = QLine(0, ll, textArea.width(), ll).translated(shiftedTextRect.topLeft());
        painter->setPen(foregroundPrimaryColor);
        painter->setOpacity(animationProgress); // "fade in" separator line and description
        painter->drawLine(line);

        // The description text.
        const int dd = ll + 5;
        const QRect descRect = shiftedTextRect.adjusted(0, dd, 0, dd);
        painter->setPen(textColor);
        painter->setFont(sizedFont(11, option.widget));
        painter->drawText(descRect, item->description, wrapped);
        painter->setOpacity(1);
    } else {
        // The title of the example
        const QString elidedName = painter->fontMetrics()
                .elidedText(item->name, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, elidedName);
    }

    // Separator line between text and 'Tags:' section
    painter->setPen(foregroundPrimaryColor);
    painter->drawLine(QLineF(textArea.topLeft(), textArea.topRight())
                      .translated(0, TagsSeparatorY));

    // The 'Tags:' section
    painter->setPen(foregroundPrimaryColor);
    const QFont tagsFont = sizedFont(10, option.widget);
    painter->setFont(tagsFont);
    const QFontMetrics fm = painter->fontMetrics();
    const QString tagsLabelText = tr("Tags:");
    constexpr int tagsHorSpacing = 5;
    const QRect tagsLabelRect =
            QRect(0, 0, fm.horizontalAdvance(tagsLabelText) + tagsHorSpacing, fm.height())
            .translated(textArea.x(), tagsBase);
    painter->drawText(tagsLabelRect, tagsLabelText);

    painter->setPen(themeColor(Theme::Welcome_LinkColor));
    m_currentTagRects.clear();
    int xx = 0;
    int yy = 0;
    for (const QString &tag : item->tags) {
        const int ww = fm.horizontalAdvance(tag) + tagsHorSpacing;
        if (xx + ww > textArea.width() - tagsLabelRect.width()) {
            yy += fm.lineSpacing();
            xx = 0;
        }
        const QRect tagRect = QRect(xx, yy, ww, tagsLabelRect.height())
                .translated(tagsLabelRect.topRight());
        painter->drawText(tagRect, tag);
        m_currentTagRects.append({ tag, tagRect.translated(rc.topLeft()) });
        xx += ww;
    }

    painter->restore();
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
            if (pos.y() > option.rect.y() + TagsSeparatorY) {
                //const QStringList tags = idx.data(Tags).toStringList();
                for (const auto &it : qAsConst(m_currentTagRects)) {
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

QSize ListItemDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return {GridItemWidth, GridItemHeight};
}

void ListItemDelegate::drawPixmapOverlay(const ListItem *, QPainter *,
                                         const QStyleOptionViewItem &, const QRect &) const
{
}

void ListItemDelegate::clickAction(const ListItem *) const
{
}

void ListItemDelegate::goon()
{
    if (m_currentWidget)
        m_currentWidget->update(m_previousIndex);
}

} // namespace Core
