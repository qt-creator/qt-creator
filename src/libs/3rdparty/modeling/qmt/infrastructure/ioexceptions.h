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

#ifndef QMT_IOEXCEPTIONS_H
#define QMT_IOEXCEPTIONS_H

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
    IllegalXmlFile(const QString &fileName, int lineNumber = -1);
};

class UnknownFileVersion : public FileIOException
{
public:
    UnknownFileVersion(int version, const QString &fileName, int lineNumber = -1);
};

} // namespace qmt

#endif // QMT_IOEXCEPTIONS_H
