/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include <QVariant>
#include <QMainWindow>
#include <QFont>
#include <QFontMetrics>
#include <QPixmap>
#include <QPainter>
#if QT_VERSION >= 0x050000
#    include <QGuiApplication>
#    include <QWindow>
#    include <QPlatformNativeInterface>
#endif
#include <QLabel>

#include <coreplugin/icore.h>

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

#if QT_VERSION >= 0x050000

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
#else
static inline HWND hwndOfWidget(const QWidget *w)
{
    return w->winId();
}
#endif

void Core::Internal::ProgressManagerPrivate::init()
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


void Core::Internal::ProgressManagerPrivate::setApplicationLabel(const QString &text)
{
    if (!pITask)
        return;

    const HWND winId = hwndOfWidget(Core::ICore::mainWindow());
    if (text.isEmpty()) {
        pITask->SetOverlayIcon(winId, NULL, NULL);
    } else {
        QPixmap pix = QPixmap(":/projectexplorer/images/compile_error.png");
        QPainter p(&pix);
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPointSize(font.pointSize()-2);
        p.setFont(font);
        p.drawText(QRect(QPoint(0,0), pix.size()), Qt::AlignHCenter|Qt::AlignCenter, text);
#if QT_VERSION >= 0x050000
        // See pixmaputils.cpp in the Windows plugin.
        Q_UNIMPLEMENTED();
#else
        const HICON icon = pix.toWinHICON();
        pITask->SetOverlayIcon(winId, icon, (wchar_t*)text.utf16());
        DestroyIcon(icon);
#endif
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

void Core::Internal::ProgressManagerPrivate::init()
{
}

void Core::Internal::ProgressManagerPrivate::cleanup()
{
}

void Core::Internal::ProgressManagerPrivate::setApplicationLabel(const QString &text)
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
