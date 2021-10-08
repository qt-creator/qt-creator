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

#include "formeditorwidget.h"
#include "designeractionmanager.h"
#include "designersettings.h"
#include "formeditoritem.h"
#include "formeditorscene.h"
#include "qmldesignerconstants.h"
#include "qmldesignericons.h"
#include "qmldesignerplugin.h"
#include "viewmanager.h"
#include <model.h>
#include <theme.h>

#include <backgroundaction.h>
#include <formeditorgraphicsview.h>
#include <formeditorscene.h>
#include <formeditorview.h>
#include <lineeditaction.h>
#include <toolbox.h>
#include <zoomaction.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QActionGroup>
#include <QFileDialog>
#include <QMimeData>
#include <QPainter>
#include <QPicture>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace QmlDesigner {

FormEditorWidget::FormEditorWidget(FormEditorView *view)
    : m_formEditorView(view)
{
    setAcceptDrops(true);

    Core::Context context(Constants::C_QMLFORMEDITOR);
    m_context = new Core::IContext(this);
    m_context->setContext(context);
    m_context->setWidget(this);

    auto fillLayout = new QVBoxLayout(this);
    fillLayout->setContentsMargins(0, 0, 0, 0);
    fillLayout->setSpacing(0);
    setLayout(fillLayout);

    QList<QAction *> upperActions;

    m_toolActionGroup = new QActionGroup(this);

    auto layoutActionGroup = new QActionGroup(this);
    layoutActionGroup->setExclusive(true);

    m_noSnappingAction = layoutActionGroup->addAction(tr("No snapping."));
    m_noSnappingAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_noSnappingAction->setCheckable(true);
    m_noSnappingAction->setChecked(true);
    m_noSnappingAction->setIcon(Icons::NO_SNAPPING.icon());
    registerActionAsCommand(m_noSnappingAction, Constants::FORMEDITOR_NO_SNAPPING, QKeySequence(Qt::Key_T));

    m_snappingAndAnchoringAction = layoutActionGroup->addAction(tr("Snap to parent or sibling components and generate anchors."));
    m_snappingAndAnchoringAction->setCheckable(true);
    m_snappingAndAnchoringAction->setChecked(true);
    m_snappingAndAnchoringAction->setIcon(Icons::NO_SNAPPING_AND_ANCHORING.icon());
    registerActionAsCommand(m_snappingAndAnchoringAction, Constants::FORMEDITOR_NO_SNAPPING_AND_ANCHORING, QKeySequence(Qt::Key_W));

    m_snappingAction = layoutActionGroup->addAction(tr("Snap to parent or sibling components but do not generate anchors."));
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
                                           tr("Show bounding rectangles and stripes for empty components."),
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
    m_rootWidthAction->setToolTip(tr("Override width of root component."));
    connect(m_rootWidthAction.data(), &LineEditAction::textChanged,
            this, &FormEditorWidget::changeRootItemWidth);
    addAction(m_rootWidthAction.data());
    upperActions.append(m_rootWidthAction.data());

    m_rootHeightAction = new LineEditAction(tr("Override Height"), this);
    m_rootHeightAction->setToolTip(tr("Override height of root component."));
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

    // Zoom actions
    const QString fontName = "qtds_propertyIconFont.ttf";
    const QColor iconColorNormal(Theme::getColor(Theme::IconsBaseColor));
    const QColor iconColorDisabled(Theme::getColor(Theme::IconsDisabledColor));
    const QIcon zoomAllIcon = Utils::StyleHelper::getIconFromIconFont(
        fontName, Theme::getIconUnicode(Theme::Icon::zoomAll), 28, 28, iconColorNormal);

    const QString zoomSelectionUnicode = Theme::getIconUnicode(Theme::Icon::zoomSelection);
    const auto zoomSelectionNormal = Utils::StyleHelper::IconFontHelper(zoomSelectionUnicode,
                                                                        iconColorNormal,
                                                                        QSize(28, 28),
                                                                        QIcon::Normal);
    const auto zoomSelectionDisabeld = Utils::StyleHelper::IconFontHelper(zoomSelectionUnicode,
                                                                          iconColorDisabled,
                                                                          QSize(28, 28),
                                                                          QIcon::Disabled);

    const QIcon zoomSelectionIcon = Utils::StyleHelper::getIconFromIconFont(fontName,
                                                                            {zoomSelectionNormal,
                                                                             zoomSelectionDisabeld});
    const QIcon zoomInIcon = Utils::StyleHelper::getIconFromIconFont(
        fontName, Theme::getIconUnicode(Theme::Icon::zoomIn), 28, 28, iconColorNormal);
    const QIcon zoomOutIcon = Utils::StyleHelper::getIconFromIconFont(
        fontName, Theme::getIconUnicode(Theme::Icon::zoomOut), 28, 28, iconColorNormal);

    auto writeZoomLevel = [this]() {
        double level = m_graphicsView->transform().m11();
        if (level == 1.0) {
            if (m_formEditorView->rootModelNode().hasAuxiliaryData("formeditorZoom"))
                m_formEditorView->rootModelNode().setAuxiliaryData("formeditorZoom", {});
        } else {
            m_formEditorView->rootModelNode().setAuxiliaryData("formeditorZoom", level);
        }
    };

    auto setZoomLevel = [this, writeZoomLevel](double level) {
        if (m_graphicsView) {
            m_graphicsView->setZoomFactor(level);
            writeZoomLevel();
        }
    };

    auto zoomIn = [this, writeZoomLevel]() {
        if (m_graphicsView) {
            double zoom = m_graphicsView->transform().m11();
            zoom = m_zoomAction->setNextZoomFactor(zoom);
            m_graphicsView->setZoomFactor(zoom);
            writeZoomLevel();
        }
    };

    auto zoomOut = [this, writeZoomLevel]() {
        if (m_graphicsView) {
            double zoom = m_graphicsView->transform().m11();
            zoom = m_zoomAction->setPreviousZoomFactor(zoom);
            m_graphicsView->setZoomFactor(zoom);
            writeZoomLevel();
        }
    };

    auto frameAll = [this, zoomOut]() {
        if (m_graphicsView) {
            QRectF bounds;

            QmlItemNode qmlItemNode(m_formEditorView->rootModelNode());
            if (qmlItemNode.isFlowView()) {
                for (QGraphicsItem *item : m_formEditorView->scene()->items()) {
                    if (auto *fitem = FormEditorItem::fromQGraphicsItem(item)) {
                        if (!fitem->qmlItemNode().modelNode().isRootNode()
                            && !fitem->sceneBoundingRect().isNull())
                            bounds |= fitem->sceneBoundingRect();
                    }
                }
            } else {
                bounds = qmlItemNode.instanceBoundingRect();
            }

            m_graphicsView->frame(bounds);
            zoomOut();
        }
    };

    auto frameSelection = [this, zoomOut]() {
        if (m_graphicsView) {
            QRectF boundingRect;
            const QList<ModelNode> nodeList = m_formEditorView->selectedModelNodes();
            for (const ModelNode &node : nodeList) {
                if (FormEditorItem *item = m_formEditorView->scene()->itemForQmlItemNode(node))
                    boundingRect |= item->sceneBoundingRect();
            }
            m_graphicsView->frame(boundingRect);
            zoomOut();
        }
    };

    m_zoomInAction = new QAction(zoomInIcon, tr("Zoom In"), this);
    m_zoomInAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Plus));
    addAction(m_zoomInAction.data());
    upperActions.append(m_zoomInAction.data());
    m_toolBox->addRightSideAction(m_zoomInAction.data());
    connect(m_zoomInAction.data(), &QAction::triggered, zoomIn);

    m_zoomOutAction = new QAction(zoomOutIcon, tr("Zoom Out"), this);
    m_zoomOutAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Minus));
    addAction(m_zoomOutAction.data());
    upperActions.append(m_zoomOutAction.data());
    m_toolBox->addRightSideAction(m_zoomOutAction.data());
    connect(m_zoomOutAction.data(), &QAction::triggered, zoomOut);

    m_zoomAction = new ZoomAction(m_toolActionGroup.data());
    addAction(m_zoomAction.data());
    upperActions.append(m_zoomAction.data());
    m_toolBox->addRightSideAction(m_zoomAction.data());
    connect(m_zoomAction.data(), &ZoomAction::zoomLevelChanged, setZoomLevel);

    m_zoomAllAction = new QAction(zoomAllIcon, tr("Zoom screen to fit all content."), this);
    m_zoomAllAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_0));
    addAction(m_zoomAllAction.data());
    upperActions.append(m_zoomAllAction.data());
    m_toolBox->addRightSideAction(m_zoomAllAction.data());
    connect(m_zoomAllAction.data(), &QAction::triggered, frameAll);

    m_zoomSelectionAction = new QAction(zoomSelectionIcon,
                                        tr("Zoom screen to fit current selection."),
                                        this);
    m_zoomSelectionAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_I));
    addAction(m_zoomSelectionAction.data());
    upperActions.append(m_zoomSelectionAction.data());
    m_toolBox->addRightSideAction(m_zoomSelectionAction.data());
    connect(m_zoomSelectionAction.data(), &QAction::triggered, frameSelection);

    m_resetAction = new QAction(Utils::Icons::RESET_TOOLBAR.icon(), tr("Reset View"), this);
    registerActionAsCommand(m_resetAction, Constants::FORMEDITOR_REFRESH, QKeySequence(Qt::Key_R));

    addAction(m_resetAction.data());
    upperActions.append(m_resetAction.data());
    m_toolBox->addRightSideAction(m_resetAction.data());

    m_graphicsView = new FormEditorGraphicsView(this);
    auto applyZoom = [this](double zoom) { zoomAction()->setZoomFactor(zoom); };
    connect(m_graphicsView, &FormEditorGraphicsView::zoomChanged, applyZoom);
    connect(m_graphicsView, &FormEditorGraphicsView::zoomIn, zoomIn);
    connect(m_graphicsView, &FormEditorGraphicsView::zoomOut, zoomOut);

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

