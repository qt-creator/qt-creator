// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/icontext.h>

#include <QWidget>

namespace QmlDesigner {

class TimelineContext : public Core::IContext
{
    Q_OBJECT

public:
    explicit TimelineContext(QWidget *widget);
    void contextHelp(const HelpCallback &callback) const override;
};

} // namespace QmlDesigner
