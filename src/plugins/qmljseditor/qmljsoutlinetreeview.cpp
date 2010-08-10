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
        opt.text += "   " + annotation;

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

    // paint type name
    QRect typeRect = style->itemTextRect(opt.fontMetrics, opt.rect, Qt::AlignLeft, true, typeString);

    QPalette::ColorRole textColorRole = QPalette::Text;
    if (option.state & QStyle::State_Selected) {
        textColorRole = QPalette::HighlightedText;
    }

    style->drawItemText(painter, typeRect, Qt::AlignLeft, opt.palette, true, typeString, textColorRole);

    // paint annotation (e.g. id)
    if (!annotationString.isEmpty()) {
        QRect annotationRect = style->itemTextRect(opt.fontMetrics, opt.rect, Qt::AlignLeft, true,
                                                   annotationString);
        static int space = opt.fontMetrics.width("   ");
        annotationRect.adjust(typeRect.width() + space, 0, typeRect.width() + space, 0);

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
