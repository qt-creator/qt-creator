/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "qmljspropertyinspector.h"
#include <utils/qtcassert.h>
#include <qdeclarativeproperty.h>

#include <QtGui/QHeaderView>
#include <QtGui/QItemDelegate>
#include <QtGui/QLineEdit>

namespace QmlJSInspector {
namespace Internal {

// *************************************************************************
//  PropertyEdit
// *************************************************************************

class PropertyEditDelegate : public QItemDelegate
{
    public:
    explicit PropertyEditDelegate(QObject *parent=0) : QItemDelegate(parent),
        m_treeWidget(dynamic_cast<QmlJSPropertyInspector *>(parent)) {}

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
    {
        Q_UNUSED(option);
        if (index.column() != 1)
            return 0;

        return new QLineEdit(parent);
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        QVariant data = m_treeWidget->getData(index.row(),1,Qt::DisplayRole);
        QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
        lineEdit->setText(data.toString());
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        Q_UNUSED(model);

        int objectId = m_treeWidget->getData(index.row(),0,Qt::UserRole).toInt();
        if (objectId == -1)
            return;

        QString propertyName = m_treeWidget->getData(index.row(),0,Qt::DisplayRole).toString();

        QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
        QString propertyValue = lineEdit->text();

        // add quotes if it's a string
        if ( isStringType( m_treeWidget->getData(index.row(),2,Qt::DisplayRole).toString() ) ) {
            QChar quote('\"');
            if (!propertyValue.startsWith(quote))
                propertyValue = quote + propertyValue;
            if (!propertyValue.endsWith(quote))
                propertyValue += quote;
        }

        m_treeWidget->propertyValueEdited(objectId, propertyName, propertyValue);

        lineEdit->clearFocus();
    }

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
    {
        Q_UNUSED(index);
        QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
        lineEdit->setGeometry(option.rect);
    }

private:
    bool isStringType(const QString &typeName) const {
        return typeName=="QString" || typeName=="QColor";
    }

private:
    QmlJSPropertyInspector *m_treeWidget;

};

// *************************************************************************
//  FILTER
// *************************************************************************
bool PropertiesFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    QModelIndex index1 = sourceModel()->index(sourceRow, 1, sourceParent);
    QModelIndex index2 = sourceModel()->index(sourceRow, 2, sourceParent);

    return (sourceModel()->data(index0).toString().contains(filterRegExp())
                    || sourceModel()->data(index1).toString().contains(filterRegExp())
                    || sourceModel()->data(index2).toString().contains(filterRegExp()));
}

// *************************************************************************
//  QmlJSObjectTree
// *************************************************************************
inline QString cleanPropertyValue(QString propertyValue)
{
    if (propertyValue == QString("<unknown value>"))
        return QString();
    if (propertyValue == QString("<unnamed object>"))
        return QString();
    return propertyValue;
}

QmlJSPropertyInspector::QmlJSPropertyInspector(QWidget *parent)
    : QTreeView(parent)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setExpandsOnDoubleClick(true);

    header()->setResizeMode(QHeaderView::ResizeToContents);
    header()->setMinimumSectionSize(150);
    setRootIsDecorated(false);

    setItemDelegateForColumn(1, new PropertyEditDelegate(this));

    m_filter = new PropertiesFilter(this);
    m_filter->setSourceModel(&m_model);
    setModel(m_filter);
}

void QmlJSPropertyInspector::filterBy(const QString &expression)
{
    m_filter->setFilterWildcard(expression);
    m_filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void QmlJSPropertyInspector::clear()
{
    m_model.clear();
    m_currentObjects.clear();
}

QList <int> QmlJSPropertyInspector::currentObjects() const
{
    return m_currentObjects;
}

void QmlJSPropertyInspector::setCurrentObjects(const QList<QDeclarativeDebugObjectReference> &objectList)
{
    if (objectList.isEmpty())
        return;

    clear();

    foreach ( QDeclarativeDebugObjectReference obj, objectList) {
        m_currentObjects << obj.debugId();
        buildPropertyTree(obj);
    }
}

QVariant QmlJSPropertyInspector::getData(int row, int column, int role) const
{
    return m_filter->data(m_filter->index(row,column),role);
}

void QmlJSPropertyInspector::propertyValueChanged(int debugId, const QByteArray &propertyName, const QVariant &propertyValue)
{
    if (m_model.rowCount() == 0)
        return;

    QString propertyNameS = QString(propertyName);
    for (int ii=0; ii < m_model.rowCount(); ii++) {
        if (m_model.data(m_model.index(ii,0),Qt::DisplayRole).toString() == propertyNameS &&
                m_model.data(m_model.index(ii,0),Qt::UserRole).toInt() == debugId) {
            QVariant oldData = m_model.data(m_model.index(ii,1),Qt::DisplayRole);
            m_model.setData(m_model.index(ii,1),propertyValue.toString(), Qt::DisplayRole);
            if (oldData != propertyValue) {
                m_model.item(ii,0)->setForeground(QBrush(Qt::red));
                m_model.item(ii,1)->setForeground(QBrush(Qt::red));
                m_model.item(ii,2)->setForeground(QBrush(Qt::red));
            }
            break;
        }
    }
}

void QmlJSPropertyInspector::propertyValueEdited(const int objectId,const QString& propertyName, const QString& propertyValue)
{
    emit changePropertyValue(objectId, propertyName, propertyValue);
}

void QmlJSPropertyInspector::buildPropertyTree(const QDeclarativeDebugObjectReference &obj)
{
    // Strip off the misleading metadata
    QString objTypeName = obj.className();
    QString declarativeString("QDeclarative");
    if (objTypeName.startsWith(declarativeString)) {
        objTypeName = objTypeName.mid(declarativeString.length()).section('_',0,0);
    }

    // class
    addRow(QString("class"),
           objTypeName,
           QString("qmlType"),
           obj.debugId(),
           false);

    // id
    if (!obj.idString().isEmpty()) {
        addRow(QString("id"),
               obj.idString(),
               QString("idString"),
               obj.debugId(),
               false);
    }

    foreach (const QDeclarativeDebugPropertyReference &prop, obj.properties()) {
        QString propertyName = prop.name();

        if (cleanPropertyValue(prop.value().toString()).isEmpty())
            continue;

        addRow(propertyName, prop.value().toString(), prop.valueTypeName(), obj.debugId(), prop.hasNotifySignal());
    }

    m_model.setHeaderData(0,Qt::Horizontal,QVariant("name"));
    m_model.setHeaderData(1,Qt::Horizontal,QVariant("value"));
    m_model.setHeaderData(2,Qt::Horizontal,QVariant("type"));

}

void QmlJSPropertyInspector::addRow(const QString &name,const QString &value, const QString &type,
                             const int debugId, bool editable)
{
    QStandardItem *nameColumn = new QStandardItem(name);
    nameColumn->setToolTip(name);
    nameColumn->setData(QVariant(debugId),Qt::UserRole);
    nameColumn->setEditable(false);

    QStandardItem *valueColumn = new QStandardItem(value);
    valueColumn->setToolTip(value);
    valueColumn->setEditable(editable);

    QStandardItem *typeColumn = new QStandardItem(type);
    typeColumn->setToolTip(type);
    typeColumn->setEditable(false);

    QList <QStandardItem *> newRow;
    newRow << nameColumn << valueColumn << typeColumn;
    m_model.appendRow(newRow);
}

} // Internal
} // QmlJSInspector
