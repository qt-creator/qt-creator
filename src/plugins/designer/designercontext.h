// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/icontext.h>

namespace Designer {
namespace Internal {

class DesignerContext : public Core::IContext
{
public:
    explicit DesignerContext(const Core::Context &contexts,
                             QWidget *widget,
                             QObject *parent = nullptr);

    void contextHelp(const HelpCallback &callback) const override;
};

} // namespace Internal
} // namespace Designer
