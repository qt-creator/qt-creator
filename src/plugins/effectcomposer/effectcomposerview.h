// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>
#include <modelnode.h>

#include <generatedcomponentutils.h>
#include <coreplugin/icontext.h>

#include <QPointer>

namespace EffectComposer {

class EffectComposerWidget;

class EffectComposerContext : public Core::IContext
{
    Q_OBJECT

public:
    EffectComposerContext(QWidget *widget);
    void contextHelp(const Core::IContext::HelpCallback &callback) const override;
};

class EffectComposerView : public QmlDesigner::AbstractView
{
    Q_DECLARE_TR_FUNCTIONS(EffectComposer::EffectComposerView)
public:
    EffectComposerView(QmlDesigner::ExternalDependenciesInterface &externalDependencies);
    ~EffectComposerView() override;

    bool hasWidget() const override;
    QmlDesigner::WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(QmlDesigner::Model *model) override;
    void modelAboutToBeDetached(QmlDesigner::Model *model) override;
    void selectedNodesChanged(const QList<QmlDesigner::ModelNode> &selectedNodeList,
                              const QList<QmlDesigner::ModelNode> &lastSelectedNodeList) override;
    void nodeAboutToBeRemoved(const QmlDesigner::ModelNode &removedNode) override;

private:
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<QmlDesigner::ModelNode> &nodeList, const QList<QVariant> &data) override;
    void removeUnusedEffectImports();

    QPointer<EffectComposerWidget> m_widget;
    QString m_currProjectPath;
    QmlDesigner::GeneratedComponentUtils m_componentUtils;
};

} // namespace EffectComposer
