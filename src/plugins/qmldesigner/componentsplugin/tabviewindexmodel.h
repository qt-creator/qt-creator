/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef TABVIEWINDEXMODEL_H
#define TABVIEWINDEXMODEL_H

#include <qmlitemnode.h>

#include <QObject>
#include <QtQml>

class TabViewIndexModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(QStringList tabViewIndexModel READ tabViewIndexModel NOTIFY modelNodeBackendChanged)

public:
    explicit TabViewIndexModel(QObject *parent = 0);

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    void setModelNode(const QmlDesigner::ModelNode &modelNode);
    QStringList tabViewIndexModel() const;
    void setupModel();

    static void registerDeclarativeType();

signals:
    void modelNodeBackendChanged();

private:
    QVariant modelNodeBackend() const;

    QmlDesigner::ModelNode m_modelNode;
    QStringList m_tabViewIndexModel;

};

QML_DECLARE_TYPE(TabViewIndexModel)

#endif // TABVIEWINDEXMODEL_H
