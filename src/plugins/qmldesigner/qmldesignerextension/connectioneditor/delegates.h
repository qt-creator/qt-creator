/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QComboBox>

namespace QmlDesigner {

namespace Internal {

class PropertiesComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText USER true)
public:
    PropertiesComboBox(QWidget *parent = 0);

    virtual QString text() const;
    void setText(const QString &text);
    void disableValidator();
};

class ConnectionComboBox : public PropertiesComboBox
{
    Q_OBJECT
public:
    ConnectionComboBox(QWidget *parent = 0);
    QString text() const override;
};

class ConnectionEditorDelegate : public QStyledItemDelegate
{
public:
    ConnectionEditorDelegate(QWidget *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class BindingDelegate : public ConnectionEditorDelegate
{
public:
    BindingDelegate(QWidget *parent = 0);
    virtual QWidget *createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const override;
};

class DynamicPropertiesDelegate : public ConnectionEditorDelegate
{
public:
    DynamicPropertiesDelegate(QWidget *parent = 0);
    virtual QWidget *createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const override;
};


class ConnectionDelegate : public ConnectionEditorDelegate
{
    Q_OBJECT
public:
    ConnectionDelegate(QWidget *parent = 0);
    virtual QWidget *createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const override;
};

class BackendDelegate : public ConnectionEditorDelegate
{
    Q_OBJECT
public:
    BackendDelegate(QWidget *parent = 0);
    virtual QWidget *createEditor(QWidget *parent,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const override;
};

} // namespace Internal

} // namespace QmlDesigner
