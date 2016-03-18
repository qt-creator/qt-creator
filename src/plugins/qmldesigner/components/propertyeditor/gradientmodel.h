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

#include <qmlitemnode.h>

#include <QAbstractListModel>
#include <QColor>
#include <QtQml>

class GradientModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QVariant anchorBackendProperty READ anchorBackend WRITE setAnchorBackend NOTIFY anchorBackendChanged)
    Q_PROPERTY(QString gradientPropertyName READ gradientPropertyName WRITE setGradientPropertyName)
    Q_PROPERTY(int count READ rowCount)
    Q_PROPERTY(bool hasGradient READ hasGradient NOTIFY hasGradientChanged)

public:
    explicit GradientModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QHash<int, QByteArray> roleNames() const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    Q_INVOKABLE int addStop(qreal position, const QColor &color);
    Q_INVOKABLE void addGradient();

    Q_INVOKABLE void setColor(int index, const QColor &color);
    Q_INVOKABLE void setPosition(int index, qreal positition);

    Q_INVOKABLE QColor getColor(int index) const;
    Q_INVOKABLE qreal getPosition(int index) const;

    Q_INVOKABLE void removeStop(int index);
    Q_INVOKABLE void deleteGradient();

    Q_INVOKABLE void lock();
    Q_INVOKABLE void unlock();

    static void registerDeclarativeType();

signals:
    void anchorBackendChanged();
    void hasGradientChanged();

public slots:

private:
    void setupModel();
    void setAnchorBackend(const QVariant &anchorBackend);
    QVariant anchorBackend() const {return QVariant(); }
    QString gradientPropertyName() const;
    void setGradientPropertyName(const QString &name);
    bool hasGradient() const;

private:
    QmlDesigner::QmlItemNode m_itemNode;
    QString m_gradientPropertyName;
    bool m_lock;

};

QML_DECLARE_TYPE(GradientModel)
