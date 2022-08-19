// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <utils/aspects.h>

namespace Utils { class MacroExpander; }

namespace RemoteLinux {

class REMOTELINUX_EXPORT X11ForwardingAspect : public Utils::StringAspect
{
    Q_OBJECT

public:
    X11ForwardingAspect(const Utils::MacroExpander *macroExpander);

    struct Data : StringAspect::Data
    {
        QString display;
    };

    QString display() const;

private:
    const Utils::MacroExpander *m_macroExpander;
};

} // namespace RemoteLinux
