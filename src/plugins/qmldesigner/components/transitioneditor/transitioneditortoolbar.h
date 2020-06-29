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

#include "animationcurvedialog.h"
#include "animationcurveeditormodel.h"

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

    bool m_blockReflection = false;
};

} // namespace QmlDesigner
