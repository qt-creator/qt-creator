// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "liveviewwidget.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QBuffer>
#include <QDataStream>
#include <QPushButton>

namespace QmlDesigner {

using namespace ProjectExplorer;

LiveViewWidget::LiveViewWidget(QWidget *parent) : QWidget(parent)
    , m_sharedMemory("LiveViewSharedMemory")
{
    if (!m_sharedMemory.attach() && !m_sharedMemory.create(1024))
        qDebug() << "Warning: Unable to create shared memory segment.";

    // Hacky way to detect floating dock window movement
    m_timer.callOnTimeout(this, &LiveViewWidget::resizePreview);
    m_timer.setInterval(10);
    m_timer.start();

    QPushButton *button =  new QPushButton("Start Live Preview", this);
    button->move(15, 15);
    connect(button, &QAbstractButton::pressed, this, []() {
        ProjectExplorerPlugin::runStartupProject(Constants::QML_PREVIEW_RUN_MODE, true);
    });
}

void LiveViewWidget::resizePreview()
{
    if (!m_sharedMemory.isAttached())
        return;

    QRect liveRect;

    if (isVisible()) {
        double ratio = devicePixelRatio();
        QSize liveSize = size() * ratio;
        QPoint livePos = mapToGlobal(geometry().topLeft()) * ratio;
        liveRect = QRect(livePos, liveSize);
    }

    if (liveRect != m_lastRect) {
        m_lastRect = liveRect;
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QBuffer::WriteOnly);
        QDataStream out(&buffer);
        out << m_lastRect;
        buffer.close();

        m_sharedMemory.lock();
        char *to = (char *)m_sharedMemory.data();
        const char *from = byteArray.data();
        memcpy(to, from, qMin(m_sharedMemory.size(), byteArray.size()));
        m_sharedMemory.unlock();
    }
}

} // namespace QmlDesigner
