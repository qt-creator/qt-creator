/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GDBSERVERPROVIDERMANAGER_H
#define GDBSERVERPROVIDERMANAGER_H

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

#endif // GDBSERVERPROVIDERMANAGER_H
