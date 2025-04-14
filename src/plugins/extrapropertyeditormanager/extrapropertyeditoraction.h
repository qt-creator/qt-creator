// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAction>
#include <QHash>

namespace QmlDesigner {

class PropertyEditorView;

class ExtraPropertyEditorAction : public QAction
{
    Q_OBJECT

public:
    explicit ExtraPropertyEditorAction(QObject *parent);
    void registerView(PropertyEditorView *view);
    void unregisterView(PropertyEditorView *view);

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void increaseOne();
    void decreaseOne();
    void checkAll(bool value);
    void uncheckIfAllWidgetsHidden();
    void setCheckedIfWidgetRegistered(QObject *widgetObject);
    void ensureParentId(PropertyEditorView *view);
    void showExtraWidget(PropertyEditorView *view);
    void closeExtraWidget(PropertyEditorView *view);

    QHash<PropertyEditorView *, bool> m_viewsStatus;
    QList<PropertyEditorView *> m_views;

    int m_count = 0;
};

} // namespace QmlDesigner
