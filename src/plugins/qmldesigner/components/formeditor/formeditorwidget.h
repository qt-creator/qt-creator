// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <documentwarningwidget.h>

#include <coreplugin/icontext.h>

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QActionGroup;
QT_END_NAMESPACE

namespace QmlDesigner {

class ZoomAction;
class LineEditAction;
class BackgroundAction;
class FormEditorView;
class FormEditorScene;
class FormEditorGraphicsView;
class ToolBox;
class QmlItemNode;

class FormEditorWidget : public QWidget
{
    Q_OBJECT

public:
    FormEditorWidget(FormEditorView *view);

    ZoomAction *zoomAction() const;
    QAction *zoomSelectionAction() const;
    QAction *showBoundingRectAction() const;
    QAction *snappingAction() const;
    QAction *snappingAndAnchoringAction() const;
    QAction *resetAction() const;

    void setScene(FormEditorScene *scene);
    ToolBox *toolBox() const;

    double spacing() const;
    double containerPadding() const;

    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    void setRootItemRect(const QRectF &rect);
    QRectF rootItemRect() const;

    void initialize();
    void updateActions();

    void resetView();
    void centerScene();

    void setFocus();

    void showErrorMessageBox(const QList<DocumentMessage> &errors);
    void hideErrorMessageBox();

    void showWarningMessageBox(const QList<DocumentMessage> &warnings);

    void exportAsImage(const QRectF &boundingRect);

    QImage takeFormEditorScreenshot();
    QPicture renderToPicture() const;

    FormEditorGraphicsView *graphicsView() const;

    bool errorMessageBoxIsVisible() const;

    void setBackgoundImage(const QImage &image);
    QImage backgroundImage() const;

protected:
    QActionGroup *toolActionGroup() const;
    DocumentWarningWidget *errorWidget();
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *dragEnterEvent) override;
    void dropEvent(QDropEvent *dropEvent) override;

private:
    void changeTransformTool(bool checked);
    void changeRootItemWidth(const QString &widthText);
    void changeRootItemHeight(const QString &heightText);
    void changeBackgound(const QColor &color);
    void registerActionAsCommand(QAction *action,
                                 Utils::Id id,
                                 const QKeySequence &keysequence,
                                 const QByteArray &category = {},
                                 int priority = 0);

    QPointer<FormEditorView> m_formEditorView;
    QPointer<FormEditorGraphicsView> m_graphicsView;
    QPointer<ZoomAction> m_zoomAction;
    QPointer<ToolBox> m_toolBox;
    QPointer<QAction> m_transformToolAction;
    QPointer<QActionGroup> m_toolActionGroup;
    QPointer<QAction> m_snappingAction;
    QPointer<QAction> m_snappingAndAnchoringAction;
    QPointer<QAction> m_noSnappingAction;
    QPointer<QAction> m_showBoundingRectAction;
    QPointer<LineEditAction> m_rootWidthAction;
    QPointer<LineEditAction> m_rootHeightAction;
    QPointer<BackgroundAction> m_backgroundAction;
    QPointer<QAction> m_resetAction;
    QPointer<QAction> m_zoomAllAction;
    QPointer<QAction> m_zoomSelectionAction;
    QPointer<QAction> m_zoomInAction;
    QPointer<QAction> m_zoomOutAction;
    QPointer<DocumentWarningWidget> m_documentErrorWidget;
    Core::IContext *m_context = nullptr;
};

} // namespace QmlDesigner
