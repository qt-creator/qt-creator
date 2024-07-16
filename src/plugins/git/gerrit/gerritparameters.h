// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gerritserver.h"

#include <utils/filepath.h>

namespace Gerrit::Internal {

class GerritParameters
{
public:
    bool isValid() const;
    void toSettings() const;
    void saveQueries() const;
    void fromSettings();
    void setPortFlagBySshType();

    GerritServer server;
    Utils::FilePath ssh;
    Utils::FilePath curl;
    QStringList savedQueries;
    bool https = true;
    QString portFlag;

private:
    friend GerritParameters &gerritSettings();
    GerritParameters();
};

GerritParameters &gerritSettings();

} // Gerrit::Internal
