#include "qmlconsoleproxymodel.h"
#include "qmlconsoleitemmodel.h"

namespace QmlJSTools {
namespace Internal {

QmlConsoleProxyModel::QmlConsoleProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent),
    m_filter(QmlConsoleItem::DefaultTypes)
{
}

void QmlConsoleProxyModel::setShowLogs(bool show)
{
    m_filter = show ? m_filter | QmlConsoleItem::DebugType : m_filter & ~QmlConsoleItem::DebugType;
    setFilterRegExp(QString());
}

void QmlConsoleProxyModel::setShowWarnings(bool show)
{
    m_filter = show ? m_filter | QmlConsoleItem::WarningType
                    : m_filter & ~QmlConsoleItem::WarningType;
    setFilterRegExp(QString());
}

void QmlConsoleProxyModel::setShowErrors(bool show)
{
    m_filter = show ? m_filter | QmlConsoleItem::ErrorType : m_filter & ~QmlConsoleItem::ErrorType;
    setFilterRegExp(QString());
}

void QmlConsoleProxyModel::selectEditableRow(const QModelIndex &index,
                           QItemSelectionModel::SelectionFlags command)
{
    emit setCurrentIndex(mapFromSource(index), command);
}

bool QmlConsoleProxyModel::filterAcceptsRow(int sourceRow,
         const QModelIndex &sourceParent) const
 {
     QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
     return m_filter.testFlag((QmlConsoleItem::ItemType)sourceModel()->data(
                                  index, QmlConsoleItemModel::TypeRole).toInt());
 }

void QmlConsoleProxyModel::onRowsInserted(const QModelIndex &index, int start, int end)
{
    int rowIndex = end;
    do {
        if (filterAcceptsRow(rowIndex, index)) {
            emit scrollToBottom();
            break;
        }
    } while (--rowIndex >= start);
}

} // Internal
} // QmlJSTools
