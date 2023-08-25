// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "snapconfiguration.h"

#include "abstractview.h"
#include "designersettings.h"
#include "edit3dviewconfig.h"
#include "modelnode.h"

#include <coreplugin/icore.h>

#include <utils/environment.h>

#include <QPoint>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QTimer>
#include <QVariant>

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

static QString qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/edit3dQmlSource";
#endif
    return Core::ICore::resourcePath("qmldesigner/edit3dQmlSource").toString();
}

SnapConfiguration::SnapConfiguration(AbstractView *view)
    : QObject(view)
    , m_view(view)
{
}

SnapConfiguration::~SnapConfiguration()
{
    cleanup();
}

void SnapConfiguration::apply()
{
    Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_POSITION_INTERVAL,
                           m_positionInterval);
    Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_ROTATION_INTERVAL,
                           m_rotationInterval);
    Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_SCALE_INTERVAL,
                           m_scaleInterval);
    m_view->rootModelNode().setAuxiliaryData(edit3dSnapPosIntProperty, m_positionInterval);
    m_view->rootModelNode().setAuxiliaryData(edit3dSnapRotIntProperty, m_rotationInterval);
    m_view->rootModelNode().setAuxiliaryData(edit3dSnapScaleIntProperty, m_scaleInterval);
}

void SnapConfiguration::showConfigDialog(const QPoint &pos)
{
    double posInt = Edit3DViewConfig::load(
                        DesignerSettingsKey::EDIT3DVIEW_SNAP_POSITION_INTERVAL, 10.).toDouble();
    double rotInt = Edit3DViewConfig::load(
                        DesignerSettingsKey::EDIT3DVIEW_SNAP_ROTATION_INTERVAL, 15.).toDouble();
    double scaleInt = Edit3DViewConfig::load(
                        DesignerSettingsKey::EDIT3DVIEW_SNAP_SCALE_INTERVAL, 10.).toDouble();
    setPosInt(posInt);
    setRotInt(rotInt);
    setScaleInt(scaleInt);

    if (!m_configDialog) {
        // Show non-modal progress dialog with cancel button
        QString path = qmlSourcesPath() + "/SnapConfigurationDialog.qml";

        m_configDialog = new QQuickView;
        m_configDialog->setTitle(tr("3D Snap Configuration"));
        m_configDialog->setResizeMode(QQuickView::SizeRootObjectToView);
        m_configDialog->setFlags(Qt::Dialog);
        m_configDialog->setModality(Qt::ApplicationModal);
        m_configDialog->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
        m_configDialog->setMinimumSize({280, 170});

        m_configDialog->rootContext()->setContextProperties({
            {"rootView", QVariant::fromValue(this)}
        });
        m_configDialog->setSource(QUrl::fromLocalFile(path));
        m_configDialog->installEventFilter(this);

        QPoint finalPos = pos;
        finalPos.setX(pos.x() - m_configDialog->size().width() / 2);
        finalPos.setY(pos.y() - m_configDialog->size().height() / 2);
        m_configDialog->setPosition(finalPos);
    }

    m_configDialog->show();
}

void SnapConfiguration::setPosInt(double value)
{
    if (value != m_positionInterval) {
        m_positionInterval = value;
        emit posIntChanged();
    }
}

void SnapConfiguration::setRotInt(double value)
{
    if (value != m_rotationInterval) {
        m_rotationInterval = value;
        emit rotIntChanged();
    }
}

void SnapConfiguration::setScaleInt(double value)
{
    if (value != m_scaleInterval) {
        m_scaleInterval = value;
        emit scaleIntChanged();
    }
}

void SnapConfiguration::cleanup()
{
    delete m_configDialog;
}

void SnapConfiguration::cancel()
{
    if (!m_configDialog.isNull() && m_configDialog->isVisible())
        m_configDialog->close();

    deleteLater();
}

bool SnapConfiguration::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_configDialog) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape)
                cancel();
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                // Apply asynchronously to allow the final value to be set by dialog before apply
                QTimer::singleShot(0, this, [this]() {
                    apply();
                    cancel();
                });
            }
        } else if (event->type() == QEvent::Close) {
            cancel();
        }
    }

    return QObject::eventFilter(obj, event);
}

} // namespace QmlDesigner
