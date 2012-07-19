/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef APPLICATIONRUNCONFIGURATION_H
#define APPLICATIONRUNCONFIGURATION_H

#include "runconfiguration.h"
#include "applicationlauncher.h"

namespace Utils {
class AbstractMacroExpander;
class Environment;
}

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT LocalApplicationRunConfiguration : public RunConfiguration
{
    Q_OBJECT
public:
    enum RunMode {
        Console = ApplicationLauncher::Console,
        Gui
    };

    virtual ~LocalApplicationRunConfiguration();
    virtual QString executable() const = 0;
    virtual RunMode runMode() const = 0;
    virtual QString workingDirectory() const = 0;
    virtual QString commandLineArguments() const = 0;
    virtual Utils::Environment environment() const = 0;
    virtual QString dumperLibrary() const = 0;
    virtual QStringList dumperLibraryLocations() const = 0;

protected:
    explicit LocalApplicationRunConfiguration(Target *target, const Core::Id id);
    explicit LocalApplicationRunConfiguration(Target *target, LocalApplicationRunConfiguration *rc);

    Utils::AbstractMacroExpander *macroExpander() const;
};

} // namespace ProjectExplorer

#endif // APPLICATIONRUNCONFIGURATION_H
