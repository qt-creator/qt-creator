// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxversionnumber.h"

#include <QDir>
#include <QRegularExpression>

namespace Qnx::Internal {

static const char NONDIGIT_SEGMENT_REGEXP[] = "(?<=\\d)(?=\\D)|(?<=\\D)(?=\\d)";

QnxVersionNumber::QnxVersionNumber(const QStringList &listNumber)
    : m_segments(listNumber)
{
}

QnxVersionNumber::QnxVersionNumber(const QString &version)
{
    m_segments = version.split(QLatin1Char('.'));
}

QnxVersionNumber::QnxVersionNumber() = default;

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
    return fromFileName(targetName, QRegularExpression("^target_(.*)$"));
}

QnxVersionNumber QnxVersionNumber::fromFileName(const QString &fileName, const QRegularExpression &regExp)
{
    QStringList segments;
    const QRegularExpressionMatch match = regExp.match(fileName);
    if (match.hasMatch() && regExp.captureCount() == 1)
        segments << match.captured(1).split(QLatin1Char('_'));

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

} // Qnx::Internal
