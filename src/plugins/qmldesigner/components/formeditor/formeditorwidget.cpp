/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "designeractionmanager.h"
#include "formeditorwidget.h"
#include "formeditorscene.h"
#include "qmldesignerplugin.h"
#include "designersettings.h"
#include "qmldesignerconstants.h"
#include "qmldesignericons.h"
#include "viewmanager.h"
#include <model.h>
#include <theme.h>

#include <backgroundaction.h>
#include <formeditorgraphicsview.h>
#include <formeditorscene.h>
#include <formeditorview.h>
#include <lineeditaction.h>
#include <zoomaction.h>
#include <toolbox.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/utilsicons.h>

#include <QActionGroup>
#include <QFileDialog>
#include <QPainter>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace QmlDesigner {

FormEditorWidget::FormEditorWidget(FormEditorView *view) :
    m_formEditorView(view)
{
    Core::Context context(Constants::C_QMLFORMEDITOR);
    m_context = new Core::IContext(this);
    m_context->setContext(context);
    m_context->setWidget(this);

    auto fillLayout = new QVBoxLayout(this);
    fillLayout->setContentsMargins(0, 0, 0, 0);
    fillLayout->setSpacing(0);
    setLayout(fillLayout);

    QList<QAction*> upperActions;

    m_toolActionGroup = new QActionGroup(this);

    auto layoutActionGroup = new QActionGroup(this);
    layoutActionGroup->setExclusive(true);

    m_noSnappingAction = layoutActionGroup->addAction(tr("No snapping."));
    m_noSnappingAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_noSnappingAction->setCheckable(true);
    m_noSnappingAction->setChecked(true);
    m_noSnappingAction->setIcon(Icons::NO_SNAPPING.icon());
    registerActionAsCommand(m_noSnappingAction, Constants::FORMEDITOR_NO_SNAPPING, QKeySequence(Qt::Key_T));

    m_snappingAndAnchoringAction = layoutActionGroup->addAction(tr("Snap to parent or sibling items and generate anchors."));
    m_snappingAndAnchoringAction->setCheckable(true);
    m_snappingAndAnchoringAction->setChecked(true);
    m_snappingAndAnchoringAction->setIcon(Icons::NO_SNAPPING_AND_ANCHORING.icon());
    registerActionAsCommand(m_snappingAndAnchoringAction, Constants::FORMEDITOR_NO_SNAPPING_AND_ANCHORING, QKeySequence(Qt::Key_W));

    m_snappingAction = layoutActionGroup->addAction(tr("Snap to parent or sibling items but do not generate anchors."));
    m_snappingAction->setCheckable(true);
    m_snappingAction->setChecked(true);
    m_snappingAction->setIcon(Icons::SNAPPING.icon());
    registerActionAsCommand(m_snappingAction, Constants::FORMEDITOR_SNAPPING, QKeySequence(Qt::Key_E));

    addActions(layoutActionGroup->actions());
    upperActions.append(layoutActionGroup->actions());

    auto separatorAction = new QAction(this);
    separatorAction->setSeparator(true);
    addAction(separatorAction);
    upperActions.append(separatorAction);

    m_showBoundingRectAction = new QAction(Utils::Icons::BOUNDING_RECT.icon(),
                                           tr("Show bounding rectangles and stripes for empty items."),
                                           this);
    m_showBoundingRectAction->setCheckable(true);
    m_showBoundingRectAction->setChecked(false);
    registerActionAsCommand(m_showBoundingRectAction, Constants::FORMEDITOR_NO_SHOW_BOUNDING_RECTANGLE, QKeySequence(Qt::Key_A));

    addAction(m_showBoundingRectAction.data());
    upperActions.append(m_showBoundingRectAction.data());

    separatorAction = new QAction(this);
    separatorAction->setSeparator(true);
    addAction(separatorAction);
    upperActions.append(separatorAction);

    m_rootWidthAction = new LineEditAction(tr("Override Width"), this);
    m_rootWidthAction->setToolTip(tr("Override width of root item."));
    connect(m_rootWidthAction.data(), &LineEditAction::textChanged,
            this, &FormEditorWidget::changeRootItemWidth);
    addAction(m_rootWidthAction.data());
    upperActions.append(m_rootWidthAction.data());

    m_rootHeightAction =  new LineEditAction(tr("Override Height"), this);
    m_rootHeightAction->setToolTip(tr("Override height of root item."));
    connect(m_rootHeightAction.data(), &LineEditAction::textChanged,
            this, &FormEditorWidget::changeRootItemHeight);
    addAction(m_rootHeightAction.data());
    upperActions.append(m_rootHeightAction.data());

    m_toolBox = new ToolBox(this);
    fillLayout->addWidget(m_toolBox.data());

    m_toolBox->setLeftSideActions(upperActions);

    m_backgroundAction = new BackgroundAction(m_toolActionGroup.data());
    connect(m_backgroundAction.data(), &BackgroundAction::backgroundChanged, this, &FormEditorWidget::changeBackgound);
    addAction(m_backgroundAction.data());
    upperActions.append(m_backgroundAction.data());
    m_toolBox->addRightSideAction(m_backgroundAction.data());

    m_zoomAction = new ZoomAction(m_toolActionGroup.data());
    connect(m_zoomAction.data(), &ZoomAction::zoomLevelChanged,
            this, &FormEditorWidget::setZoomLevel);
    addAction(m_zoomAction.data());
    upperActions.append(m_zoomAction.data());
    m_toolBox->addRightSideAction(m_zoomAction.data());

    m_resetAction = new QAction(Utils::Icons::RESET_TOOLBAR.icon(), tr("Reset View"), this);
    registerActionAsCommand(m_resetAction, Constants::FORMEDITOR_REFRESH, QKeySequence(Qt::Key_R));

    addAction(m_resetAction.data());
    upperActions.append(m_resetAction.data());
    m_toolBox->addRightSideAction(m_resetAction.data());

    m_graphicsView = new FormEditorGraphicsView(this);

    fillLayout->addWidget(m_graphicsView.data());

    QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css");
    sheet += Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css");
    setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(sheet)));
}

