// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>

#include <QObject>
#include <QList>
#include <QUrl>

namespace Core {

struct HelpLink;

namespace HelpManager {

class CORE_EXPORT Signals : public QObject
{
    Q_OBJECT

public:
    static Signals *instance();

signals:
    void setupFinished();
    void documentationChanged();
};

enum HelpViewerLocation {
    SideBySideIfPossible = 0,
    SideBySideAlways = 1,
    HelpModeAlways = 2,
    ExternalHelpAlways = 3
};

CORE_EXPORT Utils::FilePath documentationPath();

CORE_EXPORT void registerDocumentation(const Utils::FilePaths &fileNames);
CORE_EXPORT void setBlockedDocumentation(const Utils::FilePaths &fileNames);
CORE_EXPORT void unregisterDocumentation(const Utils::FilePaths &fileNames);

CORE_EXPORT QList<Core::HelpLink> linksForIdentifier(const QString &id);
CORE_EXPORT QList<Core::HelpLink> linksForKeyword(const QString &id);
CORE_EXPORT QByteArray fileData(const QUrl &url);

CORE_EXPORT void showHelpUrl(const QUrl &url, HelpViewerLocation location = HelpModeAlways);
CORE_EXPORT void showHelpUrl(const QString &url, HelpViewerLocation location = HelpModeAlways);

struct CORE_EXPORT OnlineHelpHandler {
    std::function<bool(QUrl)> handlesUrl;
    std::function<void(QUrl)> openUrl;
};
CORE_EXPORT void addOnlineHelpHandler(const OnlineHelpHandler &handler);

} // HelpManager
} // Core
