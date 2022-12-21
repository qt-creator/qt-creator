// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidgetAction>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class Model;
class ComponentView;
class ModelNode;

class ComponentAction : public QWidgetAction
{
    Q_OBJECT
public:
    ComponentAction(ComponentView  *componentView);
    void setCurrentIndex(int index);
    void emitCurrentComponentChanged(int index);

protected:
    QWidget *createWidget(QWidget *parent) override;

signals:
    void currentComponentChanged(const ModelNode &node);
    void changedToMaster();
    void currentIndexChanged(int index);

private:
    QPointer<ComponentView> m_componentView;
    bool dontEmitCurrentComponentChanged;
};

} // namespace QmlDesigner
