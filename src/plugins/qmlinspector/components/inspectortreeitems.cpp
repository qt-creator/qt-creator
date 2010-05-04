#include "inspectortreeitems.h"
#include "qmlinspector.h"

#include <QtGui/QApplication>

namespace Qml {
namespace Internal {

// *************************************************************************
//  ObjectTreeItem
// *************************************************************************

ObjectTreeItem::ObjectTreeItem(QTreeWidget *widget, int type) :
    QTreeWidgetItem(widget, type), m_hasValidDebugId(true)
{

}

ObjectTreeItem::ObjectTreeItem(QTreeWidgetItem *parentItem, int type) :
    QTreeWidgetItem(parentItem, type), m_hasValidDebugId(true)
{

}

QVariant ObjectTreeItem::data (int column, int role) const
{
    if (role == Qt::ForegroundRole)
        return m_hasValidDebugId ? qApp->palette().color(QPalette::Foreground) : qApp->palette().color(QPalette::Disabled, QPalette::Foreground);

    return QTreeWidgetItem::data(column, role);
}

void ObjectTreeItem::setData (int column, int role, const QVariant & value)
{
    QTreeWidgetItem::setData(column, role, value);
}

void ObjectTreeItem::setHasValidDebugId(bool value)
{
    m_hasValidDebugId = value;
}

// *************************************************************************
//  PropertiesViewItem
// *************************************************************************
PropertiesViewItem::PropertiesViewItem(QTreeWidget *widget, int type)
    : QTreeWidgetItem(widget, type), m_disabled(false)
{

}

PropertiesViewItem::PropertiesViewItem(QTreeWidgetItem *parent, int type)
    : QTreeWidgetItem(parent, type), m_disabled(false)
{
}

QVariant PropertiesViewItem::data (int column, int role) const
{
    if (role == Qt::ForegroundRole) {
        bool makeDisabledColor = m_disabled;
        if (column == 1 && !data(0, CanEditRole).toBool())
            makeDisabledColor = true;
        return makeDisabledColor ? qApp->palette().color(QPalette::Disabled, QPalette::Foreground) : qApp->palette().color(QPalette::Foreground);
    }


    return QTreeWidgetItem::data(column, role);
}

void PropertiesViewItem::setData (int column, int role, const QVariant & value)
{
    if (role == Qt::EditRole) {
        if (column == 1)
            QmlInspector::instance()->executeExpression(property.objectDebugId(), objectIdString(), property.name(), value);
    }

    QTreeWidgetItem::setData(column, role, value);
}

QString PropertiesViewItem::objectIdString() const
{
    return data(0, ObjectIdStringRole).toString();
}
void PropertiesViewItem::setWatchingDisabled(bool disabled)
{
    m_disabled = disabled;
}

bool PropertiesViewItem::isWatchingDisabled() const
{
    return m_disabled;
}

} // Internal
} // Qml
