// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QSharedPointer>

namespace Gerrit::Internal {

class GerritParameters;

class GerritOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    GerritOptionsPage(const QSharedPointer<GerritParameters> &p, QObject *parent = nullptr);

signals:
    void settingsChanged();
};

} // Gerrit::Internal
