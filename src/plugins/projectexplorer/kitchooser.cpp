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

#include "kitchooser.h"

#include "kitinformation.h"
#include "kitmanager.h"
#include "abi.h"

#include <utils/qtcassert.h>

namespace ProjectExplorer {

KitChooser::KitChooser(QWidget *parent, unsigned flags) :
    QComboBox(parent)
{
    populate(flags);
    onCurrentIndexChanged(currentIndex());
    connect(this, SIGNAL(currentIndexChanged(int)), SLOT(onCurrentIndexChanged(int)));
}

void KitChooser::onCurrentIndexChanged(int index)
{
    if (Kit *kit = kitAt(index))
        setToolTip(kit->toHtml());
    else
        setToolTip(QString());
}

void KitChooser::populate(unsigned flags)
{
    clear();
    const Abi hostAbi = Abi::hostAbi();
    foreach (Kit *kit, KitManager::instance()->kits()) {
        if (!kit->isValid() && !(flags & IncludeInvalidKits))
            continue;
        ToolChain *tc = ToolChainKitInformation::toolChain(kit);
        if (!tc)
            continue;
        const Abi abi = tc->targetAbi();
        if ((flags & HostAbiOnly) && hostAbi.os() != abi.os())
            continue;
        const QString debuggerCommand = kit->value(Core::Id("Debugger.Information")).toString();
        if ((flags & HasDebugger) && debuggerCommand.isEmpty())
            continue;
        const QString completeBase = QFileInfo(debuggerCommand).completeBaseName();
        const QString name = tr("%1 (%2)").arg(kit->displayName(), completeBase);
        addItem(name, qVariantFromValue(kit->id()));
        setItemData(count() - 1, kit->toHtml(), Qt::ToolTipRole);
    }
    setEnabled(count() > 1);
}

Kit *KitChooser::currentKit() const
{
    const int index = currentIndex();
    return index == -1 ? 0 : kitAt(index);
}

void KitChooser::setCurrentKitId(Core::Id id)
{
    for (int i = 0, n = count(); i != n; ++i) {
        if (kitAt(i)->id() == id) {
            setCurrentIndex(i);
            break;
        }
    }
}

Core::Id KitChooser::currentKitId() const
{
    Kit *kit = currentKit();
    return kit ? kit->id() : Core::Id();
}

Kit *KitChooser::kitAt(int index) const
{
    Core::Id id = qvariant_cast<Core::Id>(itemData(index));
    return KitManager::instance()->find(id);
}

} // namespace ProjectExplorer
