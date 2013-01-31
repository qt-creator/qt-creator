/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "toolchain.h"

#include "abi.h"
#include "toolchainmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/environment.h>

#include <QCoreApplication>
#include <QUuid>

static const char ID_KEY[] = "ProjectExplorer.ToolChain.Id";
static const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ToolChain.DisplayName";
static const char AUTODETECT_KEY[] = "ProjectExplorer.ToolChain.Autodetect";

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// ToolChainPrivate
// --------------------------------------------------------------------------

class ToolChainPrivate
{
public:
    ToolChainPrivate(const QString &id, bool autodetect) :
        m_autodetect(autodetect)
    {
        m_id = createId(id);
    }

    static QString createId(const QString &id)
    {
        QString newId = id.left(id.indexOf(QLatin1Char(':')));
        newId.append(QLatin1Char(':') + QUuid::createUuid().toString());
        return newId;
    }

    QString m_id;
    bool m_autodetect;
    mutable QString m_displayName;
};

} // namespace Internal

/*!
    \class ProjectExplorer::ToolChain
    \brief Representation of a ToolChain.
    \sa ProjectExplorer::ToolChainManager
*/

// --------------------------------------------------------------------------

ToolChain::ToolChain(const QString &id, bool autodetect) :
    d(new Internal::ToolChainPrivate(id, autodetect))
{ }

ToolChain::ToolChain(const ToolChain &other) :
    d(new Internal::ToolChainPrivate(other.d->m_id, false))
{
    // leave the autodetection bit at false.
    d->m_displayName = QCoreApplication::translate("ProjectExplorer::ToolChain", "Clone of %1")
            .arg(other.displayName());
}

ToolChain::~ToolChain()
{
    delete d;
}

QString ToolChain::displayName() const
{
    if (d->m_displayName.isEmpty())
        return typeDisplayName();
    return d->m_displayName;
}

void ToolChain::setDisplayName(const QString &name)
{
    if (d->m_displayName == name)
        return;

    d->m_displayName = name;
    toolChainUpdated();
}

bool ToolChain::isAutoDetected() const
{
    return d->m_autodetect;
}

QString ToolChain::id() const
{
    return d->m_id;
}

Utils::FileName ToolChain::suggestedDebugger() const
{
    return ToolChainManager::instance()->defaultDebugger(targetAbi());
}

bool ToolChain::canClone() const
{
    return true;
}

bool ToolChain::operator == (const ToolChain &tc) const
{
    if (this == &tc)
        return true;

    const QString thisId = id().left(id().indexOf(QLatin1Char(':')));
    const QString tcId = tc.id().left(tc.id().indexOf(QLatin1Char(':')));

    // We ignore displayname
    return thisId == tcId && isAutoDetected() == tc.isAutoDetected();
}

/*!
    \brief Used by the toolchainmanager to save user-generated tool chains.

    Make sure to call this method when deriving!
*/

QVariantMap ToolChain::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(ID_KEY), id());
    result.insert(QLatin1String(DISPLAY_NAME_KEY), displayName());
    result.insert(QLatin1String(AUTODETECT_KEY), isAutoDetected());

    return result;
}

void ToolChain::toolChainUpdated()
{
    ToolChainManager::instance()->notifyAboutUpdate(this);
}

void ToolChain::setAutoDetected(bool autodetect)
{
    if (d->m_autodetect == autodetect)
        return;
    d->m_autodetect = autodetect;
    toolChainUpdated();
}

/*!
    \brief Used by the toolchainmanager to load user-generated tool chains.

    Make sure to call this method when deriving!
*/

bool ToolChain::fromMap(const QVariantMap &data)
{
    d->m_displayName = data.value(QLatin1String(DISPLAY_NAME_KEY)).toString();
    // make sure we have new style ids:
    d->m_id = data.value(QLatin1String(ID_KEY)).toString();
    d->m_autodetect = data.value(QLatin1String(AUTODETECT_KEY), false).toBool();

    return true;
}

/*!
    \class ProjectExplorer::ToolChainFactory
    \brief Creates toolchains from settings or autodetects them.
*/

/*!
    \fn QString ProjectExplorer::ToolChainFactory::displayName() const = 0
    \brief Name used to display the name of the tool chain that will be created.
*/

/*!
    \fn bool ProjectExplorer::ToolChainFactory::canRestore(const QVariantMap &data)
    \brief Used by the ToolChainManager to restore user-generated tool chains.
*/

QList<ToolChain *> ToolChainFactory::autoDetect()
{
    return QList<ToolChain *>();
}

bool ToolChainFactory::canCreate()
{
    return false;
}

ToolChain *ToolChainFactory::create()
{
    return 0;
}

bool ToolChainFactory::canRestore(const QVariantMap &)
{
    return false;
}

ToolChain *ToolChainFactory::restore(const QVariantMap &)
{
    return 0;
}

QString ToolChainFactory::idFromMap(const QVariantMap &data)
{
    return data.value(QLatin1String(ID_KEY)).toString();
}

void ToolChainFactory::idToMap(QVariantMap &data, const QString id)
{
    data.insert(QLatin1String(ID_KEY), id);
}

void ToolChainFactory::autoDetectionToMap(QVariantMap &data, bool detected)
{
    data.insert(QLatin1String(AUTODETECT_KEY), detected);
}

} // namespace ProjectExplorer
