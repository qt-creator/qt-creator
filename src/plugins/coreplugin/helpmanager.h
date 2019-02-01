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

#include "core_global.h"

#include <QObject>
#include <QMap>

QT_BEGIN_NAMESPACE
class QStringList;
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
CORE_EXPORT void unregisterDocumentation(const QStringList &nameSpaces);

CORE_EXPORT QMap<QString, QUrl> linksForIdentifier(const QString &id);
CORE_EXPORT QMap<QString, QUrl> linksForKeyword(const QString &id);
CORE_EXPORT QByteArray fileData(const QUrl &url);

CORE_EXPORT void showHelpUrl(const QUrl &url, HelpViewerLocation location = HelpModeAlways);
CORE_EXPORT void showHelpUrl(const QString &url, HelpViewerLocation location = HelpModeAlways);

} // HelpManager
} // Core
