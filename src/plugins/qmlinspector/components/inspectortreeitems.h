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
        BindingType = QTreeWidgetItem::UserType,
        OtherType = QTreeWidgetItem::UserType + 1,
        ClassType = QTreeWidgetItem::UserType + 2
    };
    enum DataRoles {
        CanEditRole = Qt::UserRole,
        ObjectIdStringRole = Qt::UserRole + 1,
        ClassDepthRole = Qt::UserRole + 2
    };

    PropertiesViewItem(QTreeWidget *widget, int type = OtherType);
    PropertiesViewItem(QTreeWidgetItem *parent, int type = OtherType);
    QVariant data (int column, int role) const;
    void setData (int column, int role, const QVariant & value);
    void setWatchingDisabled(bool disabled);
    bool isWatchingDisabled() const;
    QDeclarativeDebugPropertyReference property;

private:
    QString objectIdString() const;
    bool m_disabled;

};

} // Internal
} // Qml

#endif // INSPECTORTREEITEMS_H
