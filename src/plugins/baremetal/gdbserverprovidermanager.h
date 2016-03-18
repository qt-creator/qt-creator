/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include <QObject>
#include <QList>

#include <utils/fileutils.h>

namespace Utils { class PersistentSettingsWriter; }

namespace BareMetal {
namespace Internal {

class BareMetalPlugin;
class GdbServerProvider;
class GdbServerProviderFactory;

class GdbServerProviderManager : public QObject
{
    Q_OBJECT

public:
    static GdbServerProviderManager *instance();
    ~GdbServerProviderManager();

    QList<GdbServerProvider *> providers() const;
    QList<GdbServerProviderFactory *> factories() const;
    GdbServerProvider *findProvider(const QString &id) const;
    GdbServerProvider *findByDisplayName(const QString &displayName) const;
    bool registerProvider(GdbServerProvider *);
    void deregisterProvider(GdbServerProvider *);

signals:
    void providerAdded(GdbServerProvider *);
    void providerRemoved(GdbServerProvider *);
    void providerUpdated(GdbServerProvider *);
    void providersChanged();
    void providersLoaded();

private:
    void saveProviders();
    explicit GdbServerProviderManager(QObject *parent = 0);

    void restoreProviders();
    void notifyAboutUpdate(GdbServerProvider *);

    Utils::PersistentSettingsWriter *m_writer;
    QList<GdbServerProvider *> m_providers;
    const Utils::FileName m_configFile;
    const QList<GdbServerProviderFactory *> m_factories;

    friend class BareMetalPlugin; // for constructor
    friend class GdbServerProvider;
};

} // namespace Internal
} // namespace BareMetal
