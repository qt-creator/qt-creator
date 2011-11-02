/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
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

#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>

#include "customcolordialog.h"

QT_BEGIN_NAMESPACE

QT_END_NAMESPACE

namespace QmlJSInspector {
namespace Internal {

class PropertyEditDelegate;

class PropertiesFilter : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PropertiesFilter(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(true);
    }

    ~PropertiesFilter() { }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
};

class ExpressionEdit : public QDialog
{
    Q_OBJECT
public:
    explicit ExpressionEdit(const QString &title, QDialog *parent = 0);

    QString expression() const;
    void setItemData(int objectId, const QString &propertyName);

    virtual void accept();

signals:
    void dataChanged(int debugId, const QString &paramName, const QString &newExpression);

private:
    QDialogButtonBox *m_buttonBox;
    QLineEdit *m_exprInput;
    int m_debugId;
    QString m_paramName;
};

class ColorChooserDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ColorChooserDialog(const QString &title, QDialog *parent = 0);

    void setItemData(int objectId,const QString &propertyName, const QString &colorName);

public slots:
    void acceptColor(const QColor &color);

signals:
    void dataChanged(int debugId, const QString &paramName, const QString &newExpression);


private:
    int m_debugId;
    QString m_paramName;
    QmlEditorWidgets::CustomColorDialog *m_mainFrame;
};

class QmlJSPropertyInspector : public QTreeView
{
    Q_OBJECT
public:
    enum PropertyType
    {
        BooleanType,
        NumberType,
        StringType,
        ColorType,
        OtherType
    };

    explicit QmlJSPropertyInspector(QWidget *parent = 0);
    void clear();

signals:
    void changePropertyValue(int debugId, QString propertyName, QString valueExpression);
    void customContextMenuRequested(const QPoint &pos);

public slots:
    void setCurrentObjects(const QList<QDeclarativeDebugObjectReference> &);
    void propertyValueEdited(const int objectId,const QString &propertyName, const QString &propertyValue);
    void propertyValueChanged(int debugId, const QByteArray &propertyName, const QVariant &propertyValue);
    void filterBy(const QString &expression);

    void openExpressionEditor(const QModelIndex &itemIndex);
    void openColorSelector(const QModelIndex &itemIndex);

private:
    friend class PropertyEditDelegate;
    void buildPropertyTree(const QDeclarativeDebugObjectReference &);
    void addRow(const QString &name, const QString &value, const QString &type,
                const int debugId = -1, bool editable = true);
    void setColorIcon(int row);

    QVariant getData(int row, int column, int role) const;
    QmlJSPropertyInspector::PropertyType getTypeFor(int row) const;

    void contextMenuEvent(QContextMenuEvent *ev);

    QStandardItemModel m_model;
    PropertiesFilter *m_filter;
    QList<int> m_currentObjects;
};

} // Internal
} // QmlJSInspector

#endif
