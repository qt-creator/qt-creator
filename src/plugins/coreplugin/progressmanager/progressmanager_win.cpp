/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <QtCore/QVariant>
#include <QtGui/QMainWindow>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QLabel>

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

    WId winId = Core::ICore::instance()->mainWindow()->winId();
    if (text.isNull()) {
        pITask->SetOverlayIcon(winId, NULL, NULL);
    } else {
        QPixmap pix = QPixmap(":/projectexplorer/images/compile_error.png");
        QPainter p(&pix);
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPointSize(font.pointSize()-2);
        p.setFont(font);
        p.drawText(QRect(QPoint(0,0), pix.size()), Qt::AlignHCenter|Qt::AlignCenter, text);
        pITask->SetOverlayIcon(winId, pix.toWinHICON(), text.utf16());
    }
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressRange(int min, int max)
{
    total = max-min;
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressValue(int value)
{
    if (pITask) {
        WId winId = Core::ICore::instance()->mainWindow()->winId();
        pITask->SetProgressValue(winId, value, total);
    }
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressVisible(bool visible)
{
    if (!pITask)
        return;

    WId winId = Core::ICore::instance()->mainWindow()->winId();
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
