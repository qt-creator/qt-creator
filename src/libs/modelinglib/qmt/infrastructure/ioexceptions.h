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
    explicit FileIOException(const QString &errorMsg, const QString &fileName = QString::null,
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
