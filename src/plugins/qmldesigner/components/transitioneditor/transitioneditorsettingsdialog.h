// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmltimeline.h>

#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QSpinBox)

namespace QmlDesigner {

class TransitionForm;
class TransitionEditorView;

namespace Ui {
class TransitionEditorSettingsDialog;
}

class TransitionEditorSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransitionEditorSettingsDialog(QWidget *parent, class TransitionEditorView *view);
    void setCurrentTransition(const ModelNode &timeline);
    ~TransitionEditorSettingsDialog() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupTransitions(const ModelNode &node);

    void addTransitionTab(const QmlTimeline &node);

    Ui::TransitionEditorSettingsDialog *ui;

    TransitionEditorView *m_transitionEditorView;
    ModelNode m_currentTransition;
};

} // namespace QmlDesigner
