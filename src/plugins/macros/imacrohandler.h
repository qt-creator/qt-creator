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
