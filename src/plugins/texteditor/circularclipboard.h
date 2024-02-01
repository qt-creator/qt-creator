// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMimeData>

namespace TextEditor {
namespace Internal {

class CircularClipboard
{
public:
    static CircularClipboard *instance();

    void collect(const QMimeData *mimeData);
    void collect(const std::shared_ptr<const QMimeData> &mimeData);
    std::shared_ptr<const QMimeData> next() const;
    void toLastCollect();
    int size() const;

private:
    CircularClipboard();
    ~CircularClipboard();
    CircularClipboard &operator=(const CircularClipboard &);

    mutable int m_current = -1;
    QList<std::shared_ptr<const QMimeData>> m_items;
};

} // namespace Internal
} // namespace TextEditor
