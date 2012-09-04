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

#ifndef PROJECTEXPLORER_KITCHOOSER_H
#define PROJECTEXPLORER_KITCHOOSER_H

#include "projectexplorer_export.h"

#include <QComboBox>

namespace Core { class Id; }

namespace ProjectExplorer {

class Kit;

// Let the user pick a kit.
class PROJECTEXPLORER_EXPORT KitChooser : public QComboBox
{
    Q_OBJECT

public:
    enum Flags {
        HostAbiOnly = 0x1,
        IncludeInvalidKits = 0x2,
        HasDebugger = 0x4,
        RemoteDebugging = IncludeInvalidKits | HasDebugger,
        LocalDebugging = RemoteDebugging | HostAbiOnly
    };

    explicit KitChooser(QWidget *parent, unsigned flags = 0);

    void setCurrentKitId(Core::Id id);
    Core::Id currentKitId() const;

    Kit *currentKit() const;

private:
    Q_SLOT void onCurrentIndexChanged(int index);
    void populate(unsigned flags);
    Kit *kitAt(int index) const;
};

} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_KITCHOOSER_H
