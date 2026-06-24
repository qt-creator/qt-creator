// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "helpmanager.h"

#include <QtCore/QList>

namespace Core {

struct HelpLink;

namespace HelpManager {

class CORE_EXPORT Implementation
{
protected:
    Implementation();
    virtual ~Implementation();

public:
    virtual void registerDocumentation(const Utils::FilePaths &fileNames) = 0;
    virtual void setBlockedDocumentation(const Utils::FilePaths &fileNames) = 0;
    virtual void unregisterDocumentation(const Utils::FilePaths &fileNames) = 0;
    virtual QList<HelpLink> linksForIdentifier(const QString &id) = 0;
    virtual QList<HelpLink> linksForKeyword(const QString &keyword) = 0;
    virtual QByteArray fileData(const QUrl &url) = 0;
    virtual void showHelpUrl(const QUrl &url, HelpViewerLocation location = HelpModeAlways) = 0;
    virtual void addOnlineHelpHandler(const OnlineHelpHandler &handler) = 0;
};

} // HelpManager
} // Core
