// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "effectcodeeditorwidget.h"

#include <texteditor/texteditor.h>

#include <utils/uniqueobjectptr.h>

QT_FORWARD_DECLARE_CLASS(QSettings)
QT_FORWARD_DECLARE_CLASS(QTabWidget)

class StudioQuickWidget;

namespace EffectComposer {

class EffectComposerModel;
class EffectComposerUniformsModel;
class EffectComposerUniformsTableModel;
class EffectComposerEditableNodesModel;
class EffectDocument;

struct ShaderEditorData
{
    EffectComposerUniformsTableModel *tableModel = nullptr;

    TextEditor::TextDocumentPtr fragmentDocument;
    TextEditor::TextDocumentPtr vertexDocument;

private:
    friend class EffectShadersCodeEditor;
    Utils::UniqueObjectLatePtr<EffectCodeEditorWidget> fragmentEditor;
    Utils::UniqueObjectLatePtr<EffectCodeEditorWidget> vertexEditor;

    ShaderEditorData() = default;
};

class EffectShadersCodeEditor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool liveUpdate READ liveUpdate WRITE setLiveUpdate NOTIFY liveUpdateChanged)

public:
    EffectShadersCodeEditor(const QString &title = tr("Untitled Editor"), QWidget *parent = nullptr);
    ~EffectShadersCodeEditor() override;

    void showWidget();
    void showWidget(int x, int y);

    bool liveUpdate() const;
    void setLiveUpdate(bool liveUpdate);

    bool isOpened() const;

    void setCompositionsModel(EffectComposerModel *compositionsModel);

    void setupShader(ShaderEditorData *data);
    void cleanFromData(ShaderEditorData *data);

    ShaderEditorData *createEditorData(
        const QString &fragmentDocument,
        const QString &vertexDocument,
        EffectComposerUniformsModel *uniforms);

    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void insertTextToCursorPosition(const QString &text);

    static EffectShadersCodeEditor *instance();

signals:
    void liveUpdateChanged(bool);
    void rebakeRequested();
    void openedChanged(bool);

protected:
    using QWidget::show;
    EffectCodeEditorWidget *createJSEditor();
    void setupUIComponents();
    void setOpened(bool value);

    void closeEvent(QCloseEvent *event) override;

private:
    void writeLiveUpdateSettings();
    void readAndApplyLiveUpdateSettings();
    void createHeader();
    void loadQml();
    void setUniformsModel(EffectComposerUniformsTableModel *uniforms);
    void selectNonEmptyShader(ShaderEditorData *data);

    EffectCodeEditorWidget *currentEditor() const;

    QSettings *m_settings = nullptr;
    QPointer<StudioQuickWidget> m_headerWidget;
    QPointer<QTabWidget> m_tabWidget;
    QPointer<EffectComposerUniformsTableModel> m_defaultTableModel;
    QPointer<EffectComposerEditableNodesModel> m_editableNodesModel;
    ShaderEditorData *m_currentEditorData = nullptr;

    bool m_liveUpdate = false;
    bool m_opened = false;
};

} // namespace EffectComposer
