/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggertoolchaincombobox.h"

#include "debuggerprofileinformation.h"

#include <projectexplorer/profileinformation.h>
#include <projectexplorer/profilemanager.h>
#include <projectexplorer/abi.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QDir>
#include <QPair>
#include <QtEvents>

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

DebuggerToolChainComboBox::DebuggerToolChainComboBox(QWidget *parent) :
    QComboBox(parent)
{
}

void DebuggerToolChainComboBox::init(bool hostAbiOnly)
{
    const Abi hostAbi = Abi::hostAbi();
    foreach (const Profile *st, ProfileManager::instance()->profiles()) {
        if (!st->isValid())
            continue;
        ToolChain *tc = ToolChainProfileInformation::toolChain(st);
        if (!tc)
            continue;
        const Abi abi = tc->targetAbi();
        if (hostAbiOnly && hostAbi.os() != abi.os())
            continue;

        const QString debuggerCommand = DebuggerProfileInformation::debuggerCommand(st).toString();
        if (debuggerCommand.isEmpty())
            continue;

        const QString completeBase = QFileInfo(debuggerCommand).completeBaseName();
        const QString name = tr("%1 (%2)").arg(st->displayName(), completeBase);
        addItem(name, qVariantFromValue(st->id()));
        QString debugger = QDir::toNativeSeparators(debuggerCommand);
        debugger.replace(QString(QLatin1Char(' ')), QLatin1String("&nbsp;"));
        QString toolTip = tr("<html><head/><body><table>"
            "<tr><td>ABI:</td><td><i>%1</i></td></tr>"
            "<tr><td>Debugger:</td><td>%2</td></tr>")
                .arg(st->displayName(), QDir::toNativeSeparators(debugger));
        setItemData(count() - 1, toolTip, Qt::ToolTipRole);
    }
    setEnabled(count() > 1);
}

void DebuggerToolChainComboBox::setCurrentProfile(const Profile *profile)
{
    QTC_ASSERT(profile->isValid(), return);
    for (int i = 0, n = count(); i != n; ++i) {
        if (profileAt(i) == profile) {
            setCurrentIndex(i);
            break;
        }
    }
}

Profile *DebuggerToolChainComboBox::currentProfile() const
{
    return profileAt(currentIndex());
}

void DebuggerToolChainComboBox::setCurrentProfileId(Core::Id id)
{
    for (int i = 0, n = count(); i != n; ++i) {
        if (profileAt(i)->id() == id) {
            setCurrentIndex(i);
            break;
        }
    }
}

Core::Id DebuggerToolChainComboBox::currentProfileId() const
{
    return profileAt(currentIndex())->id();
}

Profile *DebuggerToolChainComboBox::profileAt(int index) const
{
    Core::Id id = qvariant_cast<Core::Id>(itemData(index));
    return ProfileManager::instance()->find(id);
}

} // namespace Debugger
} // namespace Internal
