/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef LIBVALGRIND_PROTOCOL_SUPPRESSION_H
#define LIBVALGRIND_PROTOCOL_SUPPRESSION_H

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

#endif // LIBVALGRIND_PROTOCOL_SUPPRESSION_H
