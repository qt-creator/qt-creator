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
#include <private/qquick3dcamera_p.h>
#include <private/qquick3dperspectivecamera_p.h>
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
        addCurrentNodeToRenderQueue();
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
    auto initView = [&obj, this](const QString &viewId, QQuick3DViewport *&view3D) {
        QQmlProperty viewProp(obj, viewId, context());
        QObject *viewObj = viewProp.read().value<QObject *>();
        view3D = qobject_cast<QQuick3DViewport *>(viewObj);
    };
    initView("view3d", m_view3D);
    initView("iconView3d", m_iconView3D);

    if (m_view3D) {
        QQmlProperty sceneNodeProp(obj, "sceneNode", context());
        m_sceneNode = sceneNodeProp.read().value<QQuick3DNode *>();
        m_defaultCameraRotation = m_view3D->camera()->rotation();
        m_defaultCameraPosition = m_view3D->camera()->position();
        addInitToRenderQueue();
    }
#endif
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
            if (auto camera = qobject_cast<QQuick3DPerspectiveCamera *>(m_view3D->camera())) {
                if (size.width() >= size.height())
                    camera->setFieldOfViewOrientation(QQuick3DPerspectiveCamera::Vertical);
                else
                    camera->setFieldOfViewOrientation(QQuick3DPerspectiveCamera::Horizontal);
            }
            resizeCanvasToRootItem();
            addCurrentNodeToRenderQueue();
        }
        break;
    }
    case View3DActionType::Import3dRotatePreviewModel: {
        QObject *obj = rootItem();
        QQmlProperty sceneNodeProp(obj, "sceneNode", context());
        auto sceneNode = sceneNodeProp.read().value<QQuick3DNode *>();
        if (sceneNode && m_previewData.contains(m_currentNode)) {
            PreviewData &data = m_previewData[m_currentNode];
            QPointF delta = command.value().toPointF();
            m_generalHelper->orbitCamera(m_view3D->camera(), m_view3D->camera()->eulerRotation(),
                                         data.lookAt, {}, {float(delta.x()), float(delta.y()), 0.f});
            // Add 2 renders to keep render timer alive for smooth rotation
            addCurrentNodeToRenderQueue(2);
            data.cameraRotation = m_view3D->camera()->rotation();
            data.cameraPosition = m_view3D->camera()->position();
        }
        break;
    }
    case View3DActionType::Import3dAddPreviewModel: {
        const QVariantHash cmd = command.value().toHash();
        const QString name = cmd["name"].toString();
        const QString qmlName = cmd["qmlName"].toString();
        const QString folder = cmd["folder"].toString();

        bool isUpdate = m_previewData.contains(name);
        if (isUpdate) {
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
        if (!isUpdate) {
            data.cameraRotation = m_defaultCameraRotation;
            data.cameraPosition = m_defaultCameraPosition;
        }

        QFileInfo fi(fileUrl().toLocalFile());
        QString compPath = fi.absolutePath() + '/' + folder + '/' + qmlName + ".qml";
        QQmlComponent comp(engine(), compPath, QQmlComponent::PreferSynchronous);
        data.node = qobject_cast<QQuick3DNode *>(comp.create(context()));
        if (data.node) {
            engine()->setObjectOwnership(data.node, QJSEngine::CppOwnership);
            data.node->setParentItem(m_sceneNode);
            data.node->setParent(m_sceneNode);

            addInitToRenderQueue();

            if (m_currentNode == name)
                addCurrentNodeToRenderQueue();

            addIconToRenderQueue(name);
        }

        break;
    }
    case View3DActionType::Import3dSetCurrentPreviewModel: {
        QString newName = command.value().toString();
        if (m_previewData.contains(newName) && m_currentNode != newName) {
            QQuick3DCamera *camera = m_view3D->camera();
            if (m_previewData.contains(m_currentNode)) {
                PreviewData &oldData = m_previewData[m_currentNode];
                oldData.cameraPosition = camera->position();
                oldData.cameraRotation = camera->rotation();
            }
            m_currentNode = newName;
            const PreviewData &newData = m_previewData[m_currentNode];
            camera->setPosition(newData.cameraPosition);
            camera->setRotation(newData.cameraRotation);
            addInitToRenderQueue();
            addCurrentNodeToRenderQueue();
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
#ifdef QUICK3D_MODULE
    if (!m_renderQueue.isEmpty() && timerMode() == TimerMode::NormalTimer)
        return;
#endif

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

#ifdef QUICK3D_MODULE
void Qt5Import3dNodeInstanceServer::addInitToRenderQueue()
{
    startRenderTimer();

    // "Init" render is simply a rendering of the entire scene without producing any images.
    // This is done to make sure everything is initialized properly for subsequent renders.

    if (m_renderQueue.isEmpty() || m_renderQueue[0] != RenderType::Init)
        m_renderQueue.prepend(RenderType::Init);
}

void Qt5Import3dNodeInstanceServer::addCurrentNodeToRenderQueue(int count)
{
    startRenderTimer();

    int remaining = count;
    for (const RenderType &type : std::as_const(m_renderQueue)) {
        if (type == RenderType::CurrentNode && --remaining <= 0)
            return;
    }

    int index = !m_renderQueue.isEmpty() && m_renderQueue[0] == RenderType::Init ? 1 : 0;

    while (remaining > 0) {
        m_renderQueue.insert(index, RenderType::CurrentNode);
        --remaining;
    }
}

void Qt5Import3dNodeInstanceServer::addIconToRenderQueue(const QString &assetName)
{
    startRenderTimer();

    m_generateIconQueue.append(assetName);
    m_renderQueue.append(RenderType::NextIcon);
}
#endif

void Qt5Import3dNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;

    if (!rootNodeInstance().holdsGraphical())
        return;

    if (!inFunction) {
        inFunction = true;
#ifdef QUICK3D_MODULE
        QQuickDesignerSupport::polishItems(quickWindow());
#endif
        render();

        inFunction = false;
    }
}

void Qt5Import3dNodeInstanceServer::render()
{
#ifdef QUICK3D_MODULE
    if (m_renderQueue.isEmpty() || !m_view3D || !m_iconView3D)
        return;

    RenderType currentType = m_renderQueue.takeFirst();
    PreviewData data;
    QQuick3DViewport *currentView = m_view3D;
    QVector3D cameraPosition = m_view3D->camera()->position();
    QQuaternion cameraRotation = m_view3D->camera()->rotation();

    if (currentType == RenderType::Init) {
        m_view3D->setVisible(true);
        m_iconView3D->setVisible(true);
    } else {
        auto showNode = [this](const QString &name) {
            for (const PreviewData &data : std::as_const(m_previewData))
                data.node->setVisible(data.name == name);
        };

        if (currentType == RenderType::CurrentNode) {
            if (m_previewData.contains(m_currentNode)) {
                showNode(m_currentNode);
                data = m_previewData[m_currentNode];
                m_view3D->setVisible(true);
                m_iconView3D->setVisible(false);
            }
        } else if (currentType == RenderType::NextIcon) {
            if (!m_generateIconQueue.isEmpty()) {
                const QString assetName = m_generateIconQueue.takeFirst();
                if (m_previewData.contains(assetName)) {
                    m_view3D->setVisible(false);
                    m_iconView3D->setVisible(true);
                    showNode(assetName);
                    data = m_previewData[assetName];
                    m_refocus = true;
                    currentView = m_iconView3D;
                    currentView->camera()->setRotation(m_defaultCameraRotation);
                    currentView->camera()->setPosition(m_defaultCameraPosition);
                }
            }
        }

        if (m_refocus && data.node) {
            m_generalHelper->calculateBoundsAndFocusCamera(currentView->camera(), data.node,
                                                           currentView, 1050, false, data.lookAt,
                                                           data.extents);
            if (currentType == RenderType::CurrentNode) {
                auto getExtentStr = [&data](int idx) -> QString {
                    int prec = 0;
                    float val = data.extents[idx];

                    if (val == 0.f) {
                        prec = 1;
                    } else {
                        while (val < 100.f) {
                            ++prec;
                            val *= 10.f;
                        }
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
        }
    }

    currentView->update();
    QImage renderImage = grabWindow();

    if (currentType == RenderType::Init) {
        m_refocus = true;
    } else if (currentType == RenderType::CurrentNode) {
        if (m_previewData.contains(m_currentNode)) {
            ImageContainer imgContainer(0, renderImage, 1000000);
            nodeInstanceClient()->handlePuppetToCreatorCommand(
                {PuppetToCreatorCommand::Import3DPreviewImage, QVariant::fromValue(imgContainer)});
        }
    } else if (currentType == RenderType::NextIcon) {
        if (!data.name.isEmpty()) {
            QSizeF iconSize = m_iconView3D->size();
            QImage iconImage = renderImage.copy(0, 0, iconSize.width(), iconSize.height());
            static qint32 renderId = 1000001;
            ImageContainer imgContainer(0, iconImage, ++renderId);
            QVariantList cmdData;
            cmdData.append(data.name);
            cmdData.append(QVariant::fromValue(imgContainer));
            nodeInstanceClient()->handlePuppetToCreatorCommand(
                {PuppetToCreatorCommand::Import3DPreviewIcon, cmdData});
        }
        m_refocus = true;
        currentView->camera()->setRotation(cameraRotation);
        currentView->camera()->setPosition(cameraPosition);
    }
    if (m_renderQueue.isEmpty())
        slowDownRenderTimer();
#else
    slowDownRenderTimer();
#endif
}

} // namespace QmlDesigner
