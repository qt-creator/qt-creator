#include "qmljsoutlinetreeview.h"
#include "qmloutlinemodel.h"

#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QtGui/QPainter>

namespace QmlJSEditor {
namespace Internal {

QmlJSOutlineItemDelegate::QmlJSOutlineItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

QSize QmlJSOutlineItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    const QString annotation = index.data(QmlOutlineModel::AnnotationRole).toString();
    if (!annotation.isEmpty())
        opt.text += "     " + annotation;

    QStyle *style = QApplication::style();
    return style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), 0);
}

void QmlJSOutlineItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
           const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    if (opt.state & QStyle::State_Selected)
        painter->fillRect(opt.rect, option.palette.highlight());

    const QString typeString = index.data(Qt::DisplayRole).toString();
    const QString annotationString = index.data(QmlOutlineModel::AnnotationRole).toString();

    QStyle *style = QApplication::style();

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, 0);

    if (!annotationString.isEmpty()) {
        QPalette::ColorRole textColorRole = QPalette::Text;
        if (option.state & QStyle::State_Selected) {
            textColorRole = QPalette::HighlightedText;
        }

        // calculate sizes of icon, type.
        QPixmap iconPixmap = opt.icon.pixmap(opt.rect.size());
        QRect iconRect = style->itemPixmapRect(opt.rect, Qt::AlignLeft, iconPixmap);
        QRect typeRect = style->itemTextRect(opt.fontMetrics, opt.rect, Qt::AlignLeft, true, typeString);
        QRect annotationRect = style->itemTextRect(opt.fontMetrics, opt.rect, Qt::AlignLeft | Qt::AlignBottom, true,
                                                   annotationString);

        static int space = opt.fontMetrics.width("     ");
        annotationRect.adjust(iconRect.width() + typeRect.width() + space, 0,
                              iconRect.width() + typeRect.width() + space, 0);

        QPalette disabledPalette(opt.palette);
        disabledPalette.setCurrentColorGroup(QPalette::Disabled);
        style->drawItemText(painter, annotationRect, Qt::AlignLeft, disabledPalette, true,
                            annotationString, textColorRole);
    }
}

QmlJSOutlineTreeView::QmlJSOutlineTreeView(QWidget *parent) :
    Utils::NavigationTreeView(parent)
{
    // see also CppOutlineTreeView
    setFocusPolicy(Qt::NoFocus);
    setExpandsOnDoubleClick(false);

    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(InternalMove);

    QmlJSOutlineItemDelegate *itemDelegate = new QmlJSOutlineItemDelegate(this);
    setItemDelegateForColumn(0, itemDelegate);
}

} // namespace Internal
} // namespace QmlJSEditor
