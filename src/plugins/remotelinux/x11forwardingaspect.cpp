// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "x11forwardingaspect.h"

#include "remotelinuxtr.h"

#include <utils/environment.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace RemoteLinux {

static QString defaultDisplay()
{
    return qtcEnvironmentVariable("DISPLAY");
}

X11ForwardingAspect::X11ForwardingAspect(const MacroExpander *expander)
    : m_macroExpander(expander)
{
    setLabelText(Tr::tr("X11 Forwarding:"));
    setDisplayStyle(LineEditDisplay);
    setId("X11ForwardingAspect");
    setSettingsKey("RunConfiguration.X11Forwarding");
    makeCheckable(CheckBoxPlacement::Right, Tr::tr("Forward to local display"),
                  "RunConfiguration.UseX11Forwarding");
    setValue(defaultDisplay());

    addDataExtractor(this, &X11ForwardingAspect::display, &Data::display);
}

QString X11ForwardingAspect::display() const
{
    QTC_ASSERT(m_macroExpander, return value());
    return !isChecked() ? QString() : m_macroExpander->expandProcessArgs(value());
}

} // namespace RemoteLinux
