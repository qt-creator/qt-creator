/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
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

#ifndef MACROSPLUGIN_ACTIONEVENTHANDLER_H
#define MACROSPLUGIN_ACTIONEVENTHANDLER_H

#include "imacrohandler.h"

#include <QtCore/QSet>

QT_BEGIN_NAMESPACE
class QAction;
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

private:
    void registerCommand(const QString &id);

private slots:
    void addCommand(const QString &id);
    void addActionEvent(const QString &id);

private:
    QSet<QString> m_commandIds;
    QSignalMapper *m_mapper;
};

} // namespace Internal
} // namespace Macros

#endif // MACROSPLUGIN_ACTIONEVENTHANDLER_H
