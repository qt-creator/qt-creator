// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotoptionspagewidget.h"

#include "authwidget.h"
#include "copilotsettings.h"

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

using namespace Utils;
using namespace LanguageClient;

namespace Copilot {

CopilotOptionsPageWidget::CopilotOptionsPageWidget(QWidget *parent)
    : QWidget(parent)
{
    using namespace Layouting;

    auto authWdgt = new AuthWidget();

    // clang-format off
    Column {
        authWdgt, br,
        CopilotSettings::instance().nodeJsPath, br,
        CopilotSettings::instance().distPath, br,
        st
    }.attachTo(this);
    // clang-format on
}

CopilotOptionsPageWidget::~CopilotOptionsPageWidget() = default;

} // namespace Copilot
