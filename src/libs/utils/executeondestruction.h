// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>

namespace Utils {

class ExecuteOnDestruction
{
public:
    ExecuteOnDestruction() noexcept : destructionCode([] {}) {}
    ExecuteOnDestruction(std::function<void()> code) : destructionCode(std::move(code)) {}
    ~ExecuteOnDestruction() { if (destructionCode) destructionCode(); }

    void reset(std::function<void()> code) { destructionCode = std::move(code); }

private:
    std::function<void()> destructionCode;
};

} // Utils
