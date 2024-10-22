// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

QT_FORWARD_DECLARE_CLASS(QSettings)
QT_FORWARD_DECLARE_CLASS(QToolBar)

namespace EffectComposer {

class EffectCodeEditorWidget;

class EffectShadersCodeEditor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool liveUpdate READ liveUpdate WRITE setLiveUpdate NOTIFY liveUpdateChanged)

public:
    EffectShadersCodeEditor(const QString &title = tr("Untitled Editor"), QWidget *parent = nullptr);
    ~EffectShadersCodeEditor() override;

    void showWidget();
    void showWidget(int x, int y);

    QString fragmentValue() const;
    void setFragmentValue(const QString &text);

    QString vertexValue() const;
    void setVertexValue(const QString &text);

    bool liveUpdate() const;
    void setLiveUpdate(bool liveUpdate);

signals:
    void liveUpdateChanged(bool);
    void fragmentValueChanged();
    void vertexValueChanged();
    void rebakeRequested();

protected:
    using QWidget::show;
    EffectCodeEditorWidget *createJSEditor();
    void setupUIComponents();

    void closeEvent(QCloseEvent *event) override;

private:
    void writeLiveUpdateSettings();
    void readAndApplyLiveUpdateSettings();
    QToolBar *createToolbar();

    QSettings *m_settings = nullptr;
    QPointer<EffectCodeEditorWidget> m_fragmentEditor;
    QPointer<EffectCodeEditorWidget> m_vertexEditor;

    bool m_liveUpdate = false;
};

} // namespace EffectComposer
