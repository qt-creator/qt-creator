#ifndef %ClassName:u%_H
#define %ClassName:u%_H

#include <QAbstractListModel>
#include <QList>

class %ClassName% : public QAbstractListModel {
    Q_OBJECT
public:
    explicit %ClassName%(QObject *parent);
    void addItems(const QList<%Datatype%> &items);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    QList<%Datatype%> items;
};

#endif // %ClassName:u%_H

