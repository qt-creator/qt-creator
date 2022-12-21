// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffservice.h"

namespace Core {

static DiffService *g_instance = nullptr;

DiffService::DiffService()
{
    g_instance = this;
}

DiffService::~DiffService()
{
    g_instance = nullptr;
}

DiffService *DiffService::instance()
{
    return g_instance;
}

} // Core
