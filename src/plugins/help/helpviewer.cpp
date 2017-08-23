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

#include "helpviewer.h"
#include "helpconstants.h"
#include "localhelpmanager.h"

#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

#include <QFileInfo>
#include <QUrl>

#include <QGuiApplication>
#include <QDesktopServices>
#include <QMouseEvent>

#include <QHelpEngine>

using namespace Help::Internal;

struct ExtensionMap {
    const char *extension;
    const char *mimeType;
} extensionMap[] = {
    {".bmp", "image/bmp"},
    {".css", "text/css"},
    {".gif", "image/gif"},
    {".html", "text/html"},
    {".htm", "text/html"},
    {".ico", "image/x-icon"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "application/x-javascript"},
    {".mng", "video/x-mng"},
    {".pbm", "image/x-portable-bitmap"},
    {".pgm", "image/x-portable-graymap"},
    {".pdf", "application/pdf"},
    {".png", "image/png"},
    {".ppm", "image/x-portable-pixmap"},
    {".rss", "application/rss+xml"},
    {".svg", "image/svg+xml"},
    {".svgz", "image/svg+xml"},
    {".text", "text/plain"},
    {".tif", "image/tiff"},
    {".tiff", "image/tiff"},
    {".txt", "text/plain"},
    {".xbm", "image/x-xbitmap"},
    {".xml", "text/xml"},
    {".xpm", "image/x-xpm"},
    {".xsl", "text/xsl"},
    {".xhtml", "application/xhtml+xml"},
    {".wml", "text/vnd.wap.wml"},
    {".wmlc", "application/vnd.wap.wmlc"},
    {"about:blank", 0},
    {0, 0}
};

HelpViewer::HelpViewer(QWidget *parent)
    : QWidget(parent)
{
}

HelpViewer::~HelpViewer()
{
    restoreOverrideCursor();
}

void HelpViewer::setActionVisible(Action action, bool visible)
{
    if (visible)
        m_visibleActions |= Actions(action);
    else
        m_visibleActions &= ~Actions(action);
}

bool HelpViewer::isActionVisible(HelpViewer::Action action)
{
    return (m_visibleActions & Actions(action)) != 0;
}

bool HelpViewer::isLocalUrl(const QUrl &url)
{
    return url.scheme() == "about" // "No documenation available"
            || url.scheme() == "qthelp";
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
    return QString();
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
            Utils::TempFileSaver saver(Utils::TemporaryDirectory::masterDirectoryPath()
                + "/qtchelp_XXXXXX." + QFileInfo(path).completeSuffix());
            saver.setAutoRemove(false);
            if (!saver.hasError())
                saver.write(helpEngine.fileData(resolvedUrl));
            if (saver.finalize(Core::ICore::mainWindow()))
                QDesktopServices::openUrl(QUrl(saver.fileName()));
            return true;
        }
        return false;
    }
    QDesktopServices::openUrl(url);
    return true;
}

void HelpViewer::home()
{
    setSource(LocalHelpManager::homePage());
}

void HelpViewer::slotLoadStarted()
{
    ++m_loadOverrideStack;
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void HelpViewer::slotLoadFinished()
{
    restoreOverrideCursor();
    emit sourceChanged(source());
    emit loadFinished();
}

void HelpViewer::restoreOverrideCursor()
{
    while (m_loadOverrideStack > 0) {
        --m_loadOverrideStack;
        QGuiApplication::restoreOverrideCursor();
    }
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
