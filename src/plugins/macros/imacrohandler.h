/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
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

#ifndef MACROSPLUGIN_IMACROHANDLER_H
#define MACROSPLUGIN_IMACROHANDLER_H

#include "macros_global.h"
#include <QObject>

namespace Macros {

class Macro;
class MacroEvent;
class MacroManager;

class MACROS_EXPORT IMacroHandler: public QObject
{
public:
    IMacroHandler();
    virtual ~IMacroHandler();

    virtual void startRecording(Macro* macro);
    virtual void endRecordingMacro(Macro* macro);

    virtual bool canExecuteEvent(const MacroEvent &macroEvent) = 0;
    virtual bool executeEvent(const MacroEvent &macroEvent) = 0;

protected:
    void addMacroEvent(const MacroEvent& event);

    void setCurrentMacro(Macro *macro);

    bool isRecording() const;

private:
    friend class MacroManager;

    class IMacroHandlerPrivate;
    IMacroHandlerPrivate *d;
};

} // namespace Macros

#endif // MACROSPLUGIN_IMACROHANDLER_H
