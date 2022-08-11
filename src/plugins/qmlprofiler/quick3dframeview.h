/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qmlprofilermodelmanager.h"
#include "quick3dframemodel.h"
#include "qmlprofilereventsview.h"
#include "qmlprofilereventtypes.h"

#include <utils/itemviews.h>

#include <QPointer>
#include <QSortFilterProxyModel>

#include <memory>

namespace QmlProfiler {
namespace Internal {

class Quick3DMainView;

class Quick3DFrameView : public QmlProfilerEventsView
{
    Q_OBJECT
public:
    explicit Quick3DFrameView(QmlProfilerModelManager *profilerModelManager,
                                         QWidget *parent = nullptr);
    ~Quick3DFrameView() override = default;

    void selectByTypeId(int typeIndex) override;
    void onVisibleFeaturesChanged(quint64 features) override;

private:

    std::unique_ptr<Quick3DMainView> m_mainView;
    std::unique_ptr<Quick3DMainView> m_compareFrameView;
};

class Quick3DMainView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit Quick3DMainView(Quick3DFrameModel *model, bool compareView, QWidget *parent = nullptr);
    ~Quick3DMainView() override = default;

    void setFilterView3D(const QString &objectName);
    void setFilterFrame(const QString &objectName);

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);

private:
    Quick3DFrameModel *m_model;
    QSortFilterProxyModel *m_sortModel;
    bool m_compareView;
};

} // namespace Internal
} // namespace QmlProfiler
