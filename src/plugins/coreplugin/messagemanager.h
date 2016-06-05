/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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
    static MessageManager *instance();

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

public slots:
    static void write(const QString &text,
                      Core::MessageManager::PrintToOutputPaneFlags flags = NoModeSwitch);

private:
    MessageManager();
    ~MessageManager();
    static void init();
    friend class Core::Internal::MainWindow;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::MessageManager::PrintToOutputPaneFlags)
