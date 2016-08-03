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

#include <QGuiApplication>
#include <QVariant>
#include <QMainWindow>
#include <QFont>
#include <QFontMetrics>
#include <QPixmap>
#include <QPainter>
#include <QWindow>
#include <QLabel>
#include <qpa/qplatformnativeinterface.h>

#include <coreplugin/icore.h>
#include <utils/utilsicons.h>

#include "progressmanager_p.h"

// for windows progress bar
#ifndef __GNUC__
#    include <shobjidl.h>
#endif

// Windows 7 SDK required
#ifdef __ITaskbarList3_INTERFACE_DEFINED__

namespace {
    int total = 0;
    ITaskbarList3* pITask = 0;
}

QT_BEGIN_NAMESPACE
Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &p);
QT_END_NAMESPACE

static inline QWindow *windowOfWidget(const QWidget *widget)
{
    if (QWindow *window = widget->windowHandle())
        return window;
    if (QWidget *topLevel = widget->nativeParentWidget())
        return topLevel->windowHandle();
    return 0;
}

static inline HWND hwndOfWidget(const QWidget *w)
{
    void *result = 0;
    if (QWindow *window = windowOfWidget(w))
        result = QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", window);
    return static_cast<HWND>(result);
}

void Core::Internal::ProgressManagerPrivate::initInternal()
{
    CoInitialize(NULL);
    HRESULT hRes = CoCreateInstance(CLSID_TaskbarList,
                                    NULL,CLSCTX_INPROC_SERVER,
                                    IID_ITaskbarList3,(LPVOID*) &pITask);
     if (FAILED(hRes))
     {
         pITask = 0;
         CoUninitialize();
         return;
     }

     pITask->HrInit();
     return;
}

void Core::Internal::ProgressManagerPrivate::cleanup()
{
    if (pITask) {
    pITask->Release();
    pITask = NULL;
    CoUninitialize();
    }
}


void Core::Internal::ProgressManagerPrivate::doSetApplicationLabel(const QString &text)
{
    if (!pITask)
        return;

    const HWND winId = hwndOfWidget(Core::ICore::mainWindow());
    if (text.isEmpty()) {
        pITask->SetOverlayIcon(winId, NULL, NULL);
    } else {
        QPixmap pix = Utils::Icons::ERROR_TASKBAR.pixmap();
        pix.setDevicePixelRatio(1); // We want device-pixel sized font depending on the pix.height
        QPainter p(&pix);
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPixelSize(pix.height() * 0.5);
        p.setFont(font);
        p.drawText(pix.rect(), Qt::AlignCenter, text);
        const HICON icon = qt_pixmapToWinHICON(pix);
        pITask->SetOverlayIcon(winId, icon, (wchar_t*)text.utf16());
        DestroyIcon(icon);
    }
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressRange(int min, int max)
{
    total = max-min;
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressValue(int value)
{
    if (pITask) {
        const HWND winId = hwndOfWidget(Core::ICore::mainWindow());
        pITask->SetProgressValue(winId, value, total);
    }
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressVisible(bool visible)
{
    if (!pITask)
        return;

    const HWND winId = hwndOfWidget(Core::ICore::mainWindow());
    if (visible)
        pITask->SetProgressState(winId, TBPF_NORMAL);
    else
        pITask->SetProgressState(winId, TBPF_NOPROGRESS);
}

#else

void Core::Internal::ProgressManagerPrivate::initInternal()
{
}

void Core::Internal::ProgressManagerPrivate::cleanup()
{
}

void Core::Internal::ProgressManagerPrivate::doSetApplicationLabel(const QString &text)
{
    Q_UNUSED(text)
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressRange(int min, int max)
{
    Q_UNUSED(min)
    Q_UNUSED(max)
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressValue(int value)
{
    Q_UNUSED(value)
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressVisible(bool visible)
{
    Q_UNUSED(visible)
}


#endif // __ITaskbarList2_INTERFACE_DEFINED__
