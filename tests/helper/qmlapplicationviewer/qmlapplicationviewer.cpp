/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmlapplicationviewer.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtDeclarative/QDeclarativeComponent>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeContext>
#include <QtGui/QApplication>

#include <qplatformdefs.h> // MEEGO_EDITION_HARMATTAN

#ifdef HARMATTAN_BOOSTER
#include <MDeclarativeCache>
#endif

#if defined(QMLJSDEBUGGER) && QT_VERSION < 0x040800

#include <qt_private/qdeclarativedebughelper_p.h>

#if !defined(NO_JSDEBUGGER)
#include <jsdebuggeragent.h>
#endif
#if !defined(NO_QMLOBSERVER)
#include <qdeclarativeviewobserver.h>
#endif

// Enable debugging before any QDeclarativeEngine is created
struct QmlJsDebuggingEnabler
{
    QmlJsDebuggingEnabler()
    {
        QDeclarativeDebugHelper::enableDebugging();
    }
};

// Execute code in constructor before first QDeclarativeEngine is instantiated
static QmlJsDebuggingEnabler enableDebuggingHelper;

#endif // QMLJSDEBUGGER

class QmlApplicationViewerPrivate
{
    QmlApplicationViewerPrivate(QDeclarativeView *view_) : view(view_) {}

    QString mainQmlFile;
    QDeclarativeView *view;
    friend class QmlApplicationViewer;
    QString adjustPath(const QString &path);
};

QString QmlApplicationViewerPrivate::adjustPath(const QString &path)
{
#ifdef Q_OS_MAC
    if (!QDir::isAbsolutePath(path))
        return QString::fromLatin1("%1/../Resources/%2")
                .arg(QCoreApplication::applicationDirPath(), path);
#else
    const QString pathInInstallDir =
            QString::fromLatin1("%1/../%2").arg(QCoreApplication::applicationDirPath(), path);
    if (QFileInfo(pathInInstallDir).exists())
        return pathInInstallDir;
#endif
    return path;
}

QmlApplicationViewer::QmlApplicationViewer(QWidget *parent)
    : QDeclarativeView(parent)
    , d(new QmlApplicationViewerPrivate(this))
{
    connect(engine(), SIGNAL(quit()), SLOT(close()));
    setResizeMode(QDeclarativeView::SizeRootObjectToView);
    // Qt versions prior to 4.8.0 don't have QML/JS debugging services built in
#if defined(QMLJSDEBUGGER) && QT_VERSION < 0x040800
#if !defined(NO_JSDEBUGGER)
    new QmlJSDebugger::JSDebuggerAgent(d->view->engine());
#endif
#if !defined(NO_QMLOBSERVER)
    new QmlJSDebugger::QDeclarativeViewObserver(d->view, d->view);
#endif
#endif
}

QmlApplicationViewer::QmlApplicationViewer(QDeclarativeView *view, QWidget *parent)
    : QDeclarativeView(parent)
    , d(new QmlApplicationViewerPrivate(view))
{
    connect(view->engine(), SIGNAL(quit()), view, SLOT(close()));
    view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    // Qt versions prior to 4.8.0 don't have QML/JS debugging services built in
#if defined(QMLJSDEBUGGER) && QT_VERSION < 0x040800
#if !defined(NO_JSDEBUGGER)
    new QmlJSDebugger::JSDebuggerAgent(d->view->engine());
#endif
#if !defined(NO_QMLOBSERVER)
    new QmlJSDebugger::QDeclarativeViewObserver(d->view, d->view);
#endif
#endif
}

QmlApplicationViewer::~QmlApplicationViewer()
{
    delete d;
}

QmlApplicationViewer *QmlApplicationViewer::create()
{
#ifdef HARMATTAN_BOOSTER
    return new QmlApplicationViewer(MDeclarativeCache::qDeclarativeView(), 0);
#else
    return new QmlApplicationViewer();
#endif
}

void QmlApplicationViewer::setMainQmlFile(const QString &file)
{
    d->mainQmlFile = d->adjustPath(file);
    d->view->setSource(QUrl::fromLocalFile(d->mainQmlFile));
}

void QmlApplicationViewer::addImportPath(const QString &path)
{
    d->view->engine()->addImportPath(d->adjustPath(path));
}

void QmlApplicationViewer::setOrientation(ScreenOrientation orientation)
{
#if defined(Q_OS_SYMBIAN)
    // If the version of Qt on the device is < 4.7.2, that attribute won't work
    if (orientation != ScreenOrientationAuto) {
        const QStringList v = QString::fromAscii(qVersion()).split(QLatin1Char('.'));
        if (v.count() == 3 && (v.at(0).toInt() << 16 | v.at(1).toInt() << 8 | v.at(2).toInt()) < 0x040702) {
            qWarning("Screen orientation locking only supported with Qt 4.7.2 and above");
            return;
        }
    }
#endif // Q_OS_SYMBIAN

    Qt::WidgetAttribute attribute;
    switch (orientation) {
#if QT_VERSION < 0x040702
    // Qt < 4.7.2 does not yet have the Qt::WA_*Orientation attributes
    case ScreenOrientationLockPortrait:
        attribute = static_cast<Qt::WidgetAttribute>(128);
        break;
    case ScreenOrientationLockLandscape:
        attribute = static_cast<Qt::WidgetAttribute>(129);
        break;
    default:
    case ScreenOrientationAuto:
        attribute = static_cast<Qt::WidgetAttribute>(130);
        break;
#else // QT_VERSION < 0x040702
    case ScreenOrientationLockPortrait:
        attribute = Qt::WA_LockPortraitOrientation;
        break;
    case ScreenOrientationLockLandscape:
        attribute = Qt::WA_LockLandscapeOrientation;
        break;
    default:
    case ScreenOrientationAuto:
        attribute = Qt::WA_AutoOrientation;
        break;
#endif // QT_VERSION < 0x040702
    };
    setAttribute(attribute, true);
}

void QmlApplicationViewer::showExpanded()
{
#if defined(Q_OS_SYMBIAN) || defined(MEEGO_EDITION_HARMATTAN) || defined(Q_WS_SIMULATOR)
    d->view->showFullScreen();
#elif defined(Q_WS_MAEMO_5)
    d->view->showMaximized();
#else
    d->view->show();
#endif
}

QApplication *createApplication(int &argc, char **argv)
{
#ifdef HARMATTAN_BOOSTER
    return MDeclarativeCache::qApplication(argc, argv);
#else
    return new QApplication(argc, argv);
#endif
}
