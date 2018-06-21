/****************************************************************************
**
** Copyright (C) 2016 Nicolas Arnaud-Cormos
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QObject>
#include <QMap>

namespace Macros {
namespace Internal {

class IMacroHandler;
class Macro;
class MacroOptionsWidget;
class MacrosPlugin;

class MacroManager : public QObject
{
    Q_OBJECT
public:
    static MacroManager *instance();

    static const QMap<QString, Macro *> &macros();

    static void registerMacroHandler(IMacroHandler *handler);

    static QString macrosDirectory();

    void startMacro();
    void executeLastMacro();
    void saveLastMacro();
    bool executeMacro(const QString &name);
    void endMacro();

protected:
    friend class Internal::MacroOptionsWidget;

    void deleteMacro(const QString &name);
    void changeMacro(const QString &name, const QString &description);

private:
    explicit MacroManager(QObject *parent = 0);
    ~MacroManager();

    static MacroManager *m_instance;

    class MacroManagerPrivate;
    MacroManagerPrivate* d;

    friend class Internal::MacrosPlugin;
};

} // namespace Internal
} // namespace Macros
