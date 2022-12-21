// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMimeData>
#include <QSharedPointer>

namespace TextEditor {
namespace Internal {

class CircularClipboard
{
public:
    static CircularClipboard *instance();

    void collect(const QMimeData *mimeData);
    void collect(const QSharedPointer<const QMimeData> &mimeData);
    QSharedPointer<const QMimeData> next() const;
    void toLastCollect();
    int size() const;

private:
    CircularClipboard();
    ~CircularClipboard();
    CircularClipboard &operator=(const CircularClipboard &);

    mutable int m_current = -1;
    QList< QSharedPointer<const QMimeData> > m_items;
};

} // namespace Internal
} // namespace TextEditor
