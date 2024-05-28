// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qt5import3dnodeinstanceserver.h"

#include "createscenecommand.h"
#include "view3dactioncommand.h"

#include "imagecontainer.h"
#include "nodeinstanceclientinterface.h"
#include "puppettocreatorcommand.h"

#include <QFileInfo>
#include <QLocale>
#include <QQmlComponent>
#include <QQmlEngine>

#include <QQmlProperty>

#include <private/qquickdesignersupport_p.h>

#ifdef QUICK3D_MODULE
#include <private/qquick3dnode_p.h>
#include <private/qquick3dviewport_p.h>
#endif

namespace QmlDesigner {

Qt5Import3dNodeInstanceServer::Qt5Import3dNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt5NodeInstanceServer(nodeInstanceClient)
{
    setSlowRenderTimerInterval(100000000);
    setRenderTimerInterval(20);

#ifdef QUICK3D_MODULE
    m_generalHelper = new Internal::GeneralHelper();
    QObject::connect(m_generalHelper, &Internal::GeneralHelper::requestRender, this, [this]() {
        startRenderTimer();
    });
#endif
}

Qt5Import3dNodeInstanceServer::~Qt5Import3dNodeInstanceServer()
{
    cleanup();
}

void Qt5Import3dNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    initializeView();
    registerFonts(command.resourceUrl);
    setTranslationLanguage(command.language);
    setupScene(command);

#ifdef QUICK3D_MODULE
    QObject *obj = rootItem();
    QQmlProperty viewProp(obj, "view3d", context());
    QObject *viewObj = viewProp.read().value<QObject *>();
    m_view3D = qobject_cast<QQuick3DViewport *>(viewObj);
    if (m_view3D) {
        QQmlProperty sceneNodeProp(obj, "sceneNode", context());
        m_sceneNode = sceneNodeProp.read().value<QQuick3DNode *>();
    }
#endif

    startRenderTimer();
}

void Qt5Import3dNodeInstanceServer::view3DAction([[maybe_unused]] const View3DActionCommand &command)
{
#ifdef QUICK3D_MODULE
    switch (command.type()) {
    case View3DActionType::Import3dUpdatePreviewImage: {
        QObject *obj = rootItem();
        if (obj) {
            QSize size =  command.value().toSize();
            QQmlProperty wProp(obj, "width", context());
            QQmlProperty hProp(obj, "height", context());
            wProp.write(size.width());
            hProp.write(size.height());
            resizeCanvasToRootItem();
            startRenderTimer();
        }
        break;
    }
    case View3DActionType::Import3dRotatePreviewModel: {
        QObject *obj = rootItem();
        QQmlProperty sceneNodeProp(obj, "sceneNode", context());
        auto sceneNode = sceneNodeProp.read().value<QQuick3DNode *>();
        if (sceneNode && m_previewData.contains(m_currentNode)) {
            const PreviewData &data = m_previewData[m_currentNode];
            QPointF delta = command.value().toPointF();
            m_generalHelper->orbitCamera(m_view3D->camera(), m_view3D->camera()->eulerRotation(),
                                         data.lookAt, {}, {float(delta.x()), float(delta.y()), 0.f});
            m_keepRendering = true;
            startRenderTimer();
        }
        break;
    }
    case View3DActionType::Import3dAddPreviewModel: {
        const QVariantHash cmd = command.value().toHash();
        const QString name = cmd["name"].toString();
        const QString folder = cmd["folder"].toString();

        if (m_previewData.contains(name)) {
            QQuick3DNode *node = m_previewData[name].node;
            if (node) {
                node->setParentItem({});
                node->setParent({});
                node->deleteLater();
            }
        }

        PreviewData &data = m_previewData[name];
        data.name = name;
        data.lookAt = {};

        QFileInfo fi(fileUrl().toLocalFile());
        QString compPath = fi.absolutePath() + '/' + folder + '/' + name + ".qml";
        QQmlComponent comp(engine(), compPath, QQmlComponent::PreferSynchronous);
        data.node = qobject_cast<QQuick3DNode *>(comp.create(context()));
        if (data.node) {
            engine()->setObjectOwnership(data.node, QJSEngine::CppOwnership);
            data.node->setParentItem(m_sceneNode);
            data.node->setParent(m_sceneNode);
        }

        if (m_currentNode == data.name) {
            m_renderCount = 0;
            startRenderTimer();
        } else if (data.node) {
            data.node->setVisible(false);
        }
        break;
    }
    case View3DActionType::Import3dSetCurrentPreviewModel: {
        QString newName = command.value().toString();
        if (m_previewData.contains(newName) && m_currentNode != newName) {
            const PreviewData &newData = m_previewData[newName];
            const PreviewData oldData = m_previewData.value(m_currentNode);
            if (oldData.node)
                oldData.node->setVisible(false);
            if (newData.node)
                newData.node->setVisible(true);
            m_renderCount = 0;
            m_currentNode = newName;
            startRenderTimer();
        }
        break;
    }

    default:
        break;
    }
#endif
}

