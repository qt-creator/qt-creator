// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/pathchooser.h>

#include <QPointer>

namespace Haskell {
namespace Internal {

class OptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    OptionsPage();

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    QPointer<QWidget> m_widget;
    QPointer<Utils::PathChooser> m_stackPath;
};

} // namespace Internal
} // namespace Haskell
