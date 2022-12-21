// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    PropertiesComboBox(QWidget *parent = nullptr);

    virtual QString text() const;
    void setText(const QString &text);
    void disableValidator();
};

class ConnectionComboBox : public PropertiesComboBox
{
    Q_OBJECT
public:
    ConnectionComboBox(QWidget *parent = nullptr);
    QString text() const override;
};

class ConnectionEditorDelegate : public QStyledItemDelegate
{
public:
    ConnectionEditorDelegate(QWidget *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class BindingDelegate : public ConnectionEditorDelegate
{
public:
    BindingDelegate(QWidget *parent = nullptr);
    QWidget *createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const override;
};

class DynamicPropertiesDelegate : public ConnectionEditorDelegate
{
public:
    DynamicPropertiesDelegate(QWidget *parent = nullptr);
    QWidget *createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const override;
};


class ConnectionDelegate : public ConnectionEditorDelegate
{
    Q_OBJECT
public:
    ConnectionDelegate(QWidget *parent = nullptr);
    QWidget *createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const override;
};

class BackendDelegate : public ConnectionEditorDelegate
{
    Q_OBJECT
public:
    BackendDelegate(QWidget *parent = nullptr);
    QWidget *createEditor(QWidget *parent,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const override;
};

} // namespace Internal

} // namespace QmlDesigner
