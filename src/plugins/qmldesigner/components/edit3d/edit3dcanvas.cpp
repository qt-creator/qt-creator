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

#include "edit3dcanvas.h"
#include "edit3dview.h"
#include "edit3dwidget.h"

#include "nodehints.h"
#include "qmlvisualnode.h"

#include <bindingproperty.h>
#include <nodemetainfo.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>

#include <coreplugin/icore.h>

#include <qmldesignerplugin.h>
#include <qmldesignerconstants.h>

#include <QFileInfo>
#include <QPainter>
#include <QQuickWidget>
#include <QtCore/qmimedata.h>

namespace QmlDesigner {

static QQuickWidget *createBusyIndicator(QWidget *p)
{
    auto widget = new QQuickWidget(p);

    const QString source = Core::ICore::resourcePath("qmldesigner/misc/BusyIndicator.qml").toString();
    QTC_ASSERT(QFileInfo::exists(source), return widget);
    widget->setSource(QUrl::fromLocalFile(source));
    widget->setFixedSize(64, 64);
    widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    widget->setClearColor(Qt::transparent);
    widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    return widget;
}

Edit3DCanvas::Edit3DCanvas(Edit3DWidget *parent)
    : QWidget(parent)
    , m_parent(parent)
    , m_busyIndicator(createBusyIndicator(this))
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setFocusPolicy(Qt::ClickFocus);
    m_busyIndicator->show();
}

void Edit3DCanvas::updateRenderImage(const QImage &img)
{
    m_image = img;
    update();
}

void Edit3DCanvas::updateActiveScene(qint32 activeScene)
{
    m_activeScene = activeScene;
}

QImage QmlDesigner::Edit3DCanvas::renderImage() const
{
    return m_image;
}

void Edit3DCanvas::setOpacity(qreal opacity)
{
    m_opacity = opacity;
}

QWidget *Edit3DCanvas::busyIndicator() const
{
    return m_busyIndicator;
}

void Edit3DCanvas::mousePressEvent(QMouseEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::mousePressEvent(e);
}

void Edit3DCanvas::mouseReleaseEvent(QMouseEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::mouseReleaseEvent(e);
}

void Edit3DCanvas::mouseDoubleClickEvent(QMouseEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::mouseDoubleClickEvent(e);
}

void Edit3DCanvas::mouseMoveEvent(QMouseEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::mouseMoveEvent(e);
}

void Edit3DCanvas::wheelEvent(QWheelEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::wheelEvent(e);
}

void Edit3DCanvas::keyPressEvent(QKeyEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::keyPressEvent(e);
}

void Edit3DCanvas::keyReleaseEvent(QKeyEvent *e)
{
    m_parent->view()->sendInputEvent(e);
    QWidget::keyReleaseEvent(e);
}

void Edit3DCanvas::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    QWidget::paintEvent(e);

    QPainter painter(this);

    if (m_opacity < 1.0) {
        painter.fillRect(rect(), Qt::black);
        painter.setOpacity(m_opacity);
    }

    painter.drawImage(rect(), m_image, QRect(0, 0, m_image.width(), m_image.height()));
}

void Edit3DCanvas::resizeEvent(QResizeEvent *e)
{
    positionBusyInidicator();
    m_parent->view()->edit3DViewResized(e->size());
}

void Edit3DCanvas::dragEnterEvent(QDragEnterEvent *e)
{
    // Block all drags if scene root node is locked
    ModelNode node;
    if (m_parent->view()->hasModelNodeForInternalId(m_activeScene))
        node = m_parent->view()->modelNodeForInternalId(m_activeScene);

    // Allow drop when there is no valid active scene, as the drop goes under the root node of
    // the document in that case.
    if (!node.isValid() || !ModelNode::isThisOrAncestorLocked(node)) {
        QByteArray data = e->mimeData()->data(Constants::MIME_TYPE_ITEM_LIBRARY_INFO);
        if (!data.isEmpty()) {
            QDataStream stream(data);
            stream >> m_itemLibraryEntry;
            if (NodeHints::fromItemLibraryEntry(m_itemLibraryEntry).canBeDroppedInView3D())
                e->accept();
        }
    }
}

void Edit3DCanvas::dropEvent(QDropEvent *e)
{
    auto modelNode = QmlVisualNode::createQml3DNode(m_parent->view(), m_itemLibraryEntry, m_activeScene).modelNode();
    QTC_ASSERT(modelNode.isValid(), return);

    e->accept();
    m_parent->view()->setSelectedModelNode(modelNode);

    // if added node is a Model, assign it a material
    if (modelNode.isSubclassOf("QtQuick3D.Model")) {
        ModelNode matLib = m_parent->view()->modelNodeForId(Constants::MATERIAL_LIB_ID);
        QTC_ASSERT(matLib.isValid(), return);

        const QList<ModelNode> materials = matLib.directSubModelNodes();
        ModelNode material;
        if (materials.size() > 0) {
            for (const ModelNode &mat : materials) {
                if (mat.isSubclassOf("QtQuick3D.Material")) {
                    material = mat;
                    break;
                }
            }
        }

        // if no valid material, create a new default material
        if (!material.isValid()) {
            NodeMetaInfo metaInfo = m_parent->view()->model()->metaInfo("QtQuick3D.DefaultMaterial");
            material = m_parent->view()->createModelNode("QtQuick3D.DefaultMaterial", metaInfo.majorVersion(),
                                                                                      metaInfo.minorVersion());
            VariantProperty matNameProp = material.variantProperty("objectName");
            matNameProp.setValue("New Material");
            material.validId();
            matLib.defaultNodeListProperty().reparentHere(material);
        }

        BindingProperty modelMatsProp = modelNode.bindingProperty("materials");
        modelMatsProp.setExpression(material.id());
    }
}

void Edit3DCanvas::focusOutEvent(QFocusEvent *focusEvent)
{
    QmlDesignerPlugin::emitUsageStatisticsTime(Constants::EVENT_3DEDITOR_TIME,
                                               m_usageTimer.elapsed());
    QWidget::focusOutEvent(focusEvent);
}

void Edit3DCanvas::focusInEvent(QFocusEvent *focusEvent)
{
    m_usageTimer.restart();
    QWidget::focusInEvent(focusEvent);
}

void Edit3DCanvas::positionBusyInidicator()
{
    m_busyIndicator->move(width() / 2 - 32, height() / 2 - 32);
}

}
