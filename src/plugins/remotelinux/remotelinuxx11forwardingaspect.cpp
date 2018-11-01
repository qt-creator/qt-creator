/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "remotelinuxx11forwardingaspect.h"

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace RemoteLinux {

static QString defaultDisplay() { return QLatin1String(qgetenv("DISPLAY")); }

X11ForwardingAspect::X11ForwardingAspect()
{
    setDisplayName(tr("X11 Forwarding"));
    setDisplayStyle(LineEditDisplay);
    setId("X11ForwardingAspect");
    setSettingsKey("RunConfiguration.X11Forwarding");
    makeCheckable(tr("Forward to local display"), "RunConfiguration.UseX11Forwarding");
    setValue(defaultDisplay());
}

QString X11ForwardingAspect::display(const Utils::MacroExpander *expander) const
{
    QTC_ASSERT(expander, return value());
    return !isChecked() ? QString() : expander->expandProcessArgs(value());
}

} // namespace RemoteLinux
