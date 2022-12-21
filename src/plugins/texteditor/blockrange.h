// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace TextEditor {

class BlockRange
{
public:
    BlockRange() = default;
    BlockRange(int firstPosition, int lastPosition)
      : firstPosition(firstPosition),
        lastPosition(lastPosition)
    {}

    bool isNull() const
    {
        return lastPosition < firstPosition;
    }

    int first() const
    {
        return firstPosition;
    }

    int last() const
    {
        return lastPosition;
    }

private:
    int firstPosition = 0;
    int lastPosition = -1;
};

} // namespace TextEditor
