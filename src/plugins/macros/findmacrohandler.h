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

#ifndef MACROSPLUGIN_FINDEVENTHANDLER_H
#define MACROSPLUGIN_FINDEVENTHANDLER_H

#include "imacrohandler.h"

#include <find/textfindconstants.h>

namespace Core {
class IEditor;
}

namespace Macros {
namespace Internal {

class FindMacroHandler : public IMacroHandler
{
    Q_OBJECT

public:
    FindMacroHandler();

    void startRecording(Macros::Macro* macro);

    bool canExecuteEvent(const Macros::MacroEvent &macroEvent);
    bool executeEvent(const Macros::MacroEvent &macroEvent);

public slots:
    void findIncremental(const QString &txt, Find::FindFlags findFlags);
    void findStep(const QString &txt, Find::FindFlags findFlags);
    void replace(const QString &before, const QString &after, Find::FindFlags findFlags);
    void replaceStep(const QString &before, const QString &after, Find::FindFlags findFlags);
    void replaceAll(const QString &before, const QString &after, Find::FindFlags findFlags);
    void resetIncrementalSearch();

private slots:
    void changeEditor(Core::IEditor *editor);
};

} // namespace Internal
} // namespace Macros

#endif // MACROSPLUGIN_FINDEVENTHANDLER_H
