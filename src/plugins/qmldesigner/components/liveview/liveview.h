// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>
#include <QPointer>

namespace QmlDesigner {

class LiveViewWidget;

class  LiveView : public AbstractView
{
    Q_OBJECT

public:
    LiveView(ExternalDependenciesInterface &externalDependencies);
    ~LiveView() override;

    // AbstractView
    void modelAttached(Model *model) override;

    WidgetInfo widgetInfo() override;
    bool hasWidget() const override;

private:
    QPointer<LiveViewWidget> m_liveViewWidget;
};

} // namespace QmlDesigner
