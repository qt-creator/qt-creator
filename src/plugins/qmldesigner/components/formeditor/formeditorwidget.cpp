// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditorwidget.h"
#include "backgroundaction.h"
#include "designeractionmanager.h"
#include "designericons.h"
#include "designersettings.h"
#include "formeditoritem.h"
#include "formeditorscene.h"
#include "modelnodecontextmenu_helper.h"
#include "qmldesignerconstants.h"
#include "qmldesignericons.h"
#include "qmldesignerplugin.h"
#include "viewmanager.h"

#include <auxiliarydataproperties.h>
#include <backgroundaction.h>
#include <formeditorgraphicsview.h>
#include <formeditorscene.h>
#include <formeditorview.h>
#include <lineeditaction.h>
#include <model.h>
#include <theme.h>
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

namespace {
constexpr AuxiliaryDataKeyView formeditorZoomProperty{AuxiliaryDataType::NodeInstancePropertyOverwrite,
                                                      "formeditorZoom"};
}

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

    m_noSnappingAction = layoutActionGroup->addAction(tr("No Snapping"));
    m_noSnappingAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    m_noSnappingAction->setCheckable(true);
    m_noSnappingAction->setChecked(true);

    registerActionAsCommand(m_noSnappingAction,
                            Constants::FORMEDITOR_NO_SNAPPING,
                            QKeySequence(Qt::Key_T),
                            ComponentCoreConstants::snappingCategory,
                            1);

    m_snappingAndAnchoringAction = layoutActionGroup->addAction(tr("Snap with Anchors"));
    m_snappingAndAnchoringAction->setCheckable(true);
    m_snappingAndAnchoringAction->setChecked(true);

    registerActionAsCommand(m_snappingAndAnchoringAction,
                            Constants::FORMEDITOR_NO_SNAPPING_AND_ANCHORING,
                            QKeySequence(Qt::Key_W),
                            ComponentCoreConstants::snappingCategory,
                            2);

    m_snappingAction = layoutActionGroup->addAction(tr("Snap without Anchors"));
    m_snappingAction->setCheckable(true);
    m_snappingAction->setChecked(true);

    registerActionAsCommand(m_snappingAction,
                            Constants::FORMEDITOR_SNAPPING,
                            QKeySequence(Qt::Key_E),
                            ComponentCoreConstants::snappingCategory,
                            3);

    addActions(layoutActionGroup->actions());

    m_showBoundingRectAction = new QAction(tr("Show Bounds"), this);
    m_showBoundingRectAction->setCheckable(true);
    m_showBoundingRectAction->setChecked(false);
    m_showBoundingRectAction->setIcon(
        DesignerActionManager::instance().contextIcon(DesignerIcons::ShowBoundsIcon));
    registerActionAsCommand(m_showBoundingRectAction,
                            Constants::FORMEDITOR_NO_SHOW_BOUNDING_RECTANGLE,
                            QKeySequence(Qt::Key_A),
                            ComponentCoreConstants::rootCategory,
                            ComponentCoreConstants::Priorities::ShowBoundingRect);

    addAction(m_showBoundingRectAction.data());

    m_rootWidthAction = new LineEditAction(tr("Override Width"), this);
    m_rootWidthAction->setToolTip(tr("Override width of root component."));
    connect(m_rootWidthAction.data(),
            &LineEditAction::textChanged,
            this,
            &FormEditorWidget::changeRootItemWidth);
    addAction(m_rootWidthAction.data());
    upperActions.append(m_rootWidthAction.data());

    m_rootHeightAction = new LineEditAction(tr("Override Height"), this);
    m_rootHeightAction->setToolTip(tr("Override height of root component."));
    connect(m_rootHeightAction.data(),
            &LineEditAction::textChanged,
            this,
            &FormEditorWidget::changeRootItemHeight);
    addAction(m_rootHeightAction.data());
    upperActions.append(m_rootHeightAction.data());

    m_toolBox = new ToolBox(this);
    fillLayout->addWidget(m_toolBox.data());

    m_toolBox->setLeftSideActions(upperActions);

    m_backgroundAction = new BackgroundAction(m_toolActionGroup.data());
    connect(m_backgroundAction.data(),
            &BackgroundAction::backgroundChanged,
            this,
            &FormEditorWidget::changeBackgound);
    addAction(m_backgroundAction.data());
    upperActions.append(m_backgroundAction.data());
    m_toolBox->addRightSideAction(m_backgroundAction.data());

    // Zoom actions
    const QIcon zoomAllIcon = Theme::iconFromName(Theme::Icon::fitAll_medium);
    auto zoomSelectionNormal = Theme::iconFromName(Theme::Icon::fitSelection_medium);
    auto zoomSelectionDisabeld = Theme::iconFromName(Theme::Icon::fitSelection_medium,
                                                     Theme::getColor(
                                                         Theme::Color::DStoolbarIcon_blocked));
    QIcon zoomSelectionIcon;
    zoomSelectionIcon.addPixmap(zoomSelectionNormal.pixmap({16, 16}), QIcon::Normal);
    zoomSelectionIcon.addPixmap(zoomSelectionDisabeld.pixmap({16, 16}), QIcon::Disabled);

    const QIcon zoomInIcon = Theme::iconFromName(Theme::Icon::zoomIn_medium);
    const QIcon zoomOutIcon = Theme::iconFromName(Theme::Icon::zoomOut_medium);
    const QIcon reloadIcon = Theme::iconFromName(Theme::Icon::reload_medium);

    auto writeZoomLevel = [this]() {
        double level = m_graphicsView->transform().m11();
        if (level == 1.0) {
            m_formEditorView->rootModelNode().removeAuxiliaryData(formeditorZoomProperty);
        } else {
            m_formEditorView->rootModelNode().setAuxiliaryData(formeditorZoomProperty, level);
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
    m_zoomInAction->setShortcut(QKeySequence(QKeySequence::ZoomIn));
    addAction(m_zoomInAction.data());
    upperActions.append(m_zoomInAction.data());
    m_toolBox->addRightSideAction(m_zoomInAction.data());
    connect(m_zoomInAction.data(), &QAction::triggered, zoomIn);

    m_zoomOutAction = new QAction(zoomOutIcon, tr("Zoom Out"), this);
    m_zoomOutAction->setShortcut(QKeySequence(QKeySequence::ZoomOut));
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
    m_zoomAllAction->setShortcut(QKeySequence(tr("Ctrl+Alt+0")));

    addAction(m_zoomAllAction.data());
    upperActions.append(m_zoomAllAction.data());
    m_toolBox->addRightSideAction(m_zoomAllAction.data());
    connect(m_zoomAllAction.data(), &QAction::triggered, frameAll);

    m_zoomSelectionAction = new QAction(zoomSelectionIcon,
                                        tr("Zoom screen to fit current selection."),
                                        this);
    m_zoomSelectionAction->setShortcut(QKeySequence(tr("Ctrl+Alt+i")));
    addAction(m_zoomSelectionAction.data());
    upperActions.append(m_zoomSelectionAction.data());
    m_toolBox->addRightSideAction(m_zoomSelectionAction.data());
    connect(m_zoomSelectionAction.data(), &QAction::triggered, frameSelection);

    m_resetAction = new QAction(reloadIcon, tr("Reload View"), this);
    registerActionAsCommand(m_resetAction,
                            Constants::FORMEDITOR_REFRESH,
                            QKeySequence(Qt::Key_R),
                            ComponentCoreConstants::rootCategory,
                            ComponentCoreConstants::Priorities::ResetView);

    addAction(m_resetAction.data());
    upperActions.append(m_resetAction.data());
    m_toolBox->addRightSideAction(m_resetAction.data());

    m_graphicsView = new FormEditorGraphicsView(this);
    auto applyZoom = [this, writeZoomLevel](double zoom) {
        zoomAction()->setZoomFactor(zoom);
        writeZoomLevel();
    };
    connect(m_graphicsView, &FormEditorGraphicsView::zoomChanged, applyZoom);
    connect(m_graphicsView, &FormEditorGraphicsView::zoomIn, zoomIn);
    connect(m_graphicsView, &FormEditorGraphicsView::zoomOut, zoomOut);

    fillLayout->addWidget(m_graphicsView.data());

    QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css");
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
    if (canConvert) {
        m_formEditorView->rootModelNode().setAuxiliaryData(widthProperty, width);
    } else {
        m_formEditorView->rootModelNode().removeAuxiliaryData(widthProperty);
    }
}

void FormEditorWidget::changeRootItemHeight(const QString &heighText)
{
    bool canConvert;
    int height = heighText.toInt(&canConvert);
    if (canConvert) {
        m_formEditorView->rootModelNode().setAuxiliaryData(heightProperty, height);
    } else {
        m_formEditorView->rootModelNode().removeAuxiliaryData(heightProperty);
    }
}

namespace {
constexpr AuxiliaryDataKeyView formeditorColorProperty{AuxiliaryDataType::Temporary,
                                                       "formeditorColor"};
}

void FormEditorWidget::changeBackgound(const QColor &color)
{
    if (color.alpha() == 0) {
        m_graphicsView->activateCheckboardBackground();
        if (m_formEditorView->rootModelNode().hasAuxiliaryData(formeditorColorProperty)) {
            m_formEditorView->rootModelNode().setAuxiliaryDataWithoutLock(formeditorColorProperty,
                                                                          {});
        }
    } else {
        m_graphicsView->activateColoredBackground(color);
        m_formEditorView->rootModelNode().setAuxiliaryDataWithoutLock(formeditorColorProperty,
                                                                      color);
    }
}

void FormEditorWidget::registerActionAsCommand(
    QAction *action, Utils::Id id, const QKeySequence &, const QByteArray &category, int priority)
{
    Core::Context context(Constants::C_QMLFORMEDITOR);

    Core::Command *command = Core::ActionManager::registerAction(action, id, context);

    DesignerActionManager &designerActionManager = QmlDesignerPlugin::instance()
                                                       ->viewManager()
                                                       .designerActionManager();

    designerActionManager.addCreatorCommand(command, category, priority);

    connect(command->action(), &QAction::enabledChanged, command, [command](bool b) {
        command->action()->setVisible(b);
    });

    command->action()->setVisible(command->action()->isEnabled());

    command->augmentActionWithShortcutToolTip(action);
}

void FormEditorWidget::initialize()
{
    double defaultZoom = 1.0;
    if (m_formEditorView->model() && m_formEditorView->rootModelNode().isValid()) {
        if (auto data = m_formEditorView->rootModelNode().auxiliaryData(formeditorZoomProperty)) {
            defaultZoom = data->toDouble();
        }
    }
    m_graphicsView->setZoomFactor(defaultZoom);
    if (m_formEditorView->scene() && m_formEditorView->scene()->rootFormEditorItem())
        m_graphicsView->centerOn(m_formEditorView->scene()->rootFormEditorItem());
    m_zoomAction->setZoomFactor(defaultZoom);
    updateActions();
}

void FormEditorWidget::updateActions()
{
    if (m_formEditorView->model() && m_formEditorView->rootModelNode().isValid()) {
        if (auto data = m_formEditorView->rootModelNode().auxiliaryData(widthProperty)) {
            m_rootWidthAction->setLineEditText(data->toString());
        } else {
            m_rootWidthAction->clearLineEditText();
        }

        if (auto data = m_formEditorView->rootModelNode().auxiliaryData(heightProperty)) {
            m_rootHeightAction->setLineEditText(data->toString());
        } else {
            m_rootHeightAction->clearLineEditText();
        }

        if (auto data = m_formEditorView->rootModelNode().auxiliaryData(formeditorColorProperty)) {
            m_backgroundAction->setColor(data->value<QColor>());
        } else {
            m_backgroundAction->setColor(Qt::transparent);
        }

        if (m_formEditorView->rootModelNode().hasAuxiliaryData(contextImageProperty))
            m_backgroundAction->setColor(BackgroundAction::ContextImage);

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
    return QmlDesignerPlugin::settings().value(DesignerSettingsKey::ITEMSPACING).toDouble();
}

double FormEditorWidget::containerPadding() const
{
    return QmlDesignerPlugin::settings().value(DesignerSettingsKey::CONTAINERPADDING).toDouble();
}

void FormEditorWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (m_formEditorView)
        QmlDesignerPlugin::contextHelp(callback, m_formEditorView->contextHelpId());
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

QImage FormEditorWidget::takeFormEditorScreenshot()
{
    if (!m_formEditorView->scene()->rootFormEditorItem())
        return {};

    const QRectF boundingRect = m_formEditorView->scene()->rootFormEditorItem()->boundingRect();

    m_formEditorView->scene()->manipulatorLayerItem()->setVisible(false);
    QImage image(boundingRect.size().toSize(), QImage::Format_ARGB32);

    if (!m_graphicsView->backgroundImage().isNull()) {
        image = m_graphicsView->backgroundImage();
        const QPoint offset = m_graphicsView->backgroundImage().offset();

        QPainter painter(&image);
        QTransform viewportTransform = m_graphicsView->viewportTransform();

        m_graphicsView->render(&painter,
                               QRectF(-offset, boundingRect.size()),
                               viewportTransform.mapRect(boundingRect).toRect());

        image.setOffset(offset);

    } else {
        QPainter painter(&image);
        QTransform viewportTransform = m_graphicsView->viewportTransform();

        m_graphicsView->render(&painter,
                               QRectF(0, 0, image.width(), image.height()),
                               viewportTransform.mapRect(boundingRect).toRect());
    }

    m_formEditorView->scene()->manipulatorLayerItem()->setVisible(true);

    return image;
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

bool FormEditorWidget::errorMessageBoxIsVisible() const
{
    return m_documentErrorWidget && m_documentErrorWidget->isVisible();
}

void FormEditorWidget::setBackgoundImage(const QImage &image)
{
    m_graphicsView->setBackgoundImage(image);
    updateActions();
}

QImage FormEditorWidget::backgroundImage() const
{
    return m_graphicsView->backgroundImage();
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
                                                     ->viewManager()
                                                     .designerActionManager();
    if (actionManager.externalDragHasSupportedAssets(dragEnterEvent->mimeData()))
        dragEnterEvent->acceptProposedAction();
}

void FormEditorWidget::dropEvent(QDropEvent *dropEvent)
{
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager()
                                                     .designerActionManager();
    QHash<QString, QStringList> addedAssets = actionManager.handleExternalAssetsDrop(
        dropEvent->mimeData());

    m_formEditorView->executeInTransaction("FormEditorWidget::dropEvent", [&] {
        // Create Image components for added image assets
        const QStringList addedImages = addedAssets.value(
            ComponentCoreConstants::addImagesDisplayString);
        for (const QString &imgPath : addedImages) {
            QmlItemNode::createQmlItemNodeFromImage(
                m_formEditorView,
                imgPath,
                {},
                m_formEditorView->scene()->rootFormEditorItem()->qmlItemNode(),
                false);
        }

        // Create Text components for added font assets
        const QStringList addedFonts = addedAssets.value(ComponentCoreConstants::addFontsDisplayString);
        for (const QString &fontPath : addedFonts) {
            QString fontFamily = QFileInfo(fontPath).baseName();
            QmlItemNode::createQmlItemNodeFromFont(
                m_formEditorView,
                fontFamily,
                rootItemRect().center(),
                m_formEditorView->scene()->rootFormEditorItem()->qmlItemNode(),
                false);
        }
    });
}

} // namespace QmlDesigner
