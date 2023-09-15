// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesigner/components/propertyeditor/qmlmodelnodeproxy.h"

#include <coreplugin/icontext.h>

#include <QFrame>

class StudioQuickWidget;

namespace EffectMaker {

class EffectMakerView;
class EffectMakerModel;
class EffectMakerNodesModel;

class EffectMakerWidget : public QFrame
{
    Q_OBJECT

public:
    EffectMakerWidget(EffectMakerView *view);
    ~EffectMakerWidget() = default;

    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    static QString qmlSourcesPath();
    void clearSearchFilter();

    void delayedUpdateModel();
    void updateModel();
    void initView();

    StudioQuickWidget *quickWidget() const;
    QPointer<EffectMakerModel> effectMakerModel() const;
    QPointer<EffectMakerNodesModel> effectMakerNodesModel() const;

    Q_INVOKABLE void addEffectNode(const QString &nodeQenPath);
    Q_INVOKABLE void focusSection(int section);

    QSize sizeHint() const override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();

    QPointer<EffectMakerModel> m_effectMakerModel;
    QPointer<EffectMakerNodesModel> m_effectMakerNodesModel;
    QPointer<EffectMakerView> m_effectMakerView;
    QPointer<StudioQuickWidget> m_quickWidget;
    QmlDesigner::QmlModelNodeProxy m_backendModelNode;
};

} // namespace EffectMaker

