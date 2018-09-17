/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <coreplugin/dialogs/ioptionspage.h>

#include <QAbstractItemModel>
#include <QPointer>
#include <QWidget>

namespace LanguageClient {

constexpr char noLanguageFilter[] = "No Filter";

class BaseClient;

class LanguageClientSettings
{
public:
    static void init();
};

class BaseSettings
{
public:
    BaseSettings() = default;
    BaseSettings(const QString &name, bool enabled, const QString &mimeTypeName,
                 const QString &executable, const QString &arguments)
        : m_name(name)
        , m_enabled(enabled)
        , m_mimeType(mimeTypeName)
        , m_executable(executable)
        , m_arguments(arguments)
    {}
    QString m_name = QString("New Language Server");
    bool m_enabled = true;
    QString m_mimeType = QLatin1String(noLanguageFilter);
    QString m_executable;
    QString m_arguments;

    bool isValid();

    BaseClient *createClient();

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);
};

} // namespace LanguageClient
