// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helpviewer.h"
#include "helpconstants.h"
#include "helptr.h"
#include "localhelpmanager.h"

#include <coreplugin/icore.h>

#include <utils/fadingindicator.h>
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
};

static ExtensionMap extensionMap[] = {
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
    {"about:blank", nullptr}
};

HelpViewer::HelpViewer(QWidget *parent)
    : QWidget(parent)
{
}

HelpViewer::~HelpViewer()
{
    restoreOverrideCursor();
}

void HelpViewer::setAntialias(bool) {}

void HelpViewer::setFontZoom(int percentage)
{
    setScale(percentage / 100.0);
}

void HelpViewer::setScrollWheelZoomingEnabled(bool enabled)
{
    m_scrollWheelZoomingEnabled = enabled;
}

bool HelpViewer::isScrollWheelZoomingEnabled() const
{
    return m_scrollWheelZoomingEnabled;
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

    for (const auto &e : extensionMap) {
        if (ext == e.extension)
            return QLatin1String(e.mimeType);
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
        // Workaround QTBUG-71833
        // QHelpEngineCore::findFile returns a valid url even though the file does not exist
        if (resolvedUrl.scheme() == "about" && resolvedUrl.path() == "blank")
            return false;

        const QString& path = resolvedUrl.path();
        if (!canOpenPage(path)) {
            Utils::TempFileSaver saver(Utils::TemporaryDirectory::masterDirectoryPath()
                + "/qtchelp_XXXXXX." + QFileInfo(path).completeSuffix());
            saver.setAutoRemove(false);
            if (!saver.hasError())
                saver.write(helpEngine.fileData(resolvedUrl));
            if (saver.finalize(Core::ICore::dialogParent()))
                QDesktopServices::openUrl(QUrl(saver.filePath().toString()));
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

void HelpViewer::scaleUp()
{
    incrementZoom(1);
}

void HelpViewer::scaleDown()
{
    incrementZoom(-1);
}

void HelpViewer::resetScale()
{
    applyZoom(100);
}

void HelpViewer::wheelEvent(QWheelEvent *event)
{
    if (m_scrollWheelZoomingEnabled && event->modifiers() == Qt::ControlModifier) {
        event->accept();
        const int deltaY = event->angleDelta().y();
        if (deltaY != 0)
            incrementZoom(deltaY / 120);
        return;
    }
    QWidget::wheelEvent(event);
}

void HelpViewer::incrementZoom(int steps)
{
    const int incrementPercentage = 10 * steps; // 10 percent increase by single step
    const int previousZoom = LocalHelpManager::fontZoom();
    applyZoom(previousZoom + incrementPercentage);
}

void HelpViewer::applyZoom(int percentage)
{
    const int newZoom = LocalHelpManager::setFontZoom(percentage);
    Utils::FadingIndicator::showText(this,
                                     Tr::tr("Zoom: %1%").arg(newZoom),
                                     Utils::FadingIndicator::SmallText);
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
