// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"

#include <QPointer>

namespace QmlDesigner {

class EffectMakerWidget;

class EffectMakerView : public AbstractView
{
public:
    EffectMakerView(ExternalDependenciesInterface &externalDependencies);
    ~EffectMakerView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

private:
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;

    QPointer<EffectMakerWidget> m_widget;
};

} // namespace QmlDesigner
