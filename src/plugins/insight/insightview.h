// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <3rdparty/json/json.hpp>

namespace QmlDesigner {

class InsightModel;
class InsightWidget;

class InsightView : public AbstractView
{
    Q_OBJECT

public:
    explicit InsightView(ExternalDependenciesInterface &externalDependencies);
    ~InsightView() override;

    // AbstractView
    void modelAttached(Model *model) override;

    WidgetInfo widgetInfo() override;
    bool hasWidget() const override;

public slots:

private:
    std::unique_ptr<InsightModel> m_insightModel;
    QPointer<InsightWidget> m_insightWidget;
};

} // namespace QmlDesigner
