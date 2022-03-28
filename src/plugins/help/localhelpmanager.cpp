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

#include "localhelpmanager.h"

#include "bookmarkmanager.h"
#include "helpconstants.h"
#include "helpmanager.h"
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

#include <app/app_version.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/optional.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDesktopServices>
#include <QFontDatabase>
#include <QHelpEngine>
#include <QMutexLocker>

using namespace Help::Internal;

static LocalHelpManager *m_instance = nullptr;

bool LocalHelpManager::m_guiNeedsSetup = true;
bool LocalHelpManager::m_needsCollectionFile = true;

QMutex LocalHelpManager::m_guiMutex;
QHelpEngine* LocalHelpManager::m_guiEngine = nullptr;

QMutex LocalHelpManager::m_bkmarkMutex;
BookmarkManager* LocalHelpManager::m_bookmarkManager = nullptr;

#ifndef HELP_NEW_FILTER_ENGINE

QStandardItemModel *LocalHelpManager::m_filterModel = nullptr;
QString LocalHelpManager::m_currentFilter = QString();
int LocalHelpManager::m_currentFilterIndex = -1;

#endif

static const char kHelpHomePageKey[] = "Help/HomePage";
static const char kFontFamilyKey[] = "Help/FallbackFontFamily";
static const char kFontStyleNameKey[] = "Help/FallbackFontStyleName";
static const char kFontSizeKey[] = "Help/FallbackFontSize";
static const char kFontZoomKey[] = "Help/FontZoom";
static const char kStartOptionKey[] = "Help/StartOption";
static const char kContextHelpOptionKey[] = "Help/ContextHelpOption";
static const char kReturnOnCloseKey[] = "Help/ReturnOnClose";
static const char kUseScrollWheelZooming[] = "Help/UseScrollWheelZooming";
static const char kLastShownPagesKey[] = "Help/LastShownPages";
static const char kLastSelectedTabKey[] = "Help/LastSelectedTab";
static const char kViewerBackend[] = "Help/ViewerBackend";

static const int kDefaultFallbackFontSize = 14;
static const int kDefaultFontZoom = 100;
const int kDefaultStartOption = LocalHelpManager::ShowLastPages;
const int kDefaultContextHelpOption = Core::HelpManager::SideBySideIfPossible;
const bool kDefaultReturnOnClose = false;
const bool kDefaultUseScrollWheelZooming = true;

static QString defaultFallbackFontFamily()
{
    if (Utils::HostOsInfo::isMacHost())
        return QString("Helvetica");
    if (Utils::HostOsInfo::isAnyUnixHost())
        return QString("Sans Serif");
    return QString("Arial");
}

static QString defaultFallbackFontStyleName(const QString &fontFamily)
{
    const QStringList styles = QFontDatabase().styles(fontFamily);
    QTC_ASSERT(!styles.isEmpty(), return QString("Regular"));
    return styles.first();
}

LocalHelpManager::LocalHelpManager(QObject *parent)
    : QObject(parent)
{
    m_instance = this;
    qRegisterMetaType<Help::Internal::LocalHelpManager::HelpData>("Help::Internal::LocalHelpManager::HelpData");
#ifndef HELP_NEW_FILTER_ENGINE
    m_filterModel = new QStandardItemModel(this);
#endif
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

QString LocalHelpManager::defaultHomePage()
{
    static const QString url = QString::fromLatin1("qthelp://org.qt-project.qtcreator."
        "%1%2%3/doc/index.html").arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR)
        .arg(IDE_VERSION_RELEASE);
    return url;
}

QString LocalHelpManager::homePage()
{
    return Core::ICore::settings()->value(kHelpHomePageKey, defaultHomePage()).toString();
}

void LocalHelpManager::setHomePage(const QString &page)
{
    Core::ICore::settings()->setValueWithDefault(kHelpHomePageKey, page, defaultHomePage());
}

