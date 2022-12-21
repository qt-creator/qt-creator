// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "helpmanager.h"

namespace Core {

namespace HelpManager {

class CORE_EXPORT Implementation
{
protected:
    Implementation();
    virtual ~Implementation();

public:
    virtual void registerDocumentation(const QStringList &fileNames) = 0;
    virtual void unregisterDocumentation(const QStringList &fileNames) = 0;
    virtual QMultiMap<QString, QUrl> linksForIdentifier(const QString &id) = 0;
    virtual QMultiMap<QString, QUrl> linksForKeyword(const QString &keyword) = 0;
    virtual QByteArray fileData(const QUrl &url) = 0;
    virtual void showHelpUrl(const QUrl &url, HelpViewerLocation location = HelpModeAlways) = 0;
};

} // HelpManager
} // Core
