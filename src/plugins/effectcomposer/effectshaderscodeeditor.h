// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "effectcodeeditorwidget.h"
#include "shadereditordata.h"

#include <texteditor/texteditor.h>

#include <utils/uniqueobjectptr.h>

QT_FORWARD_DECLARE_CLASS(QSettings)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QSplitter)

class StudioQuickWidget;

namespace EffectComposer {

class EffectComposerModel;
class EffectComposerUniformsModel;
class EffectComposerUniformsTableModel;
class EffectComposerEditableNodesModel;
class EffectDocument;

class EffectShadersCodeEditor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool liveUpdate READ liveUpdate WRITE setLiveUpdate NOTIFY liveUpdateChanged)

    Q_PROPERTY(QString selectedShader MEMBER m_selectedShaderName WRITE selectShader NOTIFY
                   selectedShaderChanged)

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

    void selectShader(const QString &shaderName);

    const ShaderEditorData *currentEditorData() const;

    ShaderEditorData *createEditorData(
        const QString &fragmentDocument, const QString &vertexDocument);

    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void insertTextToCursorPosition(const QString &text);
    Q_INVOKABLE void switchToNodeIndex(int index);

signals:
    void liveUpdateChanged(bool);
    void rebakeRequested();
    void openedChanged(bool);
    void selectedShaderChanged(const QString &);
    void requestToOpenNode(int nodeIndex);

protected:
    using QWidget::show;
    EffectCodeEditorWidget *createJSEditor();
    void setupUIComponents();
    void setOpened(bool value);

    void closeEvent(QCloseEvent *event) override;

private:
    void writeLiveUpdateSettings();
    void readAndApplyLiveUpdateSettings();
    void writeGeometrySettings();
    void readAndApplyGeometrySettings();
    void createHeader();
    void createQmlTabs();
    void createQmlFooter();
    void loadQml();
    void setUniformsModel(EffectComposerUniformsTableModel *uniforms);
    void selectNonEmptyShader(ShaderEditorData *data);
    void setUniformCallbacksOnEditors(ShaderEditorData *data);
    void setSelectedShaderName(const QString &shaderName);
    void onEditorWidgetChanged();
    void onOpenStateChanged();

    EffectCodeEditorWidget *currentEditor() const;

    QSettings *m_settings = nullptr;
    QPointer<StudioQuickWidget> m_headerWidget;
    QPointer<StudioQuickWidget> m_qmlTabWidget;
    QPointer<StudioQuickWidget> m_qmlFooterWidget;
    QPointer<QStackedWidget> m_stackedWidget;
    QPointer<QSplitter> m_splitter;
    QPointer<EffectComposerUniformsTableModel> m_defaultTableModel;
    QPointer<EffectComposerEditableNodesModel> m_editableNodesModel;
    ShaderEditorData *m_currentEditorData = nullptr;

    bool m_liveUpdate = false;
    bool m_opened = false;
    QString m_selectedShaderName;
};

} // namespace EffectComposer
