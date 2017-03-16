/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com), KDAB (info@kdab.com)
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

#include "qnxversionnumber.h"

#include <QDir>
#include <QRegExp>

namespace Qnx {
namespace Internal {

static const char NONDIGIT_SEGMENT_REGEXP[] = "(?<=\\d)(?=\\D)|(?<=\\D)(?=\\d)";

QnxVersionNumber::QnxVersionNumber(const QStringList &listNumber)
    : m_segments(listNumber)
{
}

QnxVersionNumber::QnxVersionNumber(const QString &version)
{
    m_segments = version.split(QLatin1Char('.'));
}

QnxVersionNumber::QnxVersionNumber()
{
}

QString QnxVersionNumber::toString() const
{
    return m_segments.join(QLatin1Char('.'));
}

bool QnxVersionNumber::operator >(const QnxVersionNumber &b) const
{
    int minSize = size() > b.size() ? b.size() : size();
    for (int i = 0; i < minSize; i++) {
        if (segment(i) != b.segment(i)) {
            // Segment can contain digits and non digits expressions
            QStringList aParts = segment(i).split(QLatin1String(NONDIGIT_SEGMENT_REGEXP));
            QStringList bParts = b.segment(i).split(QLatin1String(NONDIGIT_SEGMENT_REGEXP));

            int minPartSize = aParts.length() > bParts.length() ? bParts.length() : aParts.length();
            for (int j = 0; j < minPartSize; j++) {
                bool aOk = true;
                bool bOk = true;
                int aInt = aParts[j].toInt(&aOk);
                int bInt = bParts[j].toInt(&bOk);

                if (aOk && bOk && (aInt != bInt))
                    return aInt > bInt;

                int compare = aParts[j].compare(bParts[j]);
                if (compare != 0)
                    return compare > 0;
            }
        }
    }

    return false;
}

QString QnxVersionNumber::segment(int index) const
{
    if (index < m_segments.length())
        return m_segments.at(index);

    return QString();
}

QnxVersionNumber QnxVersionNumber::fromTargetName(const QString &targetName)
{
    return fromFileName(targetName, QRegExp(QLatin1String("^target_(.*)$")));
}

QnxVersionNumber QnxVersionNumber::fromFileName(const QString &fileName, const QRegExp &regExp)
{
    QStringList segments;
    if (regExp.exactMatch(fileName) && regExp.captureCount() == 1)
        segments << regExp.cap(1).split(QLatin1Char('_'));

    return QnxVersionNumber(segments);
}

int QnxVersionNumber::size() const
{
    return m_segments.length();
}

bool QnxVersionNumber::isEmpty() const
{
    return m_segments.isEmpty();
}

} // namespace Internal
} // namespace Qnx