QFont LocalHelpManager::fallbackFont()
{
    QSettings *settings = Core::ICore::settings();
    const QString family = settings->value(kFontFamilyKey, defaultFallbackFontFamily()).toString();
    const int size = settings->value(kFontSizeKey, kDefaultFallbackFontSize).toInt();
    QFont font(family, size);
    const QString styleName = settings->value(kFontStyleNameKey,
                                              defaultFallbackFontStyleName(font.family())).toString();
    font.setStyleName(styleName);
    return font;
}

void LocalHelpManager::setFallbackFont(const QFont &font)
{
    Core::ICore::settings()->setValueWithDefault(kFontFamilyKey,
                                                 font.family(),
                                                 defaultFallbackFontFamily());
    Core::ICore::settings()->setValueWithDefault(kFontStyleNameKey,
                                                 font.styleName(),
                                                 defaultFallbackFontStyleName(font.family()));
    Core::ICore::settings()->setValueWithDefault(kFontSizeKey,
                                                 font.pointSize(),
                                                 kDefaultFallbackFontSize);
    emit m_instance->fallbackFontChanged(font);
}

int LocalHelpManager::fontZoom()
{
    return Core::ICore::settings()->value(kFontZoomKey, kDefaultFontZoom).toInt();
}

int LocalHelpManager::setFontZoom(int percentage)
{
    const int newZoom = qBound(10, percentage, 3000);
    if (newZoom == fontZoom())
        return newZoom;

    Core::ICore::settings()->setValueWithDefault(kFontZoomKey, newZoom, kDefaultFontZoom);
    emit m_instance->fontZoomChanged(newZoom);
    return newZoom;
}

LocalHelpManager::StartOption LocalHelpManager::startOption()
{
    const QVariant value = Core::ICore::settings()->value(kStartOptionKey, kDefaultStartOption);
    bool ok;
    int optionValue = value.toInt(&ok);
    if (!ok)
        optionValue = ShowLastPages;
    switch (optionValue) {
    case ShowHomePage:
        return ShowHomePage;
    case ShowBlankPage:
        return ShowBlankPage;
    case ShowLastPages:
        return ShowLastPages;
    default:
        break;
    }
    return ShowLastPages;
}

void LocalHelpManager::setStartOption(LocalHelpManager::StartOption option)
{
    Core::ICore::settings()->setValueWithDefault(kStartOptionKey, int(option), kDefaultStartOption);
}

Core::HelpManager::HelpViewerLocation LocalHelpManager::contextHelpOption()
{
    const QVariant value = Core::ICore::settings()->value(kContextHelpOptionKey,
                                                          kDefaultContextHelpOption);
    bool ok;
    int optionValue = value.toInt(&ok);
    if (!ok)
        optionValue = Core::HelpManager::SideBySideIfPossible;
    switch (optionValue) {
    case Core::HelpManager::SideBySideIfPossible:
        return Core::HelpManager::SideBySideIfPossible;
    case Core::HelpManager::SideBySideAlways:
        return Core::HelpManager::SideBySideAlways;
    case Core::HelpManager::HelpModeAlways:
        return Core::HelpManager::HelpModeAlways;
    case Core::HelpManager::ExternalHelpAlways:
        return Core::HelpManager::ExternalHelpAlways;
    default:
        break;
    }
    return Core::HelpManager::SideBySideIfPossible;
}

void LocalHelpManager::setContextHelpOption(Core::HelpManager::HelpViewerLocation location)
{
    if (location == contextHelpOption())
        return;
    Core::ICore::settings()->setValueWithDefault(kContextHelpOptionKey,
                                                 int(location),
                                                 kDefaultContextHelpOption);
    emit m_instance->contextHelpOptionChanged(location);
}

bool LocalHelpManager::returnOnClose()
{
    const QVariant value = Core::ICore::settings()->value(kReturnOnCloseKey, kDefaultReturnOnClose);
    return value.toBool();
}

