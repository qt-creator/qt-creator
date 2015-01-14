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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
