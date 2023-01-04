// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

class ViewConfig
{
public:
    static bool isQuick3DMode();
    static void enableParticleView(bool enable);
    static bool isParticleViewMode();
};
