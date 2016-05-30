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

#include <qmlprofiler/flamegraph.h>
#include <QObject>
#include <QStandardItemModel>
#include <QQmlComponent>
#include <QQuickItem>

namespace QmlProfiler {
namespace Internal {

class DelegateObject : public QQuickItem
{
    Q_OBJECT
};

class DelegateComponent : public QQmlComponent
{
    Q_OBJECT
public:
    QObject *create(QQmlContext *context) override;
    QObject *beginCreate(QQmlContext *) override;
    void completeCreate() override;
};

class FlameGraphTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void testRebuild();
    void cleanupTestCase();

private:
    static const int sizeRole = Qt::UserRole + 1;
    static const int dataRole = Qt::UserRole + 2;
    FlameGraph flameGraph;
    QStandardItemModel model;
    DelegateComponent delegate;
};


} // namespace Internal
} // namespace QmlProfiler
