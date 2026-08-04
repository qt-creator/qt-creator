// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilereventsview.h"
#include "qmlprofilerfindingsmodel.h"
#include "qmlprofilermodelmanager.h"

#include <utils/itemviews.h>

#include <memory>

namespace Profiler::Internal {

class QmlProfilerFindingsMainView;

class QmlProfilerFindingsView final : public QmlProfilerEventsView
{
public:
    explicit QmlProfilerFindingsView(QmlProfilerModelManager *profilerModelManager,
                                     QWidget *parent = nullptr);
    ~QmlProfilerFindingsView() override;

    void selectByTypeId(int typeIndex) final;
    void onVisibleFeaturesChanged(quint64 features) final;

protected:
    void contextMenuEvent(QContextMenuEvent *ev) final;

private:
    void exportFindings() const;

    QmlProfilerModelManager *m_modelManager = nullptr;
    std::unique_ptr<QmlProfilerFindingsMainView> m_mainView;
};

class QmlProfilerFindingsMainView final : public Utils::TreeView
{
    Q_OBJECT

public:
    explicit QmlProfilerFindingsMainView(QmlProfilerFindingsModel *model);

    void selectByTypeId(int typeIndex);
    const QList<Finding> &findings() const { return m_model->findings(); }

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void typeClicked(int typeIndex);

private:
    std::unique_ptr<QmlProfilerFindingsModel> m_model;
};

} // namespace Profiler::Internal
