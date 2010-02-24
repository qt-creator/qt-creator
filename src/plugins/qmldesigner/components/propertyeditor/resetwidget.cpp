/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/


#include "qml.h"
#include "resetwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringList>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>

#include <QDebug>
#include <QApplication>

QML_DECLARE_TYPE(QmlDesigner::ResetWidget);
QML_DEFINE_TYPE(Bauhaus, 1, 0, ResetWidget, QmlDesigner::ResetWidget);

namespace QmlDesigner {


ResetWidget::ResetWidget(QWidget *parent) : QGroupBox(parent), m_backendObject(0)
{
    m_vlayout = new QVBoxLayout(this);
    m_vlayout->setContentsMargins(2,2,2,2);

    QPushButton *b = new QPushButton(this);
    b->setText("reset all properties");
    m_vlayout->addWidget(b);

    setLayout(m_vlayout);
}

void ResetWidget::resetView()
{
    m_tableWidget->clear();
    delete m_tableWidget;
    setupView();
}

void ResetWidget::setupView()
{
    m_tableWidget = new QTableWidget(this);
    m_vlayout->addWidget(m_tableWidget);

    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->horizontalHeader()->hide();
    m_tableWidget->verticalHeader()->hide();
    m_tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tableWidget->setShowGrid(false);
    m_tableWidget->setSortingEnabled(true);
    m_tableWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    const QMetaObject *metaObject = m_backendObject->metaObject();
    int count = metaObject->propertyCount();

    m_tableWidget->setColumnCount(3);
    m_tableWidget->setRowCount(count);
    for (int i=0;i<count;i++) {
        QMetaProperty metaProperty = metaObject->property(i);
        addPropertyItem(metaProperty.name(), i);
    }
    m_tableWidget->resizeRowsToContents();
    m_tableWidget->resizeColumnsToContents();
    m_tableWidget->sortItems(0);
    m_tableWidget->setColumnWidth(2, 40);
    parentWidget()->resize(parentWidget()->width(), count * 28);
    qApp->processEvents();

}

void ResetWidget::addPropertyItem(const QString &name, int row)
{
    QTableWidgetItem *newItem = new QTableWidgetItem(name);
    m_tableWidget->setItem(row, 0, newItem);
    ResetWidgetPushButton *b = new  ResetWidgetPushButton(m_tableWidget);
    b->setName(name);
    b->setText("reset");
    connect(b, SIGNAL(pressed(const QString &)), this, SLOT(buttonPressed(const QString &)));
    b->setMaximumHeight(15);
    b->setMinimumHeight(10);
    m_tableWidget->setCellWidget(row, 2, b);
}

void ResetWidget::buttonPressed(const QString &)
{
}

ResetWidgetPushButton::ResetWidgetPushButton(QWidget *parent) : QPushButton(parent)
{
    connect(this, SIGNAL(pressed()), this, SLOT(myPressed()));
}

void ResetWidgetPushButton::myPressed()
{
    emit pressed(m_name);
}


}

