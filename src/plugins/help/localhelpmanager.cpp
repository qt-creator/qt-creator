// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "localhelpmanager.h"

#include "bookmarkmanager.h"
#include "helpmanager.h"
#include "helptr.h"
#include "helpviewer.h"
#include "textbrowserhelpviewer.h"

#ifdef QTC_WEBENGINE_HELPVIEWER
#include "webenginehelpviewer.h"
#include <QWebEngineUrlScheme>
#endif
#ifdef QTC_LITEHTML_HELPVIEWER
#include "litehtmlhelpviewer.h"
#endif
#ifdef QTC_MAC_NATIVE_HELPVIEWER
#include "macwebkithelpviewer.h"
#endif

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/appinfo.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDesktopServices>
#include <QFontDatabase>
#include <QHelpEngine>
#include <QHelpLink>
#include <QMutexLocker>
#include <QVersionNumber>

#include <optional>

using namespace Utils;

namespace Help::Internal {

static LocalHelpManager *m_instance = nullptr;

static bool m_guiNeedsSetup = true;
static bool m_needsCollectionFile = true;

static QMutex m_guiMutex;
static QHelpEngine *m_guiEngine = nullptr;

static QMutex m_bkmarkMutex;
static BookmarkManager *m_bookmarkManager = nullptr;

QList<Core::HelpManager::OnlineHelpHandler> m_onlineHelpHandlerList;

HelpSettings &helpSettings()
{
    static HelpSettings theHelpSettings;
    return theHelpSettings;
}

static QString defaultFallbackFontFamily()
{
    if (Utils::HostOsInfo::isMacHost())
        return QString("Helvetica");
    if (Utils::HostOsInfo::isAnyUnixHost())
        return QString("Sans Serif");
    return QString("Arial");
}

QVariant ViewerBackendAspect::fromSettingsValue(const QVariant &savedValue) const
{
    return indexForItemValue(savedValue);
}

QVariant ViewerBackendAspect::toSettingsValue(const QVariant &valueToSave) const
{
    return itemValueForIndex(valueToSave.toInt());
}

QByteArray ViewerBackendAspect::operator()() const
{
    return itemValueForIndex(value()).toByteArray();
}

HelpSettings::HelpSettings()
{
    setAutoApply(false);

    const auto version = QVersionNumber::fromString(QCoreApplication::applicationVersion());
    const QString defaultHomePage =
        QString("qthelp://org.qt-project.qtcreator.%1%2%3/doc/index.html")
                                   .arg(version.majorVersion())
                                   .arg(version.minorVersion())
                                   .arg(version.microVersion());

    homePage.setSettingsKey("Help/HomePage");
    homePage.setDefaultValue(defaultHomePage);
    homePage.setDisplayStyle(StringAspect::LineEditDisplay);
    homePage.setLabelText(Tr::tr("Home page:"));

    fontZoom.setSettingsKey("Help/FontZoom");
    fontZoom.setRange(10, 3000);
    fontZoom.setDefaultValue(100);
    fontZoom.setSingleStep(10);
    fontZoom.setSuffix(Tr::tr("%"));

    antiAlias.setSettingsKey("Help/FontAntialias");
    antiAlias.setDefaultValue(true);
    antiAlias.setLabelText(Tr::tr("Antialias"));

    scrollWheelZooming.setSettingsKey("Help/UseScrollWheelZooming");
    scrollWheelZooming.setDefaultValue(true);
    scrollWheelZooming.setLabelText(Tr::tr("Enable scroll wheel zooming"));

    returnOnClose.setSettingsKey("Help/ReturnOnClose");
    returnOnClose.setDefaultValue(true);
    returnOnClose.setLabelText(Tr::tr("Return to editor on closing the last page"));
    returnOnClose.setToolTip(
        Tr::tr("Switches to editor context after last help page is closed."));

    lastShownPages.setSettingsKey("Help/LastShownPages");

    viewerBackend.setSettingsKey("Help/ViewerBackend");
    viewerBackend.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    viewerBackend.addOption(Tr::tr("Default (%1)", "Default viewer backend")
                              .arg(LocalHelpManager::defaultViewerBackend().displayName));
    const QVector<HelpViewerFactory> backends = LocalHelpManager::viewerBackends();
    for (const HelpViewerFactory &f : backends)
        viewerBackend.addOption(SelectionAspect::Option(f.displayName, {}, f.id));
    viewerBackend.setEnabled(backends.size() != 1);

    startOption.setSettingsKey("Help/StartOption");
    startOption.setDefaultValue(ShowLastPages);
    startOption.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    startOption.setLabelText(Tr::tr("On help start:"));
    startOption.addOption(Tr::tr("Show My Home Page"));
    startOption.addOption(Tr::tr("Show a Blank Page"));
    startOption.addOption(Tr::tr("Show My Tabs from Last Session"));

    contextHelpOption.setSettingsKey("Help/ContextHelpOption");
    contextHelpOption.setObjectName("contextHelpComboBox");
    contextHelpOption.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    contextHelpOption.setDefaultValue(Core::HelpManager::SideBySideIfPossible);
    contextHelpOption.addOption(Tr::tr("Show Side-by-Side if Possible"));
    contextHelpOption.addOption(Tr::tr("Always Show Side-by-Side"));
    contextHelpOption.addOption(Tr::tr("Always Show in Help Mode"));
    contextHelpOption.addOption(Tr::tr("Always Show in External Window"));
    contextHelpOption.setLabelText(Tr::tr("On context help:"));

    fallbackFont.fontFamily.setSettingsKey("Help/FallbackFontFamily");
    fallbackFont.fontFamily.setDefaultValue(defaultFallbackFontFamily());
    fallbackFont.fontFamily.setLabelText(Tr::tr("Family:"));
    fallbackFont.fontPointSize.setSettingsKey("Help/FallbackFontSize");
    fallbackFont.fontPointSize.setDefaultValue(14);
    fallbackFont.fontPointSize.setLabelText(Tr::tr("Size:"));

    readSettings();
}

LocalHelpManager::LocalHelpManager(QObject *parent)
    : QObject(parent)
{
    m_instance = this;
    qRegisterMetaType<Help::Internal::LocalHelpManager::HelpData>("Help::Internal::LocalHelpManager::HelpData");

    addOnlineHelpHandler({LocalHelpManager::isQtUrl, LocalHelpManager::openQtUrl});
}

LocalHelpManager::~LocalHelpManager()
{
    if (m_bookmarkManager) {
        m_bookmarkManager->saveBookmarks();
        delete m_bookmarkManager;
        m_bookmarkManager = nullptr;
    }

    delete m_guiEngine;
    m_guiEngine = nullptr;
}

LocalHelpManager *LocalHelpManager::instance()
{
    return m_instance;
}

static std::optional<HelpViewerFactory> backendForId(const QByteArray &id)
{
    const QVector<HelpViewerFactory> factories = LocalHelpManager::viewerBackends();
    const auto backend = std::find_if(std::begin(factories),
                                      std::end(factories),
                                      Utils::equal(&HelpViewerFactory::id, id));
    if (backend != std::end(factories))
        return *backend;
    return {};
}

HelpViewerFactory LocalHelpManager::defaultViewerBackend()
{
    const QString backend = Utils::qtcEnvironmentVariable("QTC_HELPVIEWER_BACKEND");
    if (!backend.isEmpty()) {
        const std::optional<HelpViewerFactory> factory = backendForId(backend.toLatin1());
        if (factory)
            return *factory;
    }
    if (!backend.isEmpty())
        qWarning("Help viewer backend \"%s\" not found, using default.", qPrintable(backend));
    const QVector<HelpViewerFactory> backends = viewerBackends();
    return backends.isEmpty() ? HelpViewerFactory() : backends.first();
}

QVector<HelpViewerFactory> LocalHelpManager::viewerBackends()
{
    QVector<HelpViewerFactory> result;
#ifdef QTC_LITEHTML_HELPVIEWER
    result.append({"litehtml", Tr::tr("litehtml"), []() { return new LiteHtmlHelpViewer; }});
#endif
#ifdef QTC_WEBENGINE_HELPVIEWER
    static bool schemeRegistered = false;
    if (!schemeRegistered) {
        schemeRegistered = true;
        QWebEngineUrlScheme scheme("qthelp");
        scheme.setFlags(QWebEngineUrlScheme::LocalScheme | QWebEngineUrlScheme::LocalAccessAllowed);
        QWebEngineUrlScheme::registerScheme(scheme);
    }
    result.append({"qtwebengine", Tr::tr("QtWebEngine"), []() { return new WebEngineHelpViewer; }});
#endif
    result.append(
        {"textbrowser", Tr::tr("QTextBrowser"), []() { return new TextBrowserHelpViewer; }});
#ifdef QTC_MAC_NATIVE_HELPVIEWER
    result.append({"native", Tr::tr("WebKit"), []() { return new MacWebKitHelpViewer; }});
#endif
#ifdef QTC_DEFAULT_HELPVIEWER_BACKEND
    const int index = Utils::indexOf(result, [](const HelpViewerFactory &f) {
        return f.id == QByteArray(QTC_DEFAULT_HELPVIEWER_BACKEND);
    });
    if (QTC_GUARD(index >= 0)) {
        const HelpViewerFactory defaultBackend = result.takeAt(index);
        result.prepend(defaultBackend);
    }
#endif
    return result;
}

HelpViewerFactory LocalHelpManager::viewerBackend()
{
    const QByteArray id = helpSettings().viewerBackend();
    if (!id.isEmpty())
        return backendForId(id).value_or(defaultViewerBackend());
    return defaultViewerBackend();
}

void LocalHelpManager::setupGuiHelpEngine()
{
    if (m_needsCollectionFile) {
        m_needsCollectionFile = false;
        helpEngine().setCollectionFile(HelpManager::collectionFilePath());
        m_guiNeedsSetup = true;
    }

    if (m_guiNeedsSetup) {
        m_guiNeedsSetup = false;
        helpEngine().setupData();
    }
}

void LocalHelpManager::setEngineNeedsUpdate()
{
    m_guiNeedsSetup = true;
}

QHelpEngine &LocalHelpManager::helpEngine()
{
    if (!m_guiEngine) {
        QMutexLocker _(&m_guiMutex);
        if (!m_guiEngine) {
            m_guiEngine = new QHelpEngine(QString());
            m_guiEngine->setReadOnly(false);
            m_guiEngine->setUsesFilterEngine(true);
        }
    }
    return *m_guiEngine;
}

BookmarkManager& LocalHelpManager::bookmarkManager()
{
    if (!m_bookmarkManager) {
        QMutexLocker _(&m_bkmarkMutex);
        if (!m_bookmarkManager)
            m_bookmarkManager = new BookmarkManager;
    }
    return *m_bookmarkManager;
}

QByteArray LocalHelpManager::loadErrorMessage(const QUrl &url, const QString &errorString)
{
    const char g_htmlPage[] =
        "<html>"
        "<head>"
        "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">"
        "<title>%1</title>"
        "<style>"
        "body {padding: 3em 0em; background: #eeeeee;}"
        "hr {color: lightgray; width: 100%;}"
        "img {float: left; opacity: .8;}"
        "#box {background: white; border: 1px solid lightgray; width: 600px; padding: 60px; margin: auto;}"
        "h1 {font-size: 130%; font-weight: bold; border-bottom: 1px solid lightgray; margin-left: 48px;}"
        "h2 {font-size: 100%; font-weight: normal; border-bottom: 1px solid lightgray; margin-left: 48px;}"
        "p {font-size: 90%; margin-left: 48px;}"
        "</style>"
        "</head>"
        "<body>"
        "<div id=\"box\">"
        "<img "
            "src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACA"
            "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\""
            "width=\"32\" height=\"32\"/>"
        "<h1>%2</h1>"
        "<h2>%3</h2>"
        "%4"
        "</div>"
        "</body>"
        "</html>";

    // some of the values we will replace %1...6 inside the former html
    const QString g_percent1 = Tr::tr("Error loading page");
    // percent2 will be the error details
    // percent3 will be the url of the page we got the error from
    const QString g_percent4 = Tr::tr("<p>Check that you have the corresponding "
                                      "documentation set installed.</p>");

    return QString::fromLatin1(g_htmlPage)
        .arg(g_percent1, errorString, Tr::tr("Error loading: %1").arg(url.toString()), g_percent4)
        .toUtf8();
}

LocalHelpManager::HelpData LocalHelpManager::helpData(const QUrl &url)
{
    HelpData data;
    const QHelpEngineCore &engine = helpEngine();

    data.resolvedUrl = engine.findFile(url);
    if (data.resolvedUrl.isValid()) {
        data.data = engine.fileData(data.resolvedUrl);
        data.mimeType = HelpViewer::mimeFromUrl(data.resolvedUrl);
        if (data.mimeType.isEmpty())
            data.mimeType = Utils::Constants::OCTET_STREAM_MIMETYPE;
    } else {
        data.data = loadErrorMessage(url, Tr::tr("The page could not be found"));
        data.mimeType = "text/html";
    }
    return data;
}

QHelpFilterEngine *LocalHelpManager::filterEngine()
{
    return helpEngine().filterEngine();
}

bool LocalHelpManager::canOpenOnlineHelp(const QUrl &url)
{
    return Utils::anyOf(
        m_onlineHelpHandlerList, [url](const Core::HelpManager::OnlineHelpHandler &handler) {
            return handler.handlesUrl(url);
        });
}

bool LocalHelpManager::isQtUrl(const QUrl &url)
{
    const QString address = url.toString();
    return address.startsWith("qthelp://org.qt-project.")
        || address.startsWith("qthelp://com.nokia.")
        || address.startsWith("qthelp://com.trolltech.");
}

void LocalHelpManager::openQtUrl(const QUrl &url)
{
    static const QString unversionedLocalDomainName
        = QString("org.qt-project.%1").arg(Utils::appInfo().id);

    QString urlPrefix = "http://doc.qt.io/";
    if (url.authority().startsWith(unversionedLocalDomainName)) {
        urlPrefix.append(Utils::appInfo().id);
    } else {
        const QString host = url.host();
        const qsizetype dot = host.lastIndexOf('.');
        if (dot < 0) {
            urlPrefix.append("qt-5");
        } else {
            const QString version = host.mid(dot + 1);
            if (version.startsWith('6')) {
                urlPrefix.append("qt-6");
            } else {
                urlPrefix.append("qt-5");
            }
        }
    }
    const QString address = url.toString();
    QDesktopServices::openUrl(QUrl(urlPrefix + address.mid(address.lastIndexOf(QLatin1Char('/')))));
}

bool LocalHelpManager::openOnlineHelp(const QUrl &url)
{
    return Utils::anyOf(
        m_onlineHelpHandlerList, [url](const Core::HelpManager::OnlineHelpHandler &handler) {
            if (handler.handlesUrl(url)) {
                handler.openUrl(url);
                return true;
            }
            return false;
        });
}

QMultiMap<QString, QUrl> LocalHelpManager::linksForKeyword(const QString &keyword)
{
    return HelpManager::linksForKeyword(&LocalHelpManager::helpEngine(), keyword, std::nullopt);
}

void LocalHelpManager::addOnlineHelpHandler(const Core::HelpManager::OnlineHelpHandler &handler)
{
    m_onlineHelpHandlerList.push_back(handler);
}

} // namespace Help::Internal
