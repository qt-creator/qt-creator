/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "toolchain.h"

#include "toolchainmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/environment.h>

#include <QtCore/QCoreApplication>

static const char ID_KEY[] = "ProjectExplorer.ToolChain.Id";
static const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ToolChain.DisplayName";

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// ToolChainPrivate
// --------------------------------------------------------------------------

class ToolChainPrivate
{
public:
    ToolChainPrivate(const QString &id, bool autodetect) :
        m_id(id),
        m_autodetect(autodetect)
    { Q_ASSERT(!id.isEmpty()); }

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
    d(new Internal::ToolChainPrivate(other.id(), false))
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
        return typeName();
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

/*!
    \brief Returns a list of target ids that this tool chain is restricted to.

    An empty list is shows that the toolchain is compatible with all targets.
*/

QStringList ToolChain::restrictedToTargets() const
{
    return QStringList();
}

bool ToolChain::canClone() const
{
    return true;
}

QString ToolChain::defaultMakeTarget() const
{
    return QString();
}

bool ToolChain::operator == (const ToolChain &tc) const
{
    if (this == &tc)
        return true;

    return id() == tc.id();
}

/*!
    \brief Used by the toolchainmanager to save user-generated tool chains.

    Make sure to call this method when deriving!
*/

QVariantMap ToolChain::toMap() const
{
    QVariantMap result;
    if (isAutoDetected())
        return result;

    result.insert(QLatin1String(ID_KEY), id());
    result.insert(QLatin1String(DISPLAY_NAME_KEY), displayName());

    return result;
}

void ToolChain::setId(const QString &id)
{
    Q_ASSERT(!id.isEmpty());
    if (d->m_id == id)
        return;

    d->m_id = id;
    toolChainUpdated();
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
    Q_ASSERT(!isAutoDetected());
    // do not read the id: That is already set anyway.
    d->m_displayName = data.value(QLatin1String(DISPLAY_NAME_KEY)).toString();

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

} // namespace ProjectExplorer
