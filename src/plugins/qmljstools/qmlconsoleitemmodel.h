#ifndef QMLCONSOLEITEMMODEL_H
#define QMLCONSOLEITEMMODEL_H

#include "qmlconsoleitem.h"

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QFont>

namespace QmlJSTools {
namespace Internal {

class QmlConsoleItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Roles { TypeRole = Qt::UserRole, FileRole, LineRole };

    explicit QmlConsoleItemModel(QObject *parent = 0);
    ~QmlConsoleItemModel();

    void setHasEditableRow(bool hasEditableRow);
    bool hasEditableRow() const;
    void appendEditableRow();
    void removeEditableRow();

    bool appendItem(QmlConsoleItem *item, int position = -1);
    bool appendMessage(QmlConsoleItem::ItemType itemType, const QString &message,
                       int position = -1);

    QAbstractItemModel *model() { return this; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    int sizeOfFile(const QFont &font);
    int sizeOfLineNumber(const QFont &font);

    QmlConsoleItem *root() const { return m_rootItem; }

public slots:
    void clear();

signals:
    void selectEditableRow(const QModelIndex &index, QItemSelectionModel::SelectionFlags flags);
    void rowInserted(const QModelIndex &index);

protected:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;


    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex());

    QmlConsoleItem *getItem(const QModelIndex &index) const;

private:
    bool m_hasEditableRow;
    QmlConsoleItem *m_rootItem;
    int m_maxSizeOfFileName;
};

} // Internal
} // QmlJSTools

#endif // QMLCONSOLEITEMMODEL_H
