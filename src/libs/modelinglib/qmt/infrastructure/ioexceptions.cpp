// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ioexceptions.h"

#include "../../modelinglibtr.h"

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
    : FileIOException(Tr::tr("File not found."), fileName)
{
}

FileCreationException::FileCreationException(const QString &fileName)
    : FileIOException(Tr::tr("Unable to create file."), fileName)
{
}

FileWriteError::FileWriteError(const QString &fileName, int lineNumber)
    : FileIOException(Tr::tr("Writing to file failed."), fileName, lineNumber)
{
}

FileReadError::FileReadError(const QString &fileName, int lineNumber)
    : FileIOException(Tr::tr("Reading from file failed."), fileName, lineNumber)
{
}

IllegalXmlFile::IllegalXmlFile(const QString &fileName, int lineNumber)
    : FileIOException(Tr::tr("Illegal XML file."), fileName, lineNumber)
{
}

UnknownFileVersion::UnknownFileVersion(int version, const QString &fileName, int lineNumber)
    : FileIOException(Tr::tr("Unable to handle file version %1.")
                      .arg(version), fileName, lineNumber)
{
}

} // namespace qmt

