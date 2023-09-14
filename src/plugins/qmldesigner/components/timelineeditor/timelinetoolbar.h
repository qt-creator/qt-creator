// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QToolBar>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QObject)
QT_FORWARD_DECLARE_CLASS(QResizeEvent)
QT_FORWARD_DECLARE_CLASS(QSlider)
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace QmlDesigner {

class TimelineWidget;

class QmlTimeline;

class TimelineToolBar : public QToolBar
{
    Q_OBJECT

signals:
    void settingDialogClicked();
    void curveEditorDialogClicked();

    void openEasingCurveEditor();

    void playTriggered();
    void previousFrameTriggered();
    void nextFrameTriggered();
    void toFirstFrameTriggered();
    void toLastFrameTriggered();

    void recordToggled(bool val);
    void loopPlaybackToggled(bool val);
    void playbackSpeedChanged(float val);

    void scaleFactorChanged(int value);
    void startFrameChanged(int value);
    void currentFrameChanged(int value);
    void endFrameChanged(int value);

public:
    explicit TimelineToolBar(QWidget *parent = nullptr);

    void reset();

    bool recording() const;
    int scaleFactor() const;
    QString currentTimelineId() const;

    void setCurrentState(const QString &name);
    void setBlockReflection(bool block);
    void setCurrentTimeline(const QmlTimeline &timeline);
    void setStartFrame(qreal frame);
    void setCurrentFrame(qreal frame);
    void setEndFrame(qreal frame);
    void setScaleFactor(int factor);
    void setPlayState(bool state);
    void setIsMcu(bool isMcu);

    void setActionEnabled(const QString &name, bool enabled);
    void removeTimeline(const QmlTimeline &timeline);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void createLeftControls();
    void createCenterControls();
    void createRightControls();
    void addSpacing(int width);
    void setupCurrentFrameValidator();

    QList<QObject *> m_grp;

    QLabel *m_timelineLabel = nullptr;
    QLabel *m_stateLabel = nullptr;
    QSlider *m_scale = nullptr;
    QLineEdit *m_firstFrame = nullptr;
    QLineEdit *m_currentFrame = nullptr;
    QLineEdit *m_lastFrame = nullptr;
    QLineEdit *m_animationPlaybackSpeed = nullptr;

    QAction *m_playing = nullptr;
    QAction *m_recording = nullptr;
    QAction *m_curvePicker = nullptr;
    bool m_blockReflection = false;

    static constexpr const char* m_curveEditorName = QT_TR_NOOP("Easing Curve Editor");
};

} // namespace QmlDesigner
