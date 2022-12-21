// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QElapsedTimer>
#include <QPointer>
#include <QQmlPropertyMap>
#include <QQuickWidget>

QT_BEGIN_NAMESPACE
class QShortcut;
QT_END_NAMESPACE

namespace QmlDesigner {

class StatesEditorModel;
class StatesEditorView;
class NodeInstanceView;

namespace Internal { class StatesEditorImageProvider; }

class StatesEditorWidget : public QQuickWidget
{
    Q_OBJECT

public:
    StatesEditorWidget(StatesEditorView *m_statesEditorView, StatesEditorModel *statesEditorModel);
    ~StatesEditorWidget() override;

    int currentStateInternalId() const;
    void setCurrentStateInternalId(int internalId);
    void setNodeInstanceView(const NodeInstanceView *nodeInstanceView);

    void showAddNewStatesButton(bool showAddNewStatesButton);

    static QString qmlSourcesPath();

protected:
    void showEvent(QShowEvent *) override;
    void focusOutEvent(QFocusEvent *focusEvent) override;
    void focusInEvent(QFocusEvent *focusEvent) override;

private:
    void reloadQmlSource();

private:
    QPointer<StatesEditorView> m_statesEditorView;
    Internal::StatesEditorImageProvider *m_imageProvider;
    QShortcut *m_qmlSourceUpdateShortcut;
    QElapsedTimer m_usageTimer;
};

}