void FormEditorWidget::changeTransformTool(bool checked)
{
    if (checked)
        m_formEditorView->changeToTransformTools();
}

void FormEditorWidget::changeRootItemWidth(const QString &widthText)
{
    bool canConvert;
    int width = widthText.toInt(&canConvert);
    if (canConvert)
        m_formEditorView->rootModelNode().setAuxiliaryData("width", width);
    else
        m_formEditorView->rootModelNode().setAuxiliaryData("width", QVariant());
}

void FormEditorWidget::changeRootItemHeight(const QString &heighText)
{
    bool canConvert;
    int height = heighText.toInt(&canConvert);
    if (canConvert)
        m_formEditorView->rootModelNode().setAuxiliaryData("height", height);
    else
        m_formEditorView->rootModelNode().setAuxiliaryData("height", QVariant());
}

void FormEditorWidget::changeBackgound(const QColor &color)
{
    if (color.alpha() == 0) {
        m_graphicsView->activateCheckboardBackground();
        if (m_formEditorView->rootModelNode().hasAuxiliaryData("formeditorColor"))
            m_formEditorView->rootModelNode().setAuxiliaryData("formeditorColor", {});
    } else {
        m_graphicsView->activateColoredBackground(color);
        m_formEditorView->rootModelNode().setAuxiliaryData("formeditorColor", color);
    }
}

void FormEditorWidget::registerActionAsCommand(QAction *action, Core::Id id, const QKeySequence &keysequence)
{
    Core::Context context(Constants::C_QMLFORMEDITOR);

    Core::Command *command = Core::ActionManager::registerAction(action, id, context);
    command->setDefaultKeySequence(keysequence);
    command->augmentActionWithShortcutToolTip(action);
}

void FormEditorWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (event->angleDelta().y() > 0)
            zoomAction()->zoomOut();
        else
            zoomAction()->zoomIn();

        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }

}

void FormEditorWidget::updateActions()
{
    if (m_formEditorView->model() && m_formEditorView->rootModelNode().isValid()) {
        if (m_formEditorView->rootModelNode().hasAuxiliaryData("width") && m_formEditorView->rootModelNode().auxiliaryData("width").isValid())
            m_rootWidthAction->setLineEditText(m_formEditorView->rootModelNode().auxiliaryData("width").toString());
        else
            m_rootWidthAction->clearLineEditText();
        if (m_formEditorView->rootModelNode().hasAuxiliaryData("height") && m_formEditorView->rootModelNode().auxiliaryData("height").isValid())
            m_rootHeightAction->setLineEditText(m_formEditorView->rootModelNode().auxiliaryData("height").toString());
        else
            m_rootHeightAction->clearLineEditText();

        if (m_formEditorView->rootModelNode().hasAuxiliaryData("formeditorColor"))
            m_backgroundAction->setColor(m_formEditorView->rootModelNode().auxiliaryData("formeditorColor").value<QColor>());
        else
            m_backgroundAction->setColor(Qt::transparent);

        if (m_formEditorView->rootModelNode().hasAuxiliaryData("formeditorZoom"))
            m_zoomAction->setZoomLevel(m_formEditorView->rootModelNode().auxiliaryData("formeditorZoom").toDouble());
        else
            m_zoomAction->setZoomLevel(1.0);

    } else {
        m_rootWidthAction->clearLineEditText();
        m_rootHeightAction->clearLineEditText();
    }
}


