// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSpinBox>
#include <QSlider>
#include <QToolBar>
#include <QValidator>
#include <QWidget>

#include "keyframe.h"

namespace QmlDesigner {

class CurveEditorModel;

class ValidatableSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    ValidatableSpinBox(std::function<bool(int)> validator, QWidget* parent=nullptr);

protected:
    QValidator::State validate(QString &text, int &pos) const override;

private:
    std::function<bool(int)> m_validator;
};


class CurveEditorToolBar : public QToolBar
{
    Q_OBJECT

signals:
    void unifyClicked();

    void interpolationClicked(Keyframe::Interpolation interpol);

    void startFrameChanged(int start);

    void endFrameChanged(int end);

    void currentFrameChanged(int current);

    void zoomChanged(double zoom);

public:
    CurveEditorToolBar(CurveEditorModel *model, QWidget* parent = nullptr);

    void setIsMcuProject(bool isMcu);

    void setZoom(double zoom);

    void setCurrentFrame(int current, bool notify);

    void updateBoundsSilent(int start, int end);

private:
    void addSpacer();
    void addSpace(int spaceSize = 32);
    void addSpace(QLayout *layout, int spaceSize = 32);

    ValidatableSpinBox *m_startSpin;
    ValidatableSpinBox *m_endSpin;
    QSpinBox *m_currentSpin;
    QSlider *m_zoomSlider;
    QAction *m_stepAction;
    QAction *m_splineAction;
    QAction *m_unifyAction;

    static constexpr const char* m_stepLabel = QT_TR_NOOP("Step");
    static constexpr const char* m_splineLabel = QT_TR_NOOP("Spline");
    static constexpr const char* m_unifyLabel = QT_TR_NOOP("Unify");
};

} // End namespace QmlDesigner.
