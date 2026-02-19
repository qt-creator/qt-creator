// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/helpmanager.h>
#include <utils/aspects.h>

#include <QMetaType>
#include <QMutex>
#include <QObject>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QHelpFilterEngine;
class QHelpEngine;
QT_END_NAMESPACE

class BookmarkManager;

namespace Help::Internal {

class HelpViewer;

struct HelpViewerFactory
{
    QByteArray id;
    QString displayName;
    std::function<HelpViewer *()> create;
};

class ViewerBackendAspect final : public Utils::SelectionAspect
{
public:
    using Utils::SelectionAspect::SelectionAspect;

    QVariant fromSettingsValue(const QVariant &savedValue) const final;
    QVariant toSettingsValue(const QVariant &valueToSave) const final;

    QByteArray operator()() const;
};

class HelpSettings final : public Utils::AspectContainer
{
public:
    HelpSettings();

    enum StartOption {
        ShowHomePage = 0,
        ShowBlankPage = 1,
        ShowLastPages = 2,
    };

    Utils::StringAspect homePage{this};
    Utils::IntegerAspect fontZoom{this}; // percentages
    Utils::BoolAspect antiAlias{this};
    Utils::BoolAspect scrollWheelZooming{this};
    Utils::BoolAspect returnOnClose{this};
    Utils::StringAspect lastShownPages{this};
    Utils::IntegerAspect lastSelectedTab{this};
    ViewerBackendAspect viewerBackend{this};
    Utils::TypedSelectionAspect<StartOption> startOption{this};
    Utils::TypedSelectionAspect<Core::HelpManager::HelpViewerLocation> contextHelpOption{this};
    Utils::FontAspect fallbackFont{this};
};

HelpSettings &helpSettings();

class LocalHelpManager : public QObject
{
    Q_OBJECT

public:
    struct HelpData {
        QUrl resolvedUrl;
        QByteArray data;
        QString mimeType;
    };

    LocalHelpManager(QObject *parent = nullptr);
    ~LocalHelpManager() override;

    static LocalHelpManager *instance();

    static HelpViewerFactory defaultViewerBackend();
    static QVector<HelpViewerFactory> viewerBackends();
    static HelpViewerFactory viewerBackend();

    static void setupGuiHelpEngine();
    static void setEngineNeedsUpdate();

    static QHelpEngine& helpEngine();
    static BookmarkManager& bookmarkManager();

    static QByteArray loadErrorMessage(const QUrl &url, const QString &errorString);
    Q_INVOKABLE static Help::Internal::LocalHelpManager::HelpData helpData(const QUrl &url);

    static QHelpFilterEngine *filterEngine();

    static bool canOpenOnlineHelp(const QUrl &url);
    static bool openOnlineHelp(const QUrl &url);
    static bool isQtUrl(const QUrl &url);
    static void openQtUrl(const QUrl &url);

    static QMultiMap<QString, QUrl> linksForKeyword(const QString &keyword);

    static void addOnlineHelpHandler(const Core::HelpManager::OnlineHelpHandler &handler);

signals:
    void fallbackFontChanged(const QFont &font);
};

} // namespace Help::Internal

Q_DECLARE_METATYPE(Help::Internal::LocalHelpManager::HelpData)
