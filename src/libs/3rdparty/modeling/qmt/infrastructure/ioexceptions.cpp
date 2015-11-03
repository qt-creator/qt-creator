/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
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

#include "ioexceptions.h"

namespace qmt {

IOException::IOException(const QString &error_msg)
    : Exception(error_msg)
{
}

FileIOException::FileIOException(const QString &error_msg, const QString &file_name, int line_number)
    : IOException(error_msg),
      m_fileName(file_name),
      m_lineNumber(line_number)
{
}

FileNotFoundException::FileNotFoundException(const QString &file_name)
    : FileIOException(QStringLiteral("file not found"), file_name)
{
}

FileCreationException::FileCreationException(const QString &file_name)
    : FileIOException(QStringLiteral("unable to create file"), file_name)
{
}

FileWriteError::FileWriteError(const QString &file_name, int line_number)
    : FileIOException(QStringLiteral("writing to file failed"), file_name, line_number)
{
}

FileReadError::FileReadError(const QString &file_name, int line_number)
    : FileIOException(QStringLiteral("reading from file failed"), file_name, line_number)
{
}

IllegalXmlFile::IllegalXmlFile(const QString &file_name, int line_number)
    : FileIOException(QStringLiteral("illegal xml file"), file_name, line_number)
{
}

UnknownFileVersion::UnknownFileVersion(int version, const QString &file_name, int line_number)
    : FileIOException(QString(QStringLiteral("unable to handle file version %1")).arg(version), file_name, line_number)
{
}

}

