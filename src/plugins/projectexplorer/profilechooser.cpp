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

#include <QFileInfo>
#include <QDir>
#include <QPair>
#include <QtEvents>

namespace ProjectExplorer {

ProfileChooser::ProfileChooser(QWidget *parent, bool hostAbiOnly) :
    QComboBox(parent)
{
    const Abi hostAbi = Abi::hostAbi();
    foreach (const Profile *profile, ProfileManager::instance()->profiles()) {
        if (!profile->isValid())
            continue;
        ToolChain *tc = ToolChainProfileInformation::toolChain(profile);
        if (!tc)
            continue;
        const Abi abi = tc->targetAbi();
        if (hostAbiOnly && hostAbi.os() != abi.os())
            continue;

        const QString debuggerCommand = profile->value(Core::Id("Debugger.Information")).toString();
        const QString completeBase = QFileInfo(debuggerCommand).completeBaseName();
        const QString name = tr("%1 (%2)").arg(profile->displayName(), completeBase);
        addItem(name, qVariantFromValue(profile->id()));
        QString debugger = QDir::toNativeSeparators(debuggerCommand);
        debugger.replace(QString(QLatin1Char(' ')), QLatin1String("&nbsp;"));
        QString toolTip = tr("<html><head/><body><table>"
            "<tr><td>ABI:</td><td><i>%1</i></td></tr>"
            "<tr><td>Debugger:</td><td>%2</td></tr>")
                .arg(profile->displayName(), QDir::toNativeSeparators(debugger));
        setItemData(count() - 1, toolTip, Qt::ToolTipRole);
    }
    setEnabled(count() > 1);
}

Profile *ProfileChooser::currentProfile() const
{
    return profileAt(currentIndex());
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
    return profileAt(currentIndex())->id();
}

Profile *ProfileChooser::profileAt(int index) const
{
    Core::Id id = qvariant_cast<Core::Id>(itemData(index));
    return ProfileManager::instance()->find(id);
}

} // namespace ProjectExplorer
