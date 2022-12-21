// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>
#include <QToolBar>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QObject)
QT_FORWARD_DECLARE_CLASS(QResizeEvent)
QT_FORWARD_DECLARE_CLASS(QSlider)
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace QmlDesigner {

class TimelineWidget;

class QmlTimeline;

class TransitionEditorToolBar : public QToolBar
{
    Q_OBJECT

signals:
    void settingDialogClicked();

    void scaleFactorChanged(int value);
    void durationChanged(int value);
    void currentTransitionChanged(const QString &name);
    void openEasingCurveEditor();

public:
    explicit TransitionEditorToolBar(QWidget *parent = nullptr);

    void reset();

    int scaleFactor() const;
    QString currentTransitionId() const;

    void setBlockReflection(bool block);
    void setCurrentTransition(const ModelNode &transition);
    void setDuration(qreal frame);
    void setScaleFactor(int factor);

    void setActionEnabled(const QString &name, bool enabled);

    void updateComboBox(const ModelNode &root);
protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void createLeftControls();
    void createCenterControls();
    void createRightControls();
    void addSpacing(int width);

    QList<QObject *> m_grp;

    QComboBox *m_transitionComboBox = nullptr;
    QSlider *m_scale = nullptr;
    QLineEdit *m_duration = nullptr;
};

} // namespace QmlDesigner
