// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QByteArrayView>
#include <QList>

namespace EffectMaker {

class SyntaxHighlighterData
{
public:
    SyntaxHighlighterData();

    static QList<QByteArrayView> reservedArgumentNames();
    static QList<QByteArrayView> reservedTagNames();
    static QList<QByteArrayView> reservedFunctionNames();
};

} // namespace EffectMaker


