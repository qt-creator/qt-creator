/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "ioexceptions.h"
#include <QObject>

namespace qmt {

IOException::IOException(const QString &errorMsg)
    : Exception(errorMsg)
{
}

FileIOException::FileIOException(const QString &errorMsg, const QString &fileName, int lineNumber)
    : IOException(errorMsg),
      m_fileName(fileName),
      m_lineNumber(lineNumber)
{
}

FileNotFoundException::FileNotFoundException(const QString &fileName)
    : FileIOException(Exception::tr("File not found."), fileName)
{
}

FileCreationException::FileCreationException(const QString &fileName)
    : FileIOException(Exception::tr("Unable to create file."), fileName)
{
}

FileWriteError::FileWriteError(const QString &fileName, int lineNumber)
    : FileIOException(Exception::tr("Writing to file failed."), fileName, lineNumber)
{
}

FileReadError::FileReadError(const QString &fileName, int lineNumber)
    : FileIOException(Exception::tr("Reading from file failed."), fileName, lineNumber)
{
}

IllegalXmlFile::IllegalXmlFile(const QString &fileName, int lineNumber)
    : FileIOException(Exception::tr("Illegal XML file."), fileName, lineNumber)
{
}

UnknownFileVersion::UnknownFileVersion(int version, const QString &fileName, int lineNumber)
    : FileIOException(Exception::tr("Unable to handle file version %1.")
                      .arg(version), fileName, lineNumber)
{
}

} // namespace qmt

