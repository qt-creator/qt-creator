// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineeditor/timelineutils.h"

#include <coreplugin/icontext.h>

#include <utils/transientscroll.h>

#include <QWidget>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGraphicsView)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QResizeEvent)
QT_FORWARD_DECLARE_CLASS(QShowEvent)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QScrollBar)

namespace QmlDesigner {

class TransitionEditorView;
class TransitionEditorToolBar;
class TransitionEditorGraphicsScene;
class ModelNode;

class TransitionContext : public Core::IContext
{
    Q_OBJECT

public:
    explicit TransitionContext(QWidget *widget);
    void contextHelp(const HelpCallback &callback) const override;
};

class TransitionEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TransitionEditorWidget(TransitionEditorView *view);
    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    TransitionEditorGraphicsScene *graphicsScene() const;
    TransitionEditorView *transitionEditorView() const;
    TransitionEditorToolBar *toolBar() const;

    void init(int zoom = 0);
    void reset();

    void setupScrollbar(int min, int max, int current);
    void changeScaleFactor(int factor);

    void updateData(const ModelNode &transition);
public slots:
    void selectionChanged();
    void scroll(const TimelineUtils::Side &side);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void connectToolbar();
    void setTransitionActive(bool b);
    void openEasingCurveEditor();

    TransitionEditorToolBar *m_toolbar = nullptr;

    QGraphicsView *m_rulerView = nullptr;

    QGraphicsView *m_graphicsView = nullptr;

    Utils::ScrollBar *m_scrollbar = nullptr;

    QLabel *m_statusBar = nullptr;

    TransitionEditorView *m_transitionEditorView = nullptr;

    TransitionEditorGraphicsScene *m_graphicsScene;

    QPushButton *m_addButton = nullptr;

    QWidget *m_onboardingContainer = nullptr;
};

} // namespace QmlDesigner
