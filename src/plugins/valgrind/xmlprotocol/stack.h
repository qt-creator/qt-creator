// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QSharedDataPointer>

namespace Valgrind::XmlProtocol {

class Frame;

class Stack
{
public:
    Stack();
    Stack(const Stack &other);
    ~Stack();
    void operator=(const Stack &other);
    void swap(Stack &other);
    bool operator==(const Stack &other) const;

    QString auxWhat() const;
    void setAuxWhat(const QString &auxwhat);

    QList<Frame> frames() const;
    void setFrames(const QList<Frame> &frames);

    //memcheck, ptrcheck, helgrind:
    QString file() const;
    void setFile(const QString &file);

    QString directory() const;
    void setDirectory(const QString &directory);

    qint64 line() const;
    void setLine(qint64 line);

    //helgrind:
    qint64 helgrindThreadId() const;
    void setHelgrindThreadId(qint64 threadId );

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace Stack::XmlProtocol
