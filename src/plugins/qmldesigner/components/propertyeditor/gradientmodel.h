/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GRADIENTMODEL_H
#define GRADIENTMODEL_H

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

    Q_INVOKABLE void addStop(qreal position, const QColor &color);
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

#endif // GRADIENTMODEL_H
