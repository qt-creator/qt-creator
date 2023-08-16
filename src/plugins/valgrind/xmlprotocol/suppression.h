// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Valgrind::XmlProtocol {

class SuppressionFrame
{
public:
    SuppressionFrame();
    SuppressionFrame(const SuppressionFrame &other);
    ~SuppressionFrame();
    SuppressionFrame &operator=(const SuppressionFrame &other);
    void swap(SuppressionFrame &other);
    bool operator==(const SuppressionFrame &other) const;
    bool operator!=(const SuppressionFrame &other) const
    {
        return !operator==(other);
    }

    QString object() const;
    void setObject(const QString &object);

    QString function() const;
    void setFunction(const QString &function);

    QString toString() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

using SuppressionFrames = QList<SuppressionFrame>;

class Suppression
{
public:
    Suppression();
    Suppression(const Suppression &other);
    ~Suppression();
    Suppression &operator=(const Suppression &other);

    void swap(Suppression &other);
    bool operator==(const Suppression &other) const;

    bool isNull() const;

    QString name() const;
    void setName(const QString &name);

    QString kind() const;
    void setKind(const QString &kind);

    QString auxKind() const;
    void setAuxKind(const QString &kind);

    QString rawText() const;
    void setRawText(const QString &text);

    SuppressionFrames frames() const;
    void setFrames(const SuppressionFrames &frames);

    QString toString() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace Valgrind::XmlProtocol
