// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "glyphcache.h"

#include <QTextLayout>

namespace TerminalSolution {

size_t qHash(const GlyphCacheKey &key, size_t seed = 0)
{
    return qHash(key.font, seed) ^ qHash(key.text, seed);
}

GlyphCache &GlyphCache::instance()
{
    static GlyphCache cache(5000);
    return cache;
}

const QGlyphRun *GlyphCache::get(const QFont &font, const QString &text)
{
    GlyphCacheKey key{font, text};
    if (auto *run = object(key))
        return run;

    QTextLayout layout;

    layout.setText(text);
    layout.setFont(font);

    layout.beginLayout();
    layout.createLine().setNumColumns(std::numeric_limits<int>::max());
    layout.endLayout();

    if (layout.lineCount() > 0) {
        const auto &line = layout.lineAt(0);
        const auto runs = line.glyphRuns();
        if (!runs.isEmpty()) {
            QGlyphRun *run = new QGlyphRun(layout.lineAt(0).glyphRuns().first());
            if (insert(key, run))
                return run;
        }
    }
    return nullptr;
}

} // namespace TerminalSolution
