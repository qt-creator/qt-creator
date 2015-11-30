/**************************************************************************
**
** Copyright (C) 2015 Nicolas Arnaud-Cormos
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ACTIOMACROHANDLER_H
#define ACTIOMACROHANDLER_H

#include "imacrohandler.h"

#include <coreplugin/id.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <QSet>

QT_BEGIN_NAMESPACE
class QSignalMapper;
QT_END_NAMESPACE

namespace Macros {
namespace Internal {

class ActionMacroHandler : public IMacroHandler
{
    Q_OBJECT

public:
    ActionMacroHandler();

    bool canExecuteEvent(const MacroEvent &macroEvent);
    bool executeEvent(const MacroEvent &macroEvent);

private slots:
    void addActionEvent(const QString &id);

private:
    void registerCommand(Core::Id id);
    Core::Command *command(const QString &id);
    void addCommand(Core::Id id);

private:
    QSet<Core::Id> m_commandIds;
    QSignalMapper *m_mapper;
};

} // namespace Internal
} // namespace Macros

#endif // ACTIOMACROHANDLER_H
