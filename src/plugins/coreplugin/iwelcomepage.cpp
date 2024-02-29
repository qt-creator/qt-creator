// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iwelcomepage.h"

namespace Core {

static QList<IWelcomePage *> g_welcomePages;

const QList<IWelcomePage *> IWelcomePage::allWelcomePages()
{
    return g_welcomePages;
}

IWelcomePage::IWelcomePage()
{
    g_welcomePages.append(this);
}

IWelcomePage::~IWelcomePage()
{
    g_welcomePages.removeOne(this);
}

} // namespace Core