void LocalHelpManager::setReturnOnClose(bool returnOnClose)
{
    Core::ICore::settings()->setValueWithDefault(kReturnOnCloseKey,
                                                 returnOnClose,
                                                 kDefaultReturnOnClose);
    emit m_instance->returnOnCloseChanged();
}

bool LocalHelpManager::isScrollWheelZoomingEnabled()
{
    return Core::ICore::settings()
        ->value(kUseScrollWheelZooming, kDefaultUseScrollWheelZooming)
        .toBool();
}

void LocalHelpManager::setScrollWheelZoomingEnabled(bool enabled)
{
    Core::ICore::settings()->setValueWithDefault(kUseScrollWheelZooming,
                                                 enabled,
                                                 kDefaultUseScrollWheelZooming);
    emit m_instance->scrollWheelZoomingEnabledChanged(enabled);
}

QStringList LocalHelpManager::lastShownPages()
{
    const QVariant value = Core::ICore::settings()->value(kLastShownPagesKey, QVariant());
    return value.toString().split(Constants::ListSeparator, Qt::SkipEmptyParts);
}

void LocalHelpManager::setLastShownPages(const QStringList &pages)
{
    Core::ICore::settings()->setValueWithDefault(kLastShownPagesKey,
                                                 pages.join(Constants::ListSeparator));
}

int LocalHelpManager::lastSelectedTab()
{
    const QVariant value = Core::ICore::settings()->value(kLastSelectedTabKey, 0);
    return value.toInt();
}

void LocalHelpManager::setLastSelectedTab(int index)
{
    Core::ICore::settings()->setValueWithDefault(kLastSelectedTabKey, index, -1);
}

static Utils::optional<HelpViewerFactory> backendForId(const QByteArray &id)
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
    const QByteArray backend = qgetenv("QTC_HELPVIEWER_BACKEND");
    if (!backend.isEmpty()) {
        const Utils::optional<HelpViewerFactory> factory = backendForId(backend);
        if (factory)
            return *factory;
    }
    if (!backend.isEmpty())
        qWarning("Help viewer backend \"%s\" not found, using default.", backend.constData());
    const QVector<HelpViewerFactory> backends = viewerBackends();
    return backends.isEmpty() ? HelpViewerFactory() : backends.first();
}

QVector<HelpViewerFactory> LocalHelpManager::viewerBackends()
{
    QVector<HelpViewerFactory> result;
#ifdef QTC_LITEHTML_HELPVIEWER
    result.append({"litehtml", tr("litehtml"), []() { return new LiteHtmlHelpViewer; }});
#endif
#ifdef QTC_WEBENGINE_HELPVIEWER
    static bool schemeRegistered = false;
    if (!schemeRegistered) {
        schemeRegistered = true;
        QWebEngineUrlScheme scheme("qthelp");
        scheme.setFlags(QWebEngineUrlScheme::LocalScheme | QWebEngineUrlScheme::LocalAccessAllowed);
        QWebEngineUrlScheme::registerScheme(scheme);
    }
    result.append({"qtwebengine", tr("QtWebEngine"), []() { return new WebEngineHelpViewer; }});
#endif
    result.append({"textbrowser", tr("QTextBrowser"), []() { return new TextBrowserHelpViewer; }});
#ifdef QTC_MAC_NATIVE_HELPVIEWER
    result.append({"native", tr("WebKit"), []() { return new MacWebKitHelpViewer; }});
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
    const QByteArray id = Core::ICore::settings()->value(kViewerBackend).toByteArray();
    if (!id.isEmpty())
        return backendForId(id).value_or(defaultViewerBackend());
    return defaultViewerBackend();
}

void LocalHelpManager::setViewerBackendId(const QByteArray &id)
{
    Core::ICore::settings()->setValueWithDefault(kViewerBackend, id, {});
}

QByteArray LocalHelpManager::viewerBackendId()
{
    return Core::ICore::settings()->value(kViewerBackend).toByteArray();
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            m_guiEngine->setReadOnly(false);
#endif

#ifdef HELP_NEW_FILTER_ENGINE
            m_guiEngine->setUsesFilterEngine(true);
#endif
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
    const QString g_percent1 = QCoreApplication::translate("Help", "Error loading page");
    // percent2 will be the error details
    // percent3 will be the url of the page we got the error from
    const QString g_percent4 = QCoreApplication::translate("Help", "<p>Check that you have the corresponding "
        "documentation set installed.</p>");

    return QString::fromLatin1(g_htmlPage).arg(g_percent1, errorString,
                QCoreApplication::translate("Help", "Error loading: %1").arg(url.toString()),
                g_percent4).toUtf8();
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
            data.mimeType = "application/octet-stream";
    } else {
        data.data = loadErrorMessage(url, QCoreApplication::translate(
                                         "Help", "The page could not be found"));
        data.mimeType = "text/html";
    }
    return data;
}

#ifndef HELP_NEW_FILTER_ENGINE

QAbstractItemModel *LocalHelpManager::filterModel()
{
    return m_filterModel;
}

void LocalHelpManager::setFilterIndex(int index)
{
    if (index == m_currentFilterIndex)
        return;
    m_currentFilterIndex = index;
    QStandardItem *item = m_filterModel->item(index);
    if (!item) {
        helpEngine().setCurrentFilter(QString());
        return;
    }
    helpEngine().setCurrentFilter(item->text());
    emit m_instance->filterIndexChanged(m_currentFilterIndex);
}

int LocalHelpManager::filterIndex()
{
    return m_currentFilterIndex;
}

void LocalHelpManager::updateFilterModel()
{
    const QHelpEngine &engine = helpEngine();
    if (m_currentFilter.isEmpty())
        m_currentFilter = engine.currentFilter();
    m_filterModel->clear();
    m_currentFilterIndex = -1;
    int count = 0;
    const QStringList &filters = engine.customFilters();
    foreach (const QString &filterString, filters) {
        m_filterModel->appendRow(new QStandardItem(filterString));
        if (filterString == m_currentFilter)
            m_currentFilterIndex = count;
        count++;
    }

    if (filters.size() < 1)
        return;
    if (m_currentFilterIndex < 0) {
        m_currentFilterIndex = 0;
        m_currentFilter = filters.at(0);
    }
    emit m_instance->filterIndexChanged(m_currentFilterIndex);
}

#else

QHelpFilterEngine *LocalHelpManager::filterEngine()
{
    return helpEngine().filterEngine();
}

#endif

bool LocalHelpManager::canOpenOnlineHelp(const QUrl &url)
{
    const QString address = url.toString();
    return address.startsWith("qthelp://org.qt-project.")
        || address.startsWith("qthelp://com.nokia.")
        || address.startsWith("qthelp://com.trolltech.");
}

bool LocalHelpManager::openOnlineHelp(const QUrl &url)
{
    static const QString unversionedLocalDomainName = QString("org.qt-project.%1").arg(Core::Constants::IDE_ID);

    if (canOpenOnlineHelp(url)) {
        QString urlPrefix = "http://doc.qt.io/";
        if (url.authority().startsWith(unversionedLocalDomainName)) {
            urlPrefix.append(Core::Constants::IDE_ID);
        } else {
            const auto host = url.host();
            const auto dot = host.lastIndexOf('.');
            if (dot < 0) {
                urlPrefix.append("qt-5");
            } else {
                const auto version = host.mid(dot + 1);
                if (version.startsWith('6')) {
                    urlPrefix.append("qt-6");
                } else {
                    urlPrefix.append("qt-5");
                }
            }
        }
        const QString address = url.toString();
        QDesktopServices::openUrl(QUrl(urlPrefix + address.mid(address.lastIndexOf(QLatin1Char('/')))));
        return true;
    }
    return false;
}
