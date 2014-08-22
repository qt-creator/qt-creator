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

#include "blackberrydebugtokenreader.h"

#include <QStringList>

#ifdef QNX_ZIP_FILE_SUPPORT
#include <private/qzipreader_p.h>
#endif

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
#ifdef QNX_ZIP_FILE_SUPPORT
const char MANIFEST_FILENAME[] = "META-INF/MANIFEST.MF";
#endif

const char MANIFEST_AUTHOR_KEY[]    = "Package-Author: ";
const char MANIFEST_AUTHOR_ID_KEY[] = "Package-Author-Id: ";
const char MANIFEST_EXPIRY[]        = "Debug-Token-Expiry-Date: ";
const char MANIFEST_PINS[]          = "Debug-Token-Device-Id: ";
}

BlackBerryDebugTokenReader::BlackBerryDebugTokenReader(const QString &filePath)
{
#ifdef QNX_ZIP_FILE_SUPPORT
    m_zipReader = new QZipReader(filePath);
#else
    Q_UNUSED(filePath);
    Q_UNUSED(m_zipReader);
#endif
}

BlackBerryDebugTokenReader::~BlackBerryDebugTokenReader()
{
#ifdef QNX_ZIP_FILE_SUPPORT
    m_zipReader->close();
    delete m_zipReader;
    m_zipReader = 0;
#endif
}

bool BlackBerryDebugTokenReader::isValid() const
{
#ifdef QNX_ZIP_FILE_SUPPORT
    return m_zipReader->status() == QZipReader::NoError;
#else
    return false;
#endif
}

QString BlackBerryDebugTokenReader::author() const
{
    return manifestValue(MANIFEST_AUTHOR_KEY);
}

QString BlackBerryDebugTokenReader::authorId() const
{
    return manifestValue(MANIFEST_AUTHOR_ID_KEY);
}

QString BlackBerryDebugTokenReader::expiry() const
{
    return manifestValue(MANIFEST_EXPIRY);
}

QString BlackBerryDebugTokenReader::pins() const
{
    const QString value = manifestValue(MANIFEST_PINS);
    QStringList pins = value.split(QLatin1Char(','));
    QStringList pinsHexa;
    foreach (const QString &pin, pins) {
        QString hexa;
        pinsHexa << hexa.setNum(pin.toUInt(), 16);
    }

    return pinsHexa.join(QLatin1Char(','));
}

bool BlackBerryDebugTokenReader::isSupported()
{
#ifdef QNX_ZIP_FILE_SUPPORT
    return true;
#else
    return false;
#endif
}

QString BlackBerryDebugTokenReader::manifestValue(const QByteArray &key) const
{
    if (!isValid())
        return QString();

#ifdef QNX_ZIP_FILE_SUPPORT
    QByteArray manifestContent = m_zipReader->fileData(QLatin1String(MANIFEST_FILENAME));
    return value(key, manifestContent);
#else
    Q_UNUSED(key);
    return QString();
#endif
}

QString BlackBerryDebugTokenReader::value(const QByteArray &key, const QByteArray &data) const
{
    int valueStart = data.indexOf(key) + key.size();
    int valueEnd = data.indexOf(QByteArray("\r\n"), valueStart);
    return QString::fromAscii(data.mid(valueStart, valueEnd - valueStart));
}
