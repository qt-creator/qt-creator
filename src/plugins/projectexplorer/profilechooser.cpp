/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "profilechooser.h"

#include "profileinformation.h"
#include "profilemanager.h"
#include "abi.h"

#include <utils/qtcassert.h>

namespace ProjectExplorer {

ProfileChooser::ProfileChooser(QWidget *parent, unsigned flags) :
    QComboBox(parent)
{
    populate(flags);
    onCurrentIndexChanged(currentIndex());
    connect(this, SIGNAL(currentIndexChanged(int)), SLOT(onCurrentIndexChanged(int)));
}

void ProfileChooser::onCurrentIndexChanged(int index)
{
    if (Profile *profile = profileAt(index))
        setToolTip(profile->toHtml());
    else
        setToolTip(QString());
}

void ProfileChooser::populate(unsigned flags)
{
    clear();
    const Abi hostAbi = Abi::hostAbi();
    foreach (Profile *profile, ProfileManager::instance()->profiles()) {
        if (!profile->isValid() && !(flags & IncludeInvalidProfiles))
            continue;
        ToolChain *tc = ToolChainProfileInformation::toolChain(profile);
        if (!tc)
            continue;
        const Abi abi = tc->targetAbi();
        if ((flags & HostAbiOnly) && hostAbi.os() != abi.os())
            continue;
        const QString debuggerCommand = profile->value(Core::Id("Debugger.Information")).toString();
        if ((flags & HasDebugger) && debuggerCommand.isEmpty())
            continue;
        const QString completeBase = QFileInfo(debuggerCommand).completeBaseName();
        const QString name = tr("%1 (%2)").arg(profile->displayName(), completeBase);
        addItem(name, qVariantFromValue(profile->id()));
        setItemData(count() - 1, profile->toHtml(), Qt::ToolTipRole);
    }
    setEnabled(count() > 1);
}

Profile *ProfileChooser::currentProfile() const
{
    const int index = currentIndex();
    return index == -1 ? 0 : profileAt(index);
}

void ProfileChooser::setCurrentProfileId(Core::Id id)
{
    for (int i = 0, n = count(); i != n; ++i) {
        if (profileAt(i)->id() == id) {
            setCurrentIndex(i);
            break;
        }
    }
}

Core::Id ProfileChooser::currentProfileId() const
{
    Profile *profile = currentProfile();
    return profile ? profile->id() : Core::Id();
}

Profile *ProfileChooser::profileAt(int index) const
{
    Core::Id id = qvariant_cast<Core::Id>(itemData(index));
    return ProfileManager::instance()->find(id);
}

} // namespace ProjectExplorer
