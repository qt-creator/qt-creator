/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "timelineeditor/timelineutils.h"

#include <coreplugin/icontext.h>

#include <QWidget>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGraphicsView)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QResizeEvent)
QT_FORWARD_DECLARE_CLASS(QScrollBar)
QT_FORWARD_DECLARE_CLASS(QShowEvent)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace QmlDesigner {

class TransitionEditorView;
class TransitionEditorToolBar;
class TransitionEditorGraphicsScene;
class ModelNode;

class TransitionEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TransitionEditorWidget(TransitionEditorView *view);
    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    TransitionEditorGraphicsScene *graphicsScene() const;
    TransitionEditorView *transitionEditorView() const;
    TransitionEditorToolBar *toolBar() const;

    void init();
    void reset();

    void setupScrollbar(int min, int max, int current);
    void changeScaleFactor(int factor);

    void updateData(const ModelNode &transition);
public slots:
    void selectionChanged();
    void scroll(const TimelineUtils::Side &side);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void connectToolbar();
    void setTransitionActive(bool b);
    void openEasingCurveEditor();

    TransitionEditorToolBar *m_toolbar = nullptr;

    QGraphicsView *m_rulerView = nullptr;

    QGraphicsView *m_graphicsView = nullptr;

    QScrollBar *m_scrollbar = nullptr;

    QLabel *m_statusBar = nullptr;

    TransitionEditorView *m_transitionEditorView = nullptr;

    TransitionEditorGraphicsScene *m_graphicsScene;

    QPushButton *m_addButton = nullptr;

    QWidget *m_onboardingContainer = nullptr;
};

} // namespace QmlDesigner
