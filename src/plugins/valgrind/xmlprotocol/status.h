// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QSharedDataPointer>

namespace Valgrind::XmlProtocol {

class Status
{
public:
    enum State {
        Running,
        Finished
    };

    Status();
    Status(const Status &other);
    ~Status();
    Status &operator=(const Status &other);
    void swap(Status &other);
    bool operator==(const Status &other) const;

    State state() const;
    void setState(State state);

    QString time() const;
    void setTime(const QString &time);

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace Valgrind::XmlProtocol

Q_DECLARE_METATYPE(Valgrind::XmlProtocol::Status)
