// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QCache>
#include <QFont>
#include <QGlyphRun>
#include <QString>

namespace TerminalSolution {

struct GlyphCacheKey
{
    QFont font;
    QString text;

    bool operator==(const GlyphCacheKey &other) const
    {
        return font == other.font && text == other.text;
    }
};

class GlyphCache : public QCache<GlyphCacheKey, QGlyphRun>
{
public:
    using QCache::QCache;

    static GlyphCache &instance();

    const QGlyphRun *get(const QFont &font, const QString &text);
};

} // namespace TerminalSolution
