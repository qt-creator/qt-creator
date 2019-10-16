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
