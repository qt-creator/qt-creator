// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "exceptions.h"

namespace qmt {

class IOException : public Exception
{
public:
    explicit IOException(const QString &errorMsg);
};

class FileIOException : public IOException
{
public:
    explicit FileIOException(const QString &errorMsg, const QString &fileName = QString(),
                             int lineNumber = -1);

    QString fileName() const { return m_fileName; }
    int lineNumber() const { return m_lineNumber; }

private:
    QString m_fileName;
    int m_lineNumber = -1;
};

class FileNotFoundException : public FileIOException
{
public:
    explicit FileNotFoundException(const QString &fileName);
};

class FileCreationException : public FileIOException
{
public:
    explicit FileCreationException(const QString &fileName);
};

class FileWriteError : public FileIOException
{
public:
    explicit FileWriteError(const QString &fileName, int lineNumber = -1);
};

class FileReadError : public FileIOException
{
public:
    explicit FileReadError(const QString &fileName, int lineNumber = -1);
};

class IllegalXmlFile : public FileIOException
{
public:
    explicit IllegalXmlFile(const QString &fileName, int lineNumber = -1);
};

class UnknownFileVersion : public FileIOException
{
public:
    UnknownFileVersion(int version, const QString &fileName, int lineNumber = -1);
};

} // namespace qmt
