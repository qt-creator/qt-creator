// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

class Base
{
public:
    // misc-noexcept-move-constructor
    // misc-unconventional-assign-operator
    // misc-unused-parameters
    Base operator=(Base &&param) { return {}; }
    virtual int function()
    {
        // modernize-use-nullptr
        int *a = 0;
        return 0;
    }
};

int main() {
  return 0;
}
