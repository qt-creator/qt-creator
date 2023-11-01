// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/helpmanager.h>

#include <QMetaType>
#include <QMutex>
#include <QObject>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QHelpFilterEngine;
class QHelpEngine;
QT_END_NAMESPACE

class BookmarkManager;

namespace Help {
namespace Internal {

class HelpViewer;

struct HelpViewerFactory
{
    QByteArray id;
    QString displayName;
    std::function<HelpViewer *()> create;
};

class LocalHelpManager : public QObject
{
    Q_OBJECT

public:
    struct HelpData {
        QUrl resolvedUrl;
        QByteArray data;
        QString mimeType;
    };

    enum StartOption {
        ShowHomePage = 0,
        ShowBlankPage = 1,
        ShowLastPages = 2,
    };

    LocalHelpManager(QObject *parent = nullptr);
    ~LocalHelpManager() override;

    static LocalHelpManager *instance();

    static QString defaultHomePage();
    static QString homePage();
    static void setHomePage(const QString &page);

    static QFont fallbackFont();
    static void setFallbackFont(const QFont &font);

    static int fontZoom();
    static int setFontZoom(int percentage);

    static bool antialias();
    static void setAntialias(bool on);

    static StartOption startOption();
    static void setStartOption(StartOption option);

    static Core::HelpManager::HelpViewerLocation contextHelpOption();
    static void setContextHelpOption(Core::HelpManager::HelpViewerLocation location);

    static bool returnOnClose();
    static void setReturnOnClose(bool returnOnClose);

    static bool isScrollWheelZoomingEnabled();
    static void setScrollWheelZoomingEnabled(bool enabled);

    static QStringList lastShownPages();
    static void setLastShownPages(const QStringList &pages);

    static int lastSelectedTab();
    static void setLastSelectedTab(int index);

    static HelpViewerFactory defaultViewerBackend();
    static QVector<HelpViewerFactory> viewerBackends();
    static HelpViewerFactory viewerBackend();
    static void setViewerBackendId(const QByteArray &id);
    static QByteArray viewerBackendId();

    static void setupGuiHelpEngine();
    static void setEngineNeedsUpdate();

    static QHelpEngine& helpEngine();
    static BookmarkManager& bookmarkManager();

    static QByteArray loadErrorMessage(const QUrl &url, const QString &errorString);
    Q_INVOKABLE static Help::Internal::LocalHelpManager::HelpData helpData(const QUrl &url);

    static QHelpFilterEngine *filterEngine();

    static bool canOpenOnlineHelp(const QUrl &url);
    static bool openOnlineHelp(const QUrl &url);

    static QMultiMap<QString, QUrl> linksForKeyword(const QString &keyword);

signals:
    void fallbackFontChanged(const QFont &font);
    void fontZoomChanged(int percentage);
    void antialiasChanged(bool on);
    void returnOnCloseChanged();
    void scrollWheelZoomingEnabledChanged(bool enabled);
    void contextHelpOptionChanged(Core::HelpManager::HelpViewerLocation option);

private:
    static bool m_guiNeedsSetup;
    static bool m_needsCollectionFile;

    static QMutex m_guiMutex;
    static QHelpEngine *m_guiEngine;

    static QMutex m_bkmarkMutex;
    static BookmarkManager *m_bookmarkManager;
};

}   // Internal
}   // Help

Q_DECLARE_METATYPE(Help::Internal::LocalHelpManager::HelpData)
