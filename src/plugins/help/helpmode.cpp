// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helpmode.h"
#include "helpconstants.h"
#include "helpicons.h"
#include "helptr.h"

#include <QCoreApplication>

using namespace Help;
using namespace Help::Internal;

HelpMode::HelpMode(QObject *parent)
  : Core::IMode(parent)
{
    setObjectName("HelpMode");
    setContext(Core::Context(Constants::C_MODE_HELP));
    setIcon(Utils::Icon::modeIcon(Icons::MODE_HELP_CLASSIC,
                                  Icons::MODE_HELP_FLAT, Icons::MODE_HELP_FLAT_ACTIVE));
    setDisplayName(Tr::tr("Help"));
    setPriority(Constants::P_MODE_HELP);
    setId(Constants::ID_MODE_HELP);
}
