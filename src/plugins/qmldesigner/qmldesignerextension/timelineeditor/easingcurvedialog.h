/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QDialog>
#include <QDialogButtonBox>

#include <modelnode.h>

QT_BEGIN_NAMESPACE
class QLabel;
class QPlainTextEdit;
class QHBoxLayout;
QT_END_NAMESPACE

namespace QmlDesigner {

class SplineEditor;
class PresetEditor;
class EasingCurve;

class EasingCurveDialog : public QDialog
{
    Q_OBJECT

public:
    EasingCurveDialog(const QList<ModelNode> &frames, QWidget *parent = nullptr);

    void initialize(const QString &curveString);

    static void runDialog(const QList<ModelNode> &frames, QWidget *parent = nullptr);

private:
    bool apply();

    void textChanged();

    void tabClicked(int id);

    void presetTabClicked(int id);

    void buttonsClicked(QDialogButtonBox::StandardButton button);

    void updateEasingCurve(const EasingCurve &curve);

private:
    SplineEditor *m_splineEditor = nullptr;

    QPlainTextEdit *m_text = nullptr;

    PresetEditor *m_presets = nullptr;

    QHBoxLayout *m_durationLayout = nullptr;

    QDialogButtonBox *m_buttons = nullptr;

    QLabel *m_label = nullptr;

    QList<ModelNode> m_frames;
};

} // namespace QmlDesigner
