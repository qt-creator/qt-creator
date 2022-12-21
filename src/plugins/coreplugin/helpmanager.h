// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QObject>
#include <QMap>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Core {

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

CORE_EXPORT QString documentationPath();

CORE_EXPORT void registerDocumentation(const QStringList &fileNames);
CORE_EXPORT void unregisterDocumentation(const QStringList &fileNames);

CORE_EXPORT QMultiMap<QString, QUrl> linksForIdentifier(const QString &id);
CORE_EXPORT QMultiMap<QString, QUrl> linksForKeyword(const QString &id);
CORE_EXPORT QByteArray fileData(const QUrl &url);

CORE_EXPORT void showHelpUrl(const QUrl &url, HelpViewerLocation location = HelpModeAlways);
CORE_EXPORT void showHelpUrl(const QString &url, HelpViewerLocation location = HelpModeAlways);

} // HelpManager
} // Core