void FormEditorWidget::resetView()
{
    setRootItemRect(QRectF());
}

void FormEditorWidget::centerScene()
{
    m_graphicsView->centerOn(rootItemRect().center());
}

void FormEditorWidget::setFocus()
{
    m_graphicsView->setFocus(Qt::OtherFocusReason);
}

void FormEditorWidget::showErrorMessageBox(const QList<DocumentMessage> &errors)
{
    errorWidget()->setErrors(errors);
    errorWidget()->setVisible(true);
    m_graphicsView->setDisabled(true);
    m_toolBox->setDisabled(true);
}

void FormEditorWidget::hideErrorMessageBox()
{
    if (!m_documentErrorWidget.isNull())
        errorWidget()->setVisible(false);

    m_graphicsView->setDisabled(false);
    m_toolBox->setDisabled(false);
}

void FormEditorWidget::showWarningMessageBox(const QList<DocumentMessage> &warnings)
{
      if (!errorWidget()->warningsEnabled())
          return;

    errorWidget()->setWarnings(warnings);
    errorWidget()->setVisible(true);
}

ZoomAction *FormEditorWidget::zoomAction() const
{
    return m_zoomAction.data();
}

QAction *FormEditorWidget::resetAction() const
{
    return m_resetAction.data();
}

QAction *FormEditorWidget::showBoundingRectAction() const
{
    return m_showBoundingRectAction.data();
}

QAction *FormEditorWidget::snappingAction() const
{
    return m_snappingAction.data();
}

QAction *FormEditorWidget::snappingAndAnchoringAction() const
{
    return m_snappingAndAnchoringAction.data();
}

void FormEditorWidget::setZoomLevel(double zoomLevel)
{
    m_graphicsView->resetTransform();

    m_graphicsView->scale(zoomLevel, zoomLevel);

    if (zoomLevel == 1.0) {
        if (m_formEditorView->rootModelNode().hasAuxiliaryData("formeditorZoom"))
            m_formEditorView->rootModelNode().setAuxiliaryData("formeditorZoom", {});
    } else {
        m_formEditorView->rootModelNode().setAuxiliaryData("formeditorZoom", zoomLevel);
    }
}

void FormEditorWidget::setScene(FormEditorScene *scene)
{
    m_graphicsView->setScene(scene);
}

QActionGroup *FormEditorWidget::toolActionGroup() const
{
    return m_toolActionGroup.data();
}

ToolBox *FormEditorWidget::toolBox() const
{
     return m_toolBox.data();
}

double FormEditorWidget::spacing() const
{
    return DesignerSettings::getValue(DesignerSettingsKey::ITEMSPACING).toDouble();
}

double FormEditorWidget::containerPadding() const
{
    return DesignerSettings::getValue(DesignerSettingsKey::CONTAINERPADDING).toDouble();
}

void FormEditorWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (m_formEditorView)
        m_formEditorView->contextHelp(callback);
    else
        callback({});
}

void FormEditorWidget::setRootItemRect(const QRectF &rect)
{
    m_graphicsView->setRootItemRect(rect);
}

QRectF FormEditorWidget::rootItemRect() const
{
    return m_graphicsView->rootItemRect();
}

void FormEditorWidget::exportAsImage(const QRectF &boundingRect)
{
    QString proposedFileName = m_formEditorView->model()->fileUrl().toLocalFile();
    proposedFileName.chop(4);
    if (proposedFileName.endsWith(".ui"))
        proposedFileName.chop(3);
    proposedFileName.append(".png");
    const QString fileName = QFileDialog::getSaveFileName(Core::ICore::dialogParent(),
                                                          tr("Export Current QML File as Image"),
                                                          proposedFileName,
                                                          tr("PNG (*.png);;JPG (*.jpg)"));

    if (!fileName.isNull()) {
        QImage image(boundingRect.size().toSize(), QImage::Format_ARGB32);
        QPainter painter(&image);
        QTransform viewportTransform = m_graphicsView->viewportTransform();
        m_graphicsView->render(&painter,
                               QRectF(0, 0, image.width(), image.height()),
                               viewportTransform.mapRect(boundingRect).toRect());
        image.save(fileName);
    }
}

FormEditorGraphicsView *FormEditorWidget::graphicsView() const
{
    return m_graphicsView;
}

DocumentWarningWidget *FormEditorWidget::errorWidget()
{
    if (m_documentErrorWidget.isNull()) {
        m_documentErrorWidget = new DocumentWarningWidget(this);
        connect(m_documentErrorWidget.data(), &DocumentWarningWidget::gotoCodeClicked, [=]
            (const QString &, int codeLine, int codeColumn) {
            m_formEditorView->gotoError(codeLine, codeColumn);
        });
    }
    return m_documentErrorWidget;
}


}