void Qt5Import3dNodeInstanceServer::startRenderTimer()
{
    if (m_keepRendering && timerMode() == TimerMode::NormalTimer)
        return;

    NodeInstanceServer::startRenderTimer();
}

void Qt5Import3dNodeInstanceServer::cleanup()
{
#ifdef QUICK3D_MODULE
    for (const PreviewData &data : std::as_const(m_previewData))
        delete data.node;
    m_previewData.clear();
    delete m_generalHelper;
#endif
}

void Qt5Import3dNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;

    if (!rootNodeInstance().holdsGraphical())
        return;

    if (!inFunction) {
        inFunction = true;

        QQuickDesignerSupport::polishItems(quickWindow());

        render();

        inFunction = false;
    }
}

void Qt5Import3dNodeInstanceServer::render()
{
#ifdef QUICK3D_MODULE
    ++m_renderCount;

    // Render scene at least once before calculating bounds to ensure geometries are intialized
    if (m_renderCount == 2 && m_view3D && m_previewData.contains(m_currentNode)) {
        PreviewData &data = m_previewData[m_currentNode];
        m_generalHelper->calculateBoundsAndFocusCamera(m_view3D->camera(), data.node,
                                                       m_view3D, 1050, false, data.lookAt,
                                                       data.extents);

        auto getExtentStr = [&data](int idx) -> QString {
            int prec = 0;
            float val = data.extents[idx];
            while (val < 100.f) {
                ++prec;
                val *= 10.f;
            }
            // Strip unnecessary zeroes after decimal separator
            if (prec > 0) {
                QString checkStr = QString::number(data.extents[idx], 'f', prec);
                while (prec > 0 && (checkStr.last(1) == "0" || checkStr.last(1) == ".")) {
                    --prec;
                    checkStr.chop(1);
                }
            }
            QString retval = QLocale().toString(data.extents[idx], 'f', prec);
            return retval;
        };

        QQmlProperty extentsProp(rootItem(), "extents", context());
        extentsProp.write(tr("Dimensions: %1 x %2 x %3").arg(getExtentStr(0))
                                                        .arg(getExtentStr(1))
                                                        .arg(getExtentStr(2)));
    }

    rootNodeInstance().updateDirtyNodeRecursive();
    QImage renderImage = grabWindow();

    if (m_renderCount >= 2) {
        ImageContainer imgContainer(0, renderImage, m_renderCount);
        nodeInstanceClient()->handlePuppetToCreatorCommand(
            {PuppetToCreatorCommand::Import3DPreviewImage,
             QVariant::fromValue(imgContainer)});

        if (!m_keepRendering)
            slowDownRenderTimer();

        m_keepRendering = false;
    }
#else
    slowDownRenderTimer();
#endif
}

} // namespace QmlDesigner
