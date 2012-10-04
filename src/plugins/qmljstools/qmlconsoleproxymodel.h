#ifndef QMLCONSOLEPROXYMODEL_H
#define QMLCONSOLEPROXYMODEL_H

#include "qmlconsoleitem.h"

#include <QSortFilterProxyModel>
#include <QItemSelectionModel>

namespace QmlJSTools {
namespace Internal {

class QmlConsoleProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit QmlConsoleProxyModel(QObject *parent);

public slots:
    void setShowLogs(bool show);
    void setShowWarnings(bool show);
    void setShowErrors(bool show);
    void selectEditableRow(const QModelIndex &index,
                               QItemSelectionModel::SelectionFlags command);
    void onRowsInserted(const QModelIndex &index, int start, int end);

signals:
    void scrollToBottom();
    void setCurrentIndex(const QModelIndex &index,
                         QItemSelectionModel::SelectionFlags command);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    QFlags<QmlConsoleItem::ItemType> m_filter;
};

} // Internal
} // QmlJSTools

#endif // QMLCONSOLEPROXYMODEL_H
