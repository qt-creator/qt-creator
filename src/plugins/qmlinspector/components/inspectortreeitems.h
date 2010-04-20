#ifndef INSPECTORTREEITEMS_H
#define INSPECTORTREEITEMS_H

#include <QTreeWidgetItem>
#include <QObject>
#include <private/qdeclarativedebug_p.h>

namespace Qml {
namespace Internal {


class ObjectTreeItem : public QTreeWidgetItem
{
public:
    explicit ObjectTreeItem(QTreeWidget *widget, int type = 0);
    ObjectTreeItem(QTreeWidgetItem *parentItem, int type = 0);
    QVariant data (int column, int role) const;
    void setData (int column, int role, const QVariant & value);

    void setHasValidDebugId(bool value);


private:
    bool m_hasValidDebugId;
};

class PropertiesViewItem : public QTreeWidgetItem
{
public:
    enum Type {
        BindingType,
        OtherType,
        ClassType,
    };
    enum DataRoles {
        CanEditRole = Qt::UserRole + 1,
        ObjectIdStringRole = Qt::UserRole + 50,
        ClassDepthRole = Qt::UserRole + 51
    };

    PropertiesViewItem(QTreeWidget *widget, Type type = OtherType);
    PropertiesViewItem(QTreeWidgetItem *parent, Type type = OtherType);
    QVariant data (int column, int role) const;
    void setData (int column, int role, const QVariant & value);

    QDeclarativeDebugPropertyReference property;
    Type type;
private:
    QString objectIdString() const;

};

} // Internal
} // Qml

#endif // INSPECTORTREEITEMS_H
