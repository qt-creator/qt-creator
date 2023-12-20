// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "welcomepagehelper.h"

#include "coreplugintr.h"

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QEasingCurve>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHoverEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmapCache>
#include <QScrollArea>
#include <QTimer>

#include <qdrawutil.h>

using namespace Utils;

namespace Core {

using namespace WelcomePageHelpers;

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
    QPalette pal;
    pal.setBrush(QPalette::Window, {});
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

SectionGridView::SectionGridView(QWidget *parent)
    : GridView(parent)
{}

void SectionGridView::setMaxRows(std::optional<int> max)
{
    m_maxRows = max;
    updateGeometry();
}

std::optional<int> SectionGridView::maxRows() const
{
    return m_maxRows;
}

bool SectionGridView::hasHeightForWidth() const
{
    return true;
}

int SectionGridView::heightForWidth(int width) const
{
    const int columnCount = qMax(1, width / Core::WelcomePageHelpers::GridItemWidth);
    const int rowCount = (model()->rowCount() + columnCount - 1) / columnCount;
    const int maxRowCount = m_maxRows ? std::min(*m_maxRows, rowCount) : rowCount;
    return maxRowCount * Core::WelcomePageHelpers::GridItemHeight;
}

void SectionGridView::wheelEvent(QWheelEvent *e)
{
    if (m_maxRows) // circumvent scrolling of the list view
        QWidget::wheelEvent(e);
    else
        GridView::wheelEvent(e);
}

bool SectionGridView::event(QEvent *e)
{
    if (e->type() == QEvent::Resize) {
        const auto itemsFit = [this](const QSize &size) {
            const int maxColumns = std::max(size.width() / WelcomePageHelpers::GridItemWidth, 1);
            const int maxRows = std::max(size.height() / WelcomePageHelpers::GridItemHeight, 1);
            const int maxItems = maxColumns * maxRows;
            const int items = model()->rowCount();
            return maxItems >= items;
        };
        auto resizeEvent = static_cast<QResizeEvent *>(e);
        const bool itemsCurrentyFit = itemsFit(size());
        if (!resizeEvent->oldSize().isValid()
            || itemsFit(resizeEvent->oldSize()) != itemsCurrentyFit) {
            emit itemsFitChanged(itemsCurrentyFit);
        }
    }
    return GridView::event(e);
}

ListModel::ListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ListModel::~ListModel()
{
    clear();
}

void ListModel::appendItems(const QList<ListItem *> &items)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + items.size());
    m_items.append(items);
    endInsertRows();
}

const QList<ListItem *> ListModel::items() const
{
    return m_items;
}

void ListModel::clear()
{
    beginResetModel();
    if (m_ownsItems)
        qDeleteAll(m_items);
    m_items.clear();
    endResetModel();
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
        if (pixmap.isNull() && m_fetchPixmapAndUpdatePixmapCache)
            pixmap = m_fetchPixmapAndUpdatePixmapCache(item->imageUrl);
        return pixmap;
    }
    case ItemTagsRole:
        return item->tags;
    default:
        return QVariant();
    }
}

void ListModel::setPixmapFunction(const PixmapFunction &fetchPixmapAndUpdatePixmapCache)
{
    m_fetchPixmapAndUpdatePixmapCache = fetchPixmapAndUpdatePixmapCache;
}

