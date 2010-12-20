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

#ifndef MACROSPLUGIN_MACROMANAGER_H
#define MACROSPLUGIN_MACROMANAGER_H

#include "macros_global.h"

#include <QtCore/QObject>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

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
