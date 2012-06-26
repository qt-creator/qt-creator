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

typedef QPair<Abi, QString> AbiDebuggerCommandPair;

Q_DECLARE_METATYPE(AbiDebuggerCommandPair)

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

        const AbiDebuggerCommandPair data(abi, debuggerCommand);
        const QString completeBase = QFileInfo(debuggerCommand).completeBaseName();
        const QString name = tr("%1 (%2)").arg(st->displayName(), completeBase);
        addItem(name, qVariantFromValue(data));
    }
    setEnabled(count() > 1);
}

void DebuggerToolChainComboBox::setAbi(const Abi &abi)
{
    QTC_ASSERT(abi.isValid(), return);
    const int c = count();
    for (int i = 0; i < c; i++) {
        if (abiAt(i) == abi) {
            setCurrentIndex(i);
            break;
        }
    }
}

Abi DebuggerToolChainComboBox::abi() const
{
    return abiAt(currentIndex());
}

QString DebuggerToolChainComboBox::debuggerCommand() const
{
    return debuggerCommandAt(currentIndex());
}

QString DebuggerToolChainComboBox::debuggerCommandAt(int index) const
{
    if (index >= 0 && index < count()) {
        const AbiDebuggerCommandPair abiCommandPair = qvariant_cast<AbiDebuggerCommandPair>(itemData(index));
        return abiCommandPair.second;
    }
    return QString();
}

Abi DebuggerToolChainComboBox::abiAt(int index) const
{
    if (index >= 0 && index < count()) {
        const AbiDebuggerCommandPair abiCommandPair = qvariant_cast<AbiDebuggerCommandPair>(itemData(index));
        return abiCommandPair.first;
    }
    return Abi();
}

static inline QString abiToolTip(const AbiDebuggerCommandPair &abiCommandPair)
{
    QString debugger = QDir::toNativeSeparators(abiCommandPair.second);
    debugger.replace(QString(QLatin1Char(' ')), QLatin1String("&nbsp;"));
    return DebuggerToolChainComboBox::tr(
                "<html><head/><body><table><tr><td>ABI:</td><td><i>%1</i></td></tr>"
                "<tr><td>Debugger:</td><td>%2</td></tr>").
            arg(abiCommandPair.first.toString(),
                QDir::toNativeSeparators(debugger));
}

bool DebuggerToolChainComboBox::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        const int index = currentIndex();
        if (index >= 0) {
            const AbiDebuggerCommandPair abiCommandPair = qvariant_cast<AbiDebuggerCommandPair>(itemData(index));
            setToolTip(abiToolTip(abiCommandPair));
        } else {
            setToolTip(QString());
        }
    }
    return QComboBox::event(event);
}

} // namespace Debugger
} // namespace Internal
