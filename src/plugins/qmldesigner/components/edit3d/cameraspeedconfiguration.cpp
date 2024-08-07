// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cameraspeedconfiguration.h"
#include "edit3dview.h"

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

CameraSpeedConfiguration::CameraSpeedConfiguration(Edit3DView *view)
    : QObject(view)
    , m_view(view)
{
}

CameraSpeedConfiguration::~CameraSpeedConfiguration()
{
    delete m_configDialog;
    restoreCursor();
}

void CameraSpeedConfiguration::apply()
{
    if (m_changes && !m_view.isNull())
        m_view->setCameraSpeedAuxData(speed(), multiplier());
    deleteLater();
}

void CameraSpeedConfiguration::resetDefaults()
{
    setSpeed(defaultSpeed);
    setMultiplier(defaultMultiplier);
}

void CameraSpeedConfiguration::hideCursor()
{
    if (m_cursorHidden)
        return;

    m_cursorHidden = true;

    QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    if (QWindow *w = QGuiApplication::focusWindow())
        m_lastPos = QCursor::pos(w->screen());
}

void CameraSpeedConfiguration::restoreCursor()
{
    if (!m_cursorHidden)
        return;

    m_cursorHidden = false;

    QGuiApplication::restoreOverrideCursor();

    if (QWindow *w = QGuiApplication::focusWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

void CameraSpeedConfiguration::holdCursorInPlace()
{
    if (!m_cursorHidden)
        return;

    if (QWindow *w = QGuiApplication::focusWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

int CameraSpeedConfiguration::devicePixelRatio()
{
    if (QWindow *w = QGuiApplication::focusWindow())
        return w->devicePixelRatio();

    return 1;
}

void CameraSpeedConfiguration::showConfigDialog(const QPoint &pos)
{
    double speed, multiplier;
    m_view->getCameraSpeedAuxData(speed, multiplier);

    setSpeed(speed);
    setMultiplier(multiplier);

    m_changes = false;

    if (!m_configDialog) {
        // Show non-modal progress dialog with cancel button
        QString path = qmlSourcesPath() + "/CameraSpeedConfigurationDialog.qml";

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

void CameraSpeedConfiguration::setSpeed(double value)
{
    if (value != m_speed) {
        m_speed = value;
        m_changes = true;
        emit speedChanged();
        emit totalSpeedChanged();
    }
}

void CameraSpeedConfiguration::setMultiplier(double value)
{
    if (value != m_multiplier) {
        m_multiplier = value;
        m_changes = true;
        emit multiplierChanged();
        emit totalSpeedChanged();
    }
}

void CameraSpeedConfiguration::asyncClose()
{
    QTimer::singleShot(0, this, [this] {
        if (!m_configDialog.isNull() && m_configDialog->isVisible())
            m_configDialog->close();
    });
}

void CameraSpeedConfiguration::cancel()
{
    if (!m_configDialog.isNull() && m_configDialog->isVisible())
        m_configDialog->close();

    deleteLater();
}

bool CameraSpeedConfiguration::eventFilter(QObject *obj, QEvent *event)
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

bool CameraSpeedConfiguration::isQDSTrusted() const
{
    return Edit3DView::isQDSTrusted();
}

} // namespace QmlDesigner
