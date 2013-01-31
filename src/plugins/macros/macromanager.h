/**************************************************************************
**
** Copyright (c) 2013 Nicolas Arnaud-Cormos
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MACROSPLUGIN_MACROMANAGER_H
#define MACROSPLUGIN_MACROMANAGER_H

#include "macros_global.h"

#include <QObject>
#include <QMap>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Macros {

class Macro;
class IMacroHandler;

namespace Internal {
    class MacroOptionsWidget;
}

class MACROS_EXPORT MacroManager : public QObject
{
    Q_OBJECT
public:
    explicit MacroManager(QObject *parent = 0);
    ~MacroManager();

    static MacroManager *instance();

    const QMap<QString, Macro *> &macros() const;

    void registerMacroHandler(IMacroHandler *handler);

    QString macrosDirectory() const;

public slots:
    void startMacro();
    void endMacro();
    void executeLastMacro();
    void saveLastMacro();
    bool executeMacro(const QString &name);

protected:
    friend class Internal::MacroOptionsWidget;

    void deleteMacro(const QString &name);
    void changeMacro(const QString &name, const QString &description);

private:
    static MacroManager *m_instance;

    class MacroManagerPrivate;
    MacroManagerPrivate* d;
};

} // namespace Macros

#endif // MACROSPLUGIN_MACROMANAGER_H
