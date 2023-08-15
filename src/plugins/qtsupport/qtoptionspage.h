// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace QtSupport {
namespace Internal {

class QtOptionsPage final : public Core::IOptionsPage
{
public:
    QtOptionsPage();

    QStringList keywords() const final;

};

} // QtSupport

namespace LinkWithQtSupport {
QTSUPPORT_EXPORT bool canLinkWithQt();
QTSUPPORT_EXPORT bool isLinkedWithQt();
QTSUPPORT_EXPORT Utils::FilePath linkedQt();
QTSUPPORT_EXPORT void linkWithQt();
}

} // Internal
