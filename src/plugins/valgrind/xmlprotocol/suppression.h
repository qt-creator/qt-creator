/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QString;
template <typename T> class QVector;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {

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

typedef QVector<SuppressionFrame> SuppressionFrames;

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

} // namespace XmlProtocol
} // namespace Valgrind
