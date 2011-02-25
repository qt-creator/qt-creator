/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggertoolchaincombobox.h"

#include <projectexplorer/toolchainmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QPair>

#include <QtGui/QtEvents>

typedef QPair<ProjectExplorer::Abi, QString> AbiDebuggerCommandPair;

Q_DECLARE_METATYPE(AbiDebuggerCommandPair)

namespace Debugger {
namespace Internal {

DebuggerToolChainComboBox::DebuggerToolChainComboBox(QWidget *parent) :
    QComboBox(parent)
{
}

void DebuggerToolChainComboBox::init(bool hostAbiOnly)
{
    const ProjectExplorer::Abi hostAbi = ProjectExplorer::Abi::hostAbi();
    foreach (const ProjectExplorer::ToolChain *tc, ProjectExplorer::ToolChainManager::instance()->toolChains()) {
        const ProjectExplorer::Abi abi = tc->targetAbi();
        if (!hostAbiOnly || hostAbi.os() == abi.os()) { // Offer MSVC and Mingw, etc.
            const QString debuggerCommand = tc->debuggerCommand();
            if (!debuggerCommand.isEmpty()) {
                const AbiDebuggerCommandPair data(abi, debuggerCommand);
                const QString completeBase = QFileInfo(debuggerCommand).completeBaseName();
                const QString name = tr("%1 (%2)").arg(tc->displayName(), completeBase);
                addItem(name, qVariantFromValue(data));
            }
        }
    }
    setEnabled(count() > 1);
}

void DebuggerToolChainComboBox::setAbi(const ProjectExplorer::Abi &abi)
{
    QTC_ASSERT(abi.isValid(), return; )
    const int c = count();
    for (int i = 0; i < c; i++) {
        if (abiAt(i) == abi) {
            setCurrentIndex(i);
            break;
        }
    }
}

ProjectExplorer::Abi DebuggerToolChainComboBox::abi() const
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

ProjectExplorer::Abi DebuggerToolChainComboBox::abiAt(int index) const
{
    if (index >= 0 && index < count()) {
        const AbiDebuggerCommandPair abiCommandPair = qvariant_cast<AbiDebuggerCommandPair>(itemData(index));
        return abiCommandPair.first;
    }
    return ProjectExplorer::Abi();
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
