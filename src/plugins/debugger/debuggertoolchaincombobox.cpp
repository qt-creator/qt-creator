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

#include <QtGui/QtEvents>

Q_DECLARE_METATYPE(ProjectExplorer::Abi)

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
        if (!hostAbiOnly || hostAbi.isCompatibleWith(tc->targetAbi())) {
            const QString debuggerCommand = tc->debuggerCommand();
            if (!debuggerCommand.isEmpty()) {
                const QString name = tr("%1 (%2)").arg(tc->displayName(), QFileInfo(debuggerCommand).baseName());
                addItem(name, qVariantFromValue(tc->targetAbi()));
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

ProjectExplorer::Abi DebuggerToolChainComboBox::abiAt(int index) const
{
    return index >= 0 ? qvariant_cast<ProjectExplorer::Abi>(itemData(index)) :
                        ProjectExplorer::Abi();
}

bool DebuggerToolChainComboBox::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        const ProjectExplorer::Abi current = abi();
        setToolTip(current.isValid() ? current.toString() : QString());
    }
    return QComboBox::event(event);
}

} // namespace Debugger
} // namespace Internal