void FormEditorWidget::registerActionAsCommand(QAction *action, Utils::Id id, const QKeySequence &keysequence)
{
    Core::Context context(Constants::C_QMLFORMEDITOR);

    Core::Command *command = Core::ActionManager::registerAction(action, id, context);
    command->setDefaultKeySequence(keysequence);
    command->augmentActionWithShortcutToolTip(action);
}

void FormEditorWidget::initialize()
{
    double defaultZoom = 1.0;
    if (m_formEditorView->model() && m_formEditorView->rootModelNode().isValid()) {
        if (m_formEditorView->rootModelNode().hasAuxiliaryData("formeditorZoom"))
            defaultZoom = m_formEditorView->rootModelNode().auxiliaryData("formeditorZoom").toDouble();
    }
    m_graphicsView->setZoomFactor(defaultZoom);
    m_zoomAction->setZoomFactor(defaultZoom);
    updateActions();
}

void FormEditorWidget::updateActions()
{
    if (m_formEditorView->model() && m_formEditorView->rootModelNode().isValid()) {
        if (m_formEditorView->rootModelNode().hasAuxiliaryData("width")
            && m_formEditorView->rootModelNode().auxiliaryData("width").isValid())
            m_rootWidthAction->setLineEditText(
                m_formEditorView->rootModelNode().auxiliaryData("width").toString());
        else
            m_rootWidthAction->clearLineEditText();
        if (m_formEditorView->rootModelNode().hasAuxiliaryData("height")
            && m_formEditorView->rootModelNode().auxiliaryData("height").isValid())
            m_rootHeightAction->setLineEditText(
                m_formEditorView->rootModelNode().auxiliaryData("height").toString());
        else
            m_rootHeightAction->clearLineEditText();

        if (m_formEditorView->rootModelNode().hasAuxiliaryData("formeditorColor"))
            m_backgroundAction->setColor(
                m_formEditorView->rootModelNode().auxiliaryData("formeditorColor").value<QColor>());
        else
            m_backgroundAction->setColor(Qt::transparent);

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

QAction *FormEditorWidget::zoomSelectionAction() const
{
    return m_zoomSelectionAction.data();
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

QPicture FormEditorWidget::renderToPicture() const
{
    QPicture picture;
    QPainter painter{&picture};

    const QTransform viewportTransform = m_graphicsView->viewportTransform();
    auto items = m_formEditorView->scene()->allFormEditorItems();

    QRectF boundingRect;
    for (auto &item : items)
        boundingRect |= item->childrenBoundingRect();

    picture.setBoundingRect(boundingRect.toRect());
    m_graphicsView->render(&painter, boundingRect, viewportTransform.mapRect(boundingRect.toRect()));

    return picture;
}

FormEditorGraphicsView *FormEditorWidget::graphicsView() const
{
    return m_graphicsView;
}

DocumentWarningWidget *FormEditorWidget::errorWidget()
{
    if (m_documentErrorWidget.isNull()) {
        m_documentErrorWidget = new DocumentWarningWidget(this);
        connect(m_documentErrorWidget.data(),
                &DocumentWarningWidget::gotoCodeClicked,
                [=](const QString &, int codeLine, int codeColumn) {
                    m_formEditorView->gotoError(codeLine, codeColumn);
                });
    }
    return m_documentErrorWidget;
}

void FormEditorWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);

    m_formEditorView->setEnabled(false);
}

void FormEditorWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    const bool wasEnabled = m_formEditorView->isEnabled();
    m_formEditorView->setEnabled(true);

    if (!wasEnabled && m_formEditorView->model()) {
        m_formEditorView->cleanupToolsAndScene();
        m_formEditorView->setupFormEditorWidget();
        m_formEditorView->resetToSelectionTool();
        QmlItemNode rootNode = m_formEditorView->rootModelNode();
        if (rootNode.isValid())
            setRootItemRect(rootNode.instanceBoundingRect());
    }
}

void FormEditorWidget::dragEnterEvent(QDragEnterEvent *dragEnterEvent)
{
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    if (actionManager.externalDragHasSupportedAssets(dragEnterEvent->mimeData()))
        dragEnterEvent->acceptProposedAction();
}

void FormEditorWidget::dropEvent(QDropEvent *dropEvent)
{
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    QHash<QString, QStringList> addedAssets = actionManager.handleExternalAssetsDrop(dropEvent->mimeData());

    // add image assets to Form Editor
    const QStringList addedImages = addedAssets.value(ComponentCoreConstants::addImagesDisplayString);
    for (const QString &imgPath : addedImages) {
        QmlItemNode::createQmlItemNodeFromImage(m_formEditorView, imgPath, {},
                                                m_formEditorView->scene()->rootFormEditorItem()->qmlItemNode());
    }
}

} // namespace QmlDesigner
