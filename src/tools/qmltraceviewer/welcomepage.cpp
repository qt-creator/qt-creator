// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "welcomepage.h"

#include <profiler/profilertr.h>

#include <utils/layoutbuilder.h>
#include <utils/qtdesignwidgets.h>

using namespace Layouting;
using namespace Profiler;
using namespace Utils;
using namespace Utils::StyleHelper;

namespace QmlTraceViewer {

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
{
    // clang-format off
    Row {
        st,
        Column {
            st,
            QtDesignWidgets::Button {
                role(QtcButton::LargePrimary),
                text(Tr::tr("Attach to Process")),
                Layouting::toolTip(Tr::tr("Record a trace of a running process.")),
                onClicked(this, [this] { emit attachToProcessRequested(); }),
            },
            Space(SpacingTokens::PrimitiveM),
            QtDesignWidgets::Button {
                role(QtcButton::LargePrimary),
                text(Tr::tr("Launch Executable")),
                Layouting::toolTip(Tr::tr("Launch an executable and record a trace of it.")),
                onClicked(this, [this] { emit launchExecutableRequested(); }),
            },
            Space(SpacingTokens::PrimitiveM),
            QtDesignWidgets::Button {
                role(QtcButton::LargeSecondary),
                text(Tr::tr("Open Existing")),
                Layouting::toolTip(Tr::tr("Load a QML, Chrome Trace Format or Common Trace Format trace.")),
                onClicked(this, [this] { emit openTraceRequested(); }),
            },
            st,
        }
        ,
        st,
    }.attachTo(this);
    // clang-format on
}

} // namespace QmlTraceViewer
