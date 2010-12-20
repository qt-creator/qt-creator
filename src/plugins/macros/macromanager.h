/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MACROSPLUGIN_MACROMANAGER_H
#define MACROSPLUGIN_MACROMANAGER_H

#include <QObject>
#include <QMap>

#include "macros_global.h"

class QAction;

namespace Macros {

class Macro;
class IMacroHandler;

namespace Internal {
    class MacroSettings;
    class MacroOptionsWidget;
}

class MACROS_EXPORT MacroManager : public QObject
{
    Q_OBJECT
public:
    explicit MacroManager(QObject *parent = 0);
    ~MacroManager();

    static MacroManager *instance();

    const Internal::MacroSettings &settings() const;
    const QMap<QString, Macro *> &macros() const;

    void registerMacroHandler(IMacroHandler *handler);

public slots:
    void startMacro();
    void endMacro();
    void executeLastMacro();
    bool executeMacro(const QString &name);

protected:
    friend class Internal::MacroOptionsWidget;

    void deleteMacro(const QString &name);
    void changeMacro(const QString &name, const QString &description, bool shortcut);
    void appendDirectory(const QString &directory);
    void removeDirectory(const QString &directory);
    void setDefaultDirectory(const QString &directory);
    void showSaveDialog(bool value);
    void saveSettings();

private:
    static MacroManager *m_instance;

    class MacroManagerPrivate;
    MacroManagerPrivate* d;
};

} // namespace Macros

#endif // MACROSPLUGIN_MACROMANAGER_H
