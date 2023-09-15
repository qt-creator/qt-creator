// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"

#include <coreplugin/icontext.h>

#include <QPointer>

namespace EffectMaker {

class EffectMakerWidget;

class EffectMakerContext : public Core::IContext
{
    Q_OBJECT

public:
    EffectMakerContext(QWidget *widget);
    void contextHelp(const Core::IContext::HelpCallback &callback) const override;
};

class EffectMakerView : public QmlDesigner::AbstractView
{
public:
    EffectMakerView(QmlDesigner::ExternalDependenciesInterface &externalDependencies);
    ~EffectMakerView() override;

    bool hasWidget() const override;
    QmlDesigner::WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(QmlDesigner::Model *model) override;
    void modelAboutToBeDetached(QmlDesigner::Model *model) override;

private:
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<QmlDesigner::ModelNode> &nodeList, const QList<QVariant> &data) override;

    QPointer<EffectMakerWidget> m_widget;
};

} // namespace EffectMaker

