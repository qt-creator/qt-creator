/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxversionnumber.h"

#include <QDir>

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
    return m_segments.join(QLatin1String("."));
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

                if (aOk && bOk)
                    return aInt > bInt;

                return aParts[j].compare(bParts[j]) > 0;
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

QnxVersionNumber QnxVersionNumber::fromNdkEnvFileName(const QString &ndkEnvFileName)
{
    return fromFileName(ndkEnvFileName, QRegExp(QLatin1String("^bbndk-env_(.*)$")));
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
