/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "helpviewer.h"
#include "helpconstants.h"
#include "helpmanager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QStringBuilder>
#include <QtCore/QTemporaryFile>
#include <QtCore/QUrl>

#include <QtGui/QDesktopServices>
#include <QtGui/QMouseEvent>

#include <QtHelp/QHelpEngine>

using namespace Help::Internal;

const QString HelpViewer::NsNokia = QLatin1String("qthelp://com.nokia.");
const QString HelpViewer::NsTrolltech = QLatin1String("qthelp://com.trolltech.");

const QString HelpViewer::AboutBlankPage =
    QCoreApplication::translate("HelpViewer", "<title>about:blank</title>");

const QString HelpViewer::PageNotFoundMessage =
    QCoreApplication::translate("HelpViewer", "<title>Error 404...</title><div "
    "align=\"center\"><br><br><h1>The page could not be found</h1><br><h3>'%1'"
    "</h3></div>");

struct ExtensionMap {
    const char *extension;
    const char *mimeType;
} extensionMap[] = {
    { ".bmp", "image/bmp" },
    { ".css", "text/css" },
    { ".gif", "image/gif" },
    { ".html", "text/html" },
    { ".htm", "text/html" },
    { ".ico", "image/x-icon" },
    { ".jpeg", "image/jpeg" },
    { ".jpg", "image/jpeg" },
    { ".js", "application/x-javascript" },
    { ".mng", "video/x-mng" },
    { ".pbm", "image/x-portable-bitmap" },
    { ".pgm", "image/x-portable-graymap" },
    { ".pdf", "application/pdf" },
    { ".png", "image/png" },
    { ".ppm", "image/x-portable-pixmap" },
    { ".rss", "application/rss+xml" },
    { ".svg", "image/svg+xml" },
    { ".svgz", "image/svg+xml" },
    { ".text", "text/plain" },
    { ".tif", "image/tiff" },
    { ".tiff", "image/tiff" },
    { ".txt", "text/plain" },
    { ".xbm", "image/x-xbitmap" },
    { ".xml", "text/xml" },
    { ".xpm", "image/x-xpm" },
    { ".xsl", "text/xsl" },
    { ".xhtml", "application/xhtml+xml" },
    { ".wml", "text/vnd.wap.wml" },
    { ".wmlc", "application/vnd.wap.wmlc" },
    { "about:blank", 0 },
    { 0, 0 }
};

bool HelpViewer::isLocalUrl(const QUrl &url)
{
    const QString &scheme = url.scheme();
    return scheme.isEmpty()
        || scheme == QLatin1String("file")
        || scheme == QLatin1String("qrc")
        || scheme == QLatin1String("data")
        || scheme == QLatin1String("qthelp")
        || scheme == QLatin1String("about");
}

bool HelpViewer::canOpenPage(const QString &url)
{
    return !mimeFromUrl(url).isEmpty();
}

QString HelpViewer::mimeFromUrl(const QUrl &url)
{
    const QString &path = url.path();
    const int index = path.lastIndexOf(QLatin1Char('.'));
    const QByteArray &ext = path.mid(index).toUtf8().toLower();

    const ExtensionMap *e = extensionMap;
    while (e->extension) {
        if (ext == e->extension)
            return QLatin1String(e->mimeType);
        ++e;
    }
    return QLatin1String("");
}

bool HelpViewer::launchWithExternalApp(const QUrl &url)
{
    if (isLocalUrl(url)) {
        const QHelpEngineCore &helpEngine = LocalHelpManager::helpEngine();
        const QUrl &resolvedUrl = helpEngine.findFile(url);
        if (!resolvedUrl.isValid())
            return false;

        const QString& path = resolvedUrl.path();
        if (!canOpenPage(path)) {
            QTemporaryFile tmpTmpFile;
            if (!tmpTmpFile.open())
                return false;

            const QString &extension = QFileInfo(path).completeSuffix();
            QFile actualTmpFile(tmpTmpFile.fileName() % QLatin1String(".")
                % extension);
            if (!actualTmpFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
                return false;

            actualTmpFile.write(helpEngine.fileData(resolvedUrl));
            actualTmpFile.close();
            return QDesktopServices::openUrl(QUrl(actualTmpFile.fileName()));
        }
    }
    return false;
}

void HelpViewer::home()
{
    const QHelpEngineCore &engine = LocalHelpManager::helpEngine();
    QString homepage = engine.customValue(QLatin1String("HomePage"),
        QLatin1String("")).toString();

    if (homepage.isEmpty()) {
        homepage = engine.customValue(QLatin1String("DefaultHomePage"),
            Help::Constants::AboutBlank).toString();
    }

    setSource(homepage);
}

bool HelpViewer::handleForwardBackwardMouseButtons(QMouseEvent *event)
{
    if (event->button() == Qt::XButton1) {
        backward();
        return true;
    }
    if (event->button() == Qt::XButton2) {
        forward();
        return true;
    }

    return false;
}
