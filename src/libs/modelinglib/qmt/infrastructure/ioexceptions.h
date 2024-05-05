// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "exceptions.h"

#include <utils/filepath.h>

namespace qmt {

class IOException : public Exception
{
public:
    explicit IOException(const QString &errorMsg);
};

class FileIOException : public IOException
{
public:
    explicit FileIOException(const QString &errorMsg, const Utils::FilePath &fileName = {},
                             int lineNumber = -1);

    Utils::FilePath fileName() const { return m_fileName; }
    int lineNumber() const { return m_lineNumber; }

private:
    Utils::FilePath m_fileName;
    int m_lineNumber = -1;
};

class FileNotFoundException : public FileIOException
{
public:
    explicit FileNotFoundException(const Utils::FilePath &fileName);
};

class FileCreationException : public FileIOException
{
public:
    explicit FileCreationException(const Utils::FilePath &fileName);
};

class FileWriteError : public FileIOException
{
public:
    explicit FileWriteError(const Utils::FilePath &fileName, int lineNumber = -1);
};

class FileReadError : public FileIOException
{
public:
    explicit FileReadError(const Utils::FilePath &fileName, int lineNumber = -1);
};

class IllegalXmlFile : public FileIOException
{
public:
    explicit IllegalXmlFile(const Utils::FilePath &fileName, int lineNumber = -1);
};

class UnknownFileVersion : public FileIOException
{
public:
    UnknownFileVersion(int version, const Utils::FilePath &fileName, int lineNumber = -1);
};

} // namespace qmt
