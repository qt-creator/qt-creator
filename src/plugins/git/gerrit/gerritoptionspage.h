// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QSharedPointer>

namespace Gerrit::Internal {

class GerritParameters;

class GerritOptionsPage : public Core::IOptionsPage
{
public:
    GerritOptionsPage(const QSharedPointer<GerritParameters> &p,
                      const std::function<void()> &onChanged);
};

} // Gerrit::Internal
