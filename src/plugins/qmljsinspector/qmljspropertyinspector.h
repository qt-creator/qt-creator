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
#ifndef PROPERTYINSPECTOR_H
#define PROPERTYINSPECTOR_H

#include <qmljsprivateapi.h>
#include <QtGui/QTreeView>
#include <QtGui/QStandardItemModel>
#include <QtGui/QSortFilterProxyModel>

QT_BEGIN_NAMESPACE

QT_END_NAMESPACE

namespace QmlJSInspector {
namespace Internal {

class PropertyEditDelegate;

class PropertiesFilter : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    PropertiesFilter(QObject *parent=0):QSortFilterProxyModel(parent) { setDynamicSortFilter(true); }
    ~PropertiesFilter() { }
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
};

class QmlJSPropertyInspector : public QTreeView
{
    Q_OBJECT
public:
    QmlJSPropertyInspector(QWidget *parent = 0);
    void clear();

    QList <int> currentObjects() const;

signals:
    void changePropertyValue(int debugId, QString propertyName, QString valueExpression);

public slots:
    void setCurrentObjects(const QList<QDeclarativeDebugObjectReference> &);
    void propertyValueEdited(const int objectId,const QString& propertyName, const QString& propertyValue);
    void propertyValueChanged(int debugId, const QByteArray &propertyName, const QVariant &propertyValue);
    void filterBy(const QString &expression);

private:
    friend class PropertyEditDelegate;
    void buildPropertyTree(const QDeclarativeDebugObjectReference &);
    void addRow(const QString &name, const QString &value, const QString &type,
                const int debugId=-1, bool editable=true);

    QVariant getData(int row, int column, int role) const;

    QStandardItemModel m_model;
    PropertiesFilter *m_filter;
    QList<int> m_currentObjects;
};

} // Internal
} // QmlJSInspector

#endif
