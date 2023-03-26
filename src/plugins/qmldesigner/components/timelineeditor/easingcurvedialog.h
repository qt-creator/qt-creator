// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    void initialize(const PropertyName &propName, const QString &curveString);

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

    PropertyName m_easingCurveProperty;
};

} // namespace QmlDesigner
