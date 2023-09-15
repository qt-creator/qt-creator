// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "snapconfiguration.h"

#include "designersettings.h"
#include "edit3dview.h"
#include "edit3dviewconfig.h"

#include <coreplugin/icore.h>

#include <utils/environment.h>

#include <QCursor>
#include <QGuiApplication>
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

SnapConfiguration::SnapConfiguration(Edit3DView *view)
    : QObject(view)
    , m_view(view)
{
}

SnapConfiguration::~SnapConfiguration()
{
    delete m_configDialog;
    restoreCursor();
}

void SnapConfiguration::apply()
{
    if (m_changes) {
        Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_POSITION,
                               m_positionEnabled);
        Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_ROTATION,
                               m_rotationEnabled);
        Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_SCALE,
                               m_scaleEnabled);
        Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_ABSOLUTE,
                               m_absolute);
        Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_POSITION_INTERVAL,
                               m_positionInterval);
        Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_ROTATION_INTERVAL,
                               m_rotationInterval);
        Edit3DViewConfig::save(DesignerSettingsKey::EDIT3DVIEW_SNAP_SCALE_INTERVAL,
                               m_scaleInterval);
        if (!m_view.isNull())
            m_view->syncSnapAuxPropsToSettings();
    }
    deleteLater();
}

void SnapConfiguration::resetDefaults()
{
    setPosEnabled(true);
    setRotEnabled(true);
    setScaleEnabled(true);
    setAbsolute(true);
    setPosInt(defaultPosInt);
    setRotInt(defaultRotInt);
    setScaleInt(defaultScaleInt);
}

void SnapConfiguration::hideCursor()
{
    if (QGuiApplication::overrideCursor())
        return;

    QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    if (QWindow *w = QGuiApplication::focusWindow())
        m_lastPos = QCursor::pos(w->screen());
}

void SnapConfiguration::restoreCursor()
{
    if (!QGuiApplication::overrideCursor())
        return;

    QGuiApplication::restoreOverrideCursor();

    if (QWindow *w = QGuiApplication::focusWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

void SnapConfiguration::holdCursorInPlace()
{
    if (!QGuiApplication::overrideCursor())
        return;

    if (QWindow *w = QGuiApplication::focusWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

int SnapConfiguration::devicePixelRatio()
{
    if (QWindow *w = QGuiApplication::focusWindow())
        return w->devicePixelRatio();

    return 1;
}

void SnapConfiguration::showConfigDialog(const QPoint &pos)
{
    bool posEnabled = Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_POSITION, true).toBool();
    bool rotEnabled = Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_ROTATION, true).toBool();
    bool scaleEnabled = Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_SCALE, true).toBool();
    bool absolute = Edit3DViewConfig::load(DesignerSettingsKey::EDIT3DVIEW_SNAP_ABSOLUTE, true).toBool();
    double posInt = Edit3DViewConfig::load(
                        DesignerSettingsKey::EDIT3DVIEW_SNAP_POSITION_INTERVAL, defaultPosInt).toDouble();
    double rotInt = Edit3DViewConfig::load(
                        DesignerSettingsKey::EDIT3DVIEW_SNAP_ROTATION_INTERVAL, defaultRotInt).toDouble();
    double scaleInt = Edit3DViewConfig::load(
                        DesignerSettingsKey::EDIT3DVIEW_SNAP_SCALE_INTERVAL, defaultScaleInt).toDouble();

    setPosEnabled(posEnabled);
    setRotEnabled(rotEnabled);
    setScaleEnabled(scaleEnabled);
    setAbsolute(absolute);
    setPosInt(posInt);
    setRotInt(rotInt);
    setScaleInt(scaleInt);

    m_changes = false;

    if (!m_configDialog) {
        // Show non-modal progress dialog with cancel button
        QString path = qmlSourcesPath() + "/SnapConfigurationDialog.qml";

        m_configDialog = new QQuickView;
        m_configDialog->setResizeMode(QQuickView::SizeViewToRootObject);
        m_configDialog->setFlags(Qt::Dialog | Qt::FramelessWindowHint);
        m_configDialog->setModality(Qt::NonModal);
        m_configDialog->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");

        m_configDialog->rootContext()->setContextObject(this);
        m_configDialog->setSource(QUrl::fromLocalFile(path));
        m_configDialog->installEventFilter(this);

        QPoint finalPos = pos;
        finalPos.setX(pos.x() - m_configDialog->size().width() / 2);
        finalPos.setY(pos.y());
        m_configDialog->setPosition(finalPos);
    }

    m_configDialog->show();
}

void SnapConfiguration::setPosEnabled(bool enabled)
{
    if (enabled != m_positionEnabled) {
        m_positionEnabled = enabled;
        m_changes = true;
        emit posEnabledChanged();
    }
}

void SnapConfiguration::setRotEnabled(bool enabled)
{
    if (enabled != m_rotationEnabled) {
        m_rotationEnabled = enabled;
        m_changes = true;
        emit rotEnabledChanged();
    }
}

void SnapConfiguration::setScaleEnabled(bool enabled)
{
    if (enabled != m_scaleEnabled) {
        m_scaleEnabled = enabled;
        m_changes = true;
        emit scaleEnabledChanged();
    }
}

void SnapConfiguration::setAbsolute(bool enabled)
{
    if (enabled != m_absolute) {
        m_absolute = enabled;
        m_changes = true;
        emit absoluteChanged();
    }
}

void SnapConfiguration::setPosInt(double value)
{
    if (value != m_positionInterval) {
        m_positionInterval = value;
        m_changes = true;
        emit posIntChanged();
    }
}

void SnapConfiguration::setRotInt(double value)
{
    if (value != m_rotationInterval) {
        m_rotationInterval = value;
        m_changes = true;
        emit rotIntChanged();
    }
}

void SnapConfiguration::setScaleInt(double value)
{
    if (value != m_scaleInterval) {
        m_scaleInterval = value;
        m_changes = true;
        emit scaleIntChanged();
    }
}

void SnapConfiguration::asyncClose()
{
    QTimer::singleShot(0, this, [this]() {
        if (!m_configDialog.isNull() && m_configDialog->isVisible())
            m_configDialog->close();
    });
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
        if (event->type() == QEvent::FocusOut) {
            asyncClose();
        } else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape)
                asyncClose();
        } else if (event->type() == QEvent::Close) {
            apply();
        }
    }

    return QObject::eventFilter(obj, event);
}

} // namespace QmlDesigner