void ListModel::setOwnsItems(bool owns)
{
    m_ownsItems = owns;
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
            return item->tags.contains(filterTag, Qt::CaseInsensitive);
        });
    }

    if (!m_filterStrings.isEmpty()) {
        for (const QString &subString : m_filterStrings) {
            bool wordMatch = false;
            wordMatch |= bool(item->name.contains(subString, Qt::CaseInsensitive));
            if (wordMatch)
                continue;
            const auto subMatch = [&subString](const QString &elem) {
                return elem.contains(subString, Qt::CaseInsensitive);
            };
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

    invalidateFilter();
    emit layoutChanged();
}

ListModel *ListModelFilter::sourceListModel() const
{
    return static_cast<ListModel *>(sourceModel());
}

bool ListModelFilter::leaveFilterAcceptsRowBeforeFiltering(const ListItem *, bool *) const
{
    return false;
}

ListItemDelegate::ListItemDelegate()
    : backgroundPrimaryColor(themeColor(Theme::Welcome_BackgroundPrimaryColor))
    , backgroundSecondaryColor(themeColor(Theme::Welcome_BackgroundSecondaryColor))
    , foregroundPrimaryColor(themeColor(Theme::Welcome_ForegroundPrimaryColor))
    , foregroundSecondaryColor(themeColor(Theme::Welcome_ForegroundSecondaryColor))
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
    const QSize thumbnailBgSize = GridItemImageSize.grownBy(QMargins(1, 1, 1, 1));
    const QRect thumbnailBgRect((tileRect.width() - thumbnailBgSize.width()) / 2, GridItemGap,
                                thumbnailBgSize.width(), thumbnailBgSize.height());
    const QRect textArea = tileRect.adjusted(GridItemGap, GridItemGap, -GridItemGap, -GridItemGap);

    const bool hovered = option.state & QStyle::State_MouseOver;

    constexpr int TagsSeparatorY = GridItemHeight - GridItemGap - 52;
    constexpr int tagsBase = TagsSeparatorY + 17;
    constexpr int shiftY = TagsSeparatorY - 16;
    constexpr int nameY = TagsSeparatorY - 20;

    const QRect textRect = textArea.translated(0, nameY);
    const QFont descriptionFont = sizedFont(11, option.widget);

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
            m_currentTagRects.clear();
            m_blurredThumbnail = QPixmap();
            m_startTime.start();
            m_currentWidget = qobject_cast<QAbstractItemView *>(
                const_cast<QWidget *>(option.widget));
        }
        constexpr float hoverAnimationDuration = 260;
        animationProgress = m_startTime.elapsed() / hoverAnimationDuration;
        if (animationProgress < 1) {
            static const QEasingCurve animationCurve(QEasingCurve::OutCubic);
            offset = animationCurve.valueForProgress(animationProgress) * shiftY;
            QTimer::singleShot(10, this, &ListItemDelegate::goon);
        } else {
            offset = shiftY;
        }
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
        painter->setFont(descriptionFont);
        painter->drawText(textArea, item->description, wrapped);
    }

    // The description background
    if (offset) {
        QRect backgroundPortionRect = tileRect;
        backgroundPortionRect.setTop(shiftY - offset);
        if (!pm.isNull()) {
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
            const QPixmap thumbnailPortionPM = m_blurredThumbnail.copy(backgroundPortionRect);
            painter->drawPixmap(backgroundPortionRect.topLeft(), thumbnailPortionPM);
        } else {
            painter->fillRect(backgroundPortionRect, hoverColor);
        }
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
        painter->setPen(foregroundSecondaryColor);
        painter->setOpacity(animationProgress); // "fade in" separator line and description
        painter->drawLine(line);

        // The description text.
        const int dd = ll + 5;
        const QRect descRect = shiftedTextRect.adjusted(0, dd, 0, dd);
        painter->setPen(textColor);
        painter->setFont(descriptionFont);
        painter->drawText(descRect, item->description, wrapped);
        painter->setOpacity(1);
    } else {
        // The title of the example
        const QString elidedName = painter->fontMetrics()
                .elidedText(item->name, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, elidedName);
    }

    // Separator line between text and 'Tags:' section
    painter->setPen(foregroundSecondaryColor);
    painter->drawLine(QLineF(textArea.topLeft(), textArea.topRight())
                      .translated(0, TagsSeparatorY));

    // The 'Tags:' section
    painter->setPen(foregroundPrimaryColor);
    const QFont tagsFont = sizedFont(10, option.widget);
    painter->setFont(tagsFont);
    const QFontMetrics fm = painter->fontMetrics();
    const QString tagsLabelText = Tr::tr("Tags:");
    constexpr int tagsHorSpacing = 5;
    const QRect tagsLabelRect =
            QRect(0, 0, fm.horizontalAdvance(tagsLabelText) + tagsHorSpacing, fm.height())
            .translated(textArea.x(), tagsBase);
    painter->drawText(tagsLabelRect, tagsLabelText);

    painter->setPen(themeColor(Theme::Welcome_LinkColor));
    int emptyTagRowsLeft = 2;
    int xx = 0;
    int yy = 0;
    const bool populateTagsRects = m_currentTagRects.empty();
    for (const QString &tag : item->tags) {
        const int ww = fm.horizontalAdvance(tag) + tagsHorSpacing;
        if (xx + ww > textArea.width() - tagsLabelRect.width()) {
            if (--emptyTagRowsLeft == 0)
                break;
            yy += fm.lineSpacing();
            xx = 0;
        }
        const QRect tagRect = QRect(xx, yy, ww, tagsLabelRect.height())
                .translated(tagsLabelRect.topRight());
        painter->drawText(tagRect, tag);
        if (populateTagsRects)
            m_currentTagRects.append({ tag, tagRect });
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
            const QPoint mousePos = mev->pos() - option.rect.topLeft();
            const auto tagUnderMouse =
                    Utils::findOrDefault(m_currentTagRects,
                                         [&mousePos](const QPair<QString, QRect> &tag) {
                return tag.second.contains(mousePos);
            });
            if (!tagUnderMouse.first.isEmpty())
                emit tagClicked(tagUnderMouse.first);
            else
                clickAction(item);
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

SectionedGridView::SectionedGridView(QWidget *parent)
    : QStackedWidget(parent)
{
    m_searchTimer.setInterval(320);
    m_searchTimer.setSingleShot(true);
    connect(&m_searchTimer, &QTimer::timeout, this, [this] {
        setSearchString(m_delayedSearchString);
        m_delayedSearchString.clear();
    });

    m_allItemsModel.reset(new ListModel);
    m_allItemsModel->setPixmapFunction(m_pixmapFunction);

    auto area = new QScrollArea(this);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    area->setFrameShape(QFrame::NoFrame);
    area->setWidgetResizable(true);

    auto sectionedView = new QWidget;
    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch(1);
    sectionedView->setLayout(layout);
    area->setWidget(sectionedView);

    addWidget(area);
}

SectionedGridView::~SectionedGridView()
{
    clear();
}

void SectionedGridView::setItemDelegate(QAbstractItemDelegate *delegate)
{
    m_itemDelegate = delegate;
    if (m_allItemsView)
        m_allItemsView->setItemDelegate(delegate);
    for (GridView *view : std::as_const(m_gridViews))
        view->setItemDelegate(delegate);
}

void SectionedGridView::setPixmapFunction(const Core::ListModel::PixmapFunction &pixmapFunction)
{
    m_pixmapFunction = pixmapFunction;
    m_allItemsModel->setPixmapFunction(pixmapFunction);
    for (ListModel *model : std::as_const(m_sectionModels))
        model->setPixmapFunction(pixmapFunction);
}

void SectionedGridView::setSearchStringDelayed(const QString &searchString)
{
    m_delayedSearchString = searchString;
    m_searchTimer.start();
}

void SectionedGridView::setSearchString(const QString &searchString)
{
    if (searchString.isEmpty()) {
        // back to previous view
        m_allItemsView.reset();
        if (m_zoomedInWidget)
            setCurrentWidget(m_zoomedInWidget);
        else
            setCurrentIndex(0);
        return;
    }
    if (!m_allItemsView) {
        // We don't have a grid set for searching yet.
        // Create all items view for filtering.
        m_allItemsView.reset(new GridView);
        m_allItemsView->setObjectName("AllItemsView"); // used by Squish
        m_allItemsView->setModel(new ListModelFilter(m_allItemsModel.get(), m_allItemsView.get()));
        if (m_itemDelegate)
            m_allItemsView->setItemDelegate(m_itemDelegate);
        addWidget(m_allItemsView.get());
    }
    setCurrentWidget(m_allItemsView.get());
    auto filterModel = static_cast<ListModelFilter *>(m_allItemsView.get()->model());
    filterModel->setSearchString(searchString);
}

static QWidget *createSeparator(QWidget *parent)
{
    QWidget *line = Layouting::createHr(parent);
    QSizePolicy linePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    linePolicy.setHorizontalStretch(2);
    line->setSizePolicy(linePolicy);
    QPalette pal = line->palette();
    pal.setColor(QPalette::Dark, Qt::transparent);
    pal.setColor(QPalette::Light, themeColor(Theme::Welcome_ForegroundSecondaryColor));
    line->setPalette(pal);
    return line;
}

static QLabel *createLinkLabel(const QString &text, QWidget *parent)
{
    const QString linkColor = themeColor(Theme::Welcome_LinkColor).name();
    auto link = new QLabel("<a href=\"link\" style=\"color: " + linkColor + ";\">"
                           + text + "</a>", parent);
    return link;
}

ListModel *SectionedGridView::addSection(const Section &section, const QList<ListItem *> &items)
{
    auto model = new ListModel(this);
    model->setPixmapFunction(m_pixmapFunction);
    // the sections only keep a weak reference to the items,
    // they are owned by the allProducts model, since multiple sections can contain duplicates
    // of the same item
    model->setOwnsItems(false);
    model->appendItems(items);

    auto gridView = new SectionGridView(this);
    gridView->setItemDelegate(m_itemDelegate);
    gridView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gridView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gridView->setModel(model);
    gridView->setMaxRows(section.maxRows);

    m_sectionModels.insert(section, model);
    const auto it = m_gridViews.insert(section, gridView);

    QLabel *seeAllLink = createLinkLabel(Tr::tr("Show All") + " &gt;", this);
    if (gridView->maxRows().has_value()) {
        seeAllLink->setVisible(true);
        connect(gridView, &SectionGridView::itemsFitChanged, seeAllLink, [seeAllLink](bool fits) {
            seeAllLink->setVisible(!fits);
        });
    } else {
        seeAllLink->setVisible(false);
    }
    connect(seeAllLink, &QLabel::linkActivated, this, [this, section] { zoomInSection(section); });
    using namespace Layouting;
    QWidget *sectionLabel = Row {
        section.name,
        createSeparator(this),
        seeAllLink,
        Space(HSpacing),
        noMargin
    }.emerge();
    m_sectionLabels.append(sectionLabel);
    sectionLabel->setContentsMargins(0, ItemGap, 0, 0);
    sectionLabel->setFont(Core::WelcomePageHelpers::brandFont());
    auto scrollArea = qobject_cast<QScrollArea *>(widget(0));
    auto vbox = qobject_cast<QVBoxLayout *>(scrollArea->widget()->layout());

    // insert new section depending on its priority, but before the last (stretch) item
    int position = std::distance(m_gridViews.begin(), it) * 2; // a section has a label and a grid
    QTC_ASSERT(position <= vbox->count() - 1, position = vbox->count() - 1);
    vbox->insertWidget(position, sectionLabel);
    vbox->insertWidget(position + 1, gridView);

    struct ItemHash
    {
        std::size_t operator()(ListItem *item) const { return std::hash<QString>{}(item->name); }
    };
    struct ItemEqual
    {
        bool operator()(ListItem *lhs, ListItem *rhs) const
        {
            return lhs->name == rhs->name && lhs->description == rhs->description;
        }
    };

    // add the items also to the all products model to be able to search correctly
    const QList<ListItem *> allItems = m_allItemsModel->items();
    const std::unordered_set<ListItem *, ItemHash, ItemEqual> uniqueItems{allItems.constBegin(),
                                                                          allItems.constEnd()};
    const QList<ListItem *> newItems = filtered(items, [&uniqueItems](ListItem *item) {
        return uniqueItems.find(item) == uniqueItems.end();
    });
    m_allItemsModel->appendItems(newItems);

    // only show section label(s) if there is more than one section
    m_sectionLabels.at(0)->setVisible(m_sectionLabels.size() > 1);

    return model;
}

void SectionedGridView::clear()
{
    m_allItemsModel->clear();
    qDeleteAll(m_sectionModels);
    qDeleteAll(m_sectionLabels);
    qDeleteAll(m_gridViews);
    m_sectionModels.clear();
    m_sectionLabels.clear();
    m_gridViews.clear();
    m_allItemsView.reset();
}

void SectionedGridView::zoomInSection(const Section &section)
{
    auto zoomedInWidget = new QWidget(this);
    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    zoomedInWidget->setLayout(layout);

    QLabel *backLink = createLinkLabel("&lt; " + Tr::tr("Back"), this);
    connect(backLink, &QLabel::linkActivated, this, [this, zoomedInWidget] {
        removeWidget(zoomedInWidget);
        delete zoomedInWidget;
        setCurrentIndex(0);
    });
    using namespace Layouting;
    QWidget *sectionLabel = Row {
        section.name,
        createSeparator(this),
        backLink,
        Space(HSpacing),
        noMargin
    }.emerge();
    sectionLabel->setContentsMargins(0, ItemGap, 0, 0);
    sectionLabel->setFont(Core::WelcomePageHelpers::brandFont());

    auto gridView = new GridView(zoomedInWidget);
    gridView->setItemDelegate(m_itemDelegate);
    gridView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gridView->setModel(m_sectionModels.value(section));

    layout->addWidget(sectionLabel);
    layout->addWidget(gridView);

    m_zoomedInWidget = zoomedInWidget;
    addWidget(zoomedInWidget);
    setCurrentWidget(zoomedInWidget);
}

Section::Section(const QString &name, int priority)
    : name(name)
    , priority(priority)
{}

Section::Section(const QString &name, int priority, std::optional<int> maxRows)
    : name(name)
    , priority(priority)
    , maxRows(maxRows)
{}

} // namespace Core
