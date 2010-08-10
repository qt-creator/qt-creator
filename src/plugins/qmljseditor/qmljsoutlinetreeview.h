#ifndef QMLJSOUTLINETREEVIEW_H
#define QMLJSOUTLINETREEVIEW_H

#include <utils/navigationtreeview.h>
#include <QtGui/QStyledItemDelegate>

namespace QmlJSEditor {
namespace Internal {

class QmlJSOutlineItemDelegate : public QStyledItemDelegate
{
public:
    explicit QmlJSOutlineItemDelegate(QObject *parent = 0);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
};

class QmlJSOutlineTreeView : public Utils::NavigationTreeView
{
    Q_OBJECT
public:
    explicit QmlJSOutlineTreeView(QWidget *parent = 0);
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSOUTLINETREEVIEW_H
