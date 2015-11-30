/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "localhelpmanager.h"

#include "bookmarkmanager.h"
#include "helpconstants.h"
#include "helpviewer.h"

#include <app/app_version.h>
#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QMutexLocker>

#include <QHelpEngine>

using namespace Help::Internal;

static LocalHelpManager *m_instance = 0;

bool LocalHelpManager::m_guiNeedsSetup = true;
bool LocalHelpManager::m_needsCollectionFile = true;

QMutex LocalHelpManager::m_guiMutex;
QHelpEngine* LocalHelpManager::m_guiEngine = 0;

QMutex LocalHelpManager::m_bkmarkMutex;
BookmarkManager* LocalHelpManager::m_bookmarkManager = 0;

QStandardItemModel *LocalHelpManager::m_filterModel = 0;
QString LocalHelpManager::m_currentFilter = QString();
int LocalHelpManager::m_currentFilterIndex = -1;

static const char kHelpHomePageKey[] = "Help/HomePage";
static const char kFontKey[] = "Help/Font";
static const char kStartOptionKey[] = "Help/StartOption";
static const char kContextHelpOptionKey[] = "Help/ContextHelpOption";
static const char kReturnOnCloseKey[] = "Help/ReturnOnClose";
static const char kLastShownPagesKey[] = "Help/LastShownPages";
static const char kLastShownPagesZoomKey[] = "Help/LastShownPagesZoom";
static const char kLastSelectedTabKey[] = "Help/LastSelectedTab";

// TODO remove some time after Qt Creator 3.5
static QVariant getSettingWithFallback(const QString &settingsKey,
                                       const QString &fallbackSettingsKey,
                                       const QVariant &fallbackSettingsValue)
{
    QSettings *settings = Core::ICore::settings();
    if (settings->contains(settingsKey))
        return settings->value(settingsKey);
    // read from help engine for old settings
    // TODO remove some time after Qt Creator 3.5
    return LocalHelpManager::helpEngine().customValue(fallbackSettingsKey, fallbackSettingsValue);
}

LocalHelpManager::LocalHelpManager(QObject *parent)
    : QObject(parent)
{
    m_instance = this;
    qRegisterMetaType<Help::Internal::LocalHelpManager::HelpData>("Help::Internal::LocalHelpManager::HelpData");
    m_filterModel = new QStandardItemModel(this);
}

LocalHelpManager::~LocalHelpManager()
{
    if (m_bookmarkManager) {
        m_bookmarkManager->saveBookmarks();
        delete m_bookmarkManager;
        m_bookmarkManager = 0;
    }

    delete m_guiEngine;
    m_guiEngine = 0;
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
    return Core::ICore::settings()->value(QLatin1String(kHelpHomePageKey),
                                          defaultHomePage()).toString();
}

void LocalHelpManager::setHomePage(const QString &page)
{
    Core::ICore::settings()->setValue(QLatin1String(kHelpHomePageKey), page);
}

QFont LocalHelpManager::fallbackFont()
{
    const QVariant value = getSettingWithFallback(QLatin1String(kFontKey),
                                                  QLatin1String("font"), QVariant());
    return value.value<QFont>();
}

void LocalHelpManager::setFallbackFont(const QFont &font)
{
    Core::ICore::settings()->setValue(QLatin1String(kFontKey), font);
    emit m_instance->fallbackFontChanged(font);
}

LocalHelpManager::StartOption LocalHelpManager::startOption()
{
    const QVariant value = getSettingWithFallback(QLatin1String(kStartOptionKey),
                                                  QLatin1String("StartOption"), ShowLastPages);
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
    Core::ICore::settings()->setValue(QLatin1String(kStartOptionKey), option);
}

Core::HelpManager::HelpViewerLocation LocalHelpManager::contextHelpOption()
{
    const QVariant value = getSettingWithFallback(QLatin1String(kContextHelpOptionKey),
                                                  QLatin1String("ContextHelpOption"),
                                                  Core::HelpManager::SideBySideIfPossible);
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
    Core::ICore::settings()->setValue(QLatin1String(kContextHelpOptionKey), location);
}

bool LocalHelpManager::returnOnClose()
{
    const QVariant value = getSettingWithFallback(QLatin1String(kReturnOnCloseKey),
                                                  QLatin1String("ReturnOnClose"), false);
    return value.toBool();
}

void LocalHelpManager::setReturnOnClose(bool returnOnClose)
{
    Core::ICore::settings()->setValue(QLatin1String(kReturnOnCloseKey), returnOnClose);
    emit m_instance->returnOnCloseChanged();
}

QStringList LocalHelpManager::lastShownPages()
{
    const QVariant value = getSettingWithFallback(QLatin1String(kLastShownPagesKey),
                                                  QLatin1String("LastShownPages"), QVariant());
    return value.toString().split(Constants::ListSeparator, QString::SkipEmptyParts);
}

void LocalHelpManager::setLastShownPages(const QStringList &pages)
{
    Core::ICore::settings()->setValue(QLatin1String(kLastShownPagesKey),
                                      pages.join(Constants::ListSeparator));
}

QList<float> LocalHelpManager::lastShownPagesZoom()
{
    const QVariant value = getSettingWithFallback(QLatin1String(kLastShownPagesZoomKey),
                                                  QLatin1String("LastShownPagesZoom"), QVariant());
    const QStringList stringValues = value.toString().split(Constants::ListSeparator,
                                                            QString::SkipEmptyParts);
    return Utils::transform(stringValues, [](const QString &str) { return str.toFloat(); });
}

void LocalHelpManager::setLastShownPagesZoom(const QList<float> &zoom)
{
    const QStringList stringValues = Utils::transform(zoom,
                                                      [](float z) { return QString::number(z); });
    Core::ICore::settings()->setValue(QLatin1String(kLastShownPagesZoomKey),
                                      stringValues.join(Constants::ListSeparator));
}

int LocalHelpManager::lastSelectedTab()
{
    const QVariant value = getSettingWithFallback(QLatin1String(kLastSelectedTabKey),
                                                  QLatin1String("LastTabPage"), 0);
    return value.toInt();
}

void LocalHelpManager::setLastSelectedTab(int index)
{
    Core::ICore::settings()->setValue(QLatin1String(kLastSelectedTabKey), index);
}

void LocalHelpManager::setupGuiHelpEngine()
{
    if (m_needsCollectionFile) {
        m_needsCollectionFile = false;
        helpEngine().setCollectionFile(Core::HelpManager::collectionFilePath());
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
            m_guiEngine->setAutoSaveFilter(false);
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

/*!
 * Checks if the string does contain a scheme, and if that scheme is a "sensible" scheme for
 * opening in a internal or external browser (qthelp, about, file, http, https).
 * This is necessary to avoid trying to open e.g. "Foo::bar" in a external browser.
 */
bool LocalHelpManager::isValidUrl(const QString &link)
{
    QUrl url(link);
    if (!url.isValid())
        return false;
    const QString scheme = url.scheme();
    return (scheme == QLatin1String("qthelp")
            || scheme == QLatin1String("about")
            || scheme == QLatin1String("file")
            || scheme == QLatin1String("http")
            || scheme == QLatin1String("https"));
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
            data.mimeType = QLatin1String("application/octet-stream");
    } else {
        data.data = loadErrorMessage(url, QCoreApplication::translate(
                                         "Help", "The page could not be found"));
        data.mimeType = QLatin1String("text/html");
    }
    return data;
}

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
