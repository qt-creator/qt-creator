/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MESSAGEMANAGER_H
#define MESSAGEMANAGER_H

#include "core_global.h"
#include "ioutputpane.h"
#include <QMetaType>

#include <QObject>

namespace Core {

namespace Internal { class MainWindow; }

class CORE_EXPORT MessageManager : public QObject
{
    Q_OBJECT

public:
    static QObject *instance();

    static void showOutputPane();

    enum PrintToOutputPaneFlag {
        NoModeSwitch   = IOutputPane::NoModeSwitch,
        ModeSwitch     = IOutputPane::ModeSwitch,
        WithFocus      = IOutputPane::WithFocus,
        EnsureSizeHint = IOutputPane::EnsureSizeHint,
        Silent         = 256,
        Flash          = 512
    };

    Q_DECLARE_FLAGS(PrintToOutputPaneFlags, PrintToOutputPaneFlag)

    static void write(const QString &text); // imply NoModeSwitch

public slots:
    static void write(const QString &text, Core::MessageManager::PrintToOutputPaneFlags flags);

private:
    MessageManager();
    ~MessageManager();
    static void init();
    friend class Core::Internal::MainWindow;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::MessageManager::PrintToOutputPaneFlags)

#endif // MESSAGEMANAGER_H
