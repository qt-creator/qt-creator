/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGERTOOLCHAINCOMBOBOX_H
#define DEBUGGERTOOLCHAINCOMBOBOX_H

#include <QComboBox>

namespace ProjectExplorer { class Profile; }

namespace Debugger {
namespace Internal {

// Let the user pick a profile associated with a debugger.
class DebuggerToolChainComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit DebuggerToolChainComboBox(QWidget *parent);

    void init(bool hostAbiOnly);

    void setProfile(const ProjectExplorer::Profile *profile);
    ProjectExplorer::Profile *profile() const;
    //QString debuggerCommand() const;

private:
    ProjectExplorer::Profile *profileAt(int index) const;
};

} // namespace Debugger
} // namespace Internal

#endif // DEBUGGERTOOLCHAINCOMBOBOX_H
