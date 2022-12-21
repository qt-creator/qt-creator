// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSharedDataPointer>
#include <QVector>

namespace Valgrind {
namespace XmlProtocol {

class Frame;

class AnnounceThread {
public:
    AnnounceThread();
    AnnounceThread(const AnnounceThread &other);
    ~AnnounceThread();
    AnnounceThread &operator=(const AnnounceThread &other);
    void swap(AnnounceThread &other);
    bool operator==(const AnnounceThread &other) const;

    qint64 helgrindThreadId() const;
    void setHelgrindThreadId(qint64 id);

    QVector<Frame> stack() const;
    void setStack(const QVector<Frame> &stack);

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace XmlProtocol
} // namespace Valgrind
