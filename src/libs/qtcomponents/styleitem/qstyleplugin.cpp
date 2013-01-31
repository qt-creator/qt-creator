/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qdeclarative.h>
#include "qstyleplugin.h"
#include "qstyleitem.h"
#include "qrangemodel.h"
#include "qtmenu.h"
#include "qtmenubar.h"
#include "qtmenuitem.h"
#include "qwheelarea.h"
#include <qdeclarativeextensionplugin.h>

#include <qdeclarativeengine.h>
#include <qdeclarative.h>
#include <qdeclarativeitem.h>
#include <qdeclarativeimageprovider.h>
#include <qdeclarativeview.h>
#include <QApplication>
#include <QImage>

// Load icons from desktop theme
class DesktopIconProvider : public QDeclarativeImageProvider
{
public:
    DesktopIconProvider()
        : QDeclarativeImageProvider(QDeclarativeImageProvider::Pixmap)
    {
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
    {
        Q_UNUSED(requestedSize);
        Q_UNUSED(size);
        int pos = id.lastIndexOf(QLatin1Char('/'));
        QString iconName = id.right(id.length() - pos);
        int width = qApp->style()->pixelMetric(QStyle::PM_ToolBarIconSize);
        return QIcon::fromTheme(iconName).pixmap(width);
    }
};


void StylePlugin::registerTypes(const char *uri)
{
//    qDebug() << "register" << uri;
    qmlRegisterType<QStyleItem>(uri, 1, 0, "QStyleItem");
    qmlRegisterType<QRangeModel>(uri, 1, 0, "RangeModel");
    qmlRegisterType<QGraphicsDropShadowEffect>(uri, 1, 0, "DropShadow");
    qmlRegisterType<QDeclarativeFolderListModel>(uri, 1, 0, "FileSystemModel");
    qmlRegisterType<QWheelArea>(uri, 1, 0, "WheelArea");
    qmlRegisterType<QtMenu>(uri, 1, 0, "MenuBase");
    qmlRegisterType<QtMenuBar>(uri, 1, 0, "MenuBarBase");
    qmlRegisterType<QtMenuItem>(uri, 1, 0, "MenuItemBase");
}

void StylePlugin::initializeEngine(QDeclarativeEngine *engine, const char *uri)
{
    Q_UNUSED(uri);
    engine->addImageProvider(QLatin1String("desktoptheme"), new DesktopIconProvider);
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(styleplugin, StylePlugin)
#endif
