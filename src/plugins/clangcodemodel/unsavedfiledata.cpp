/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "unsavedfiledata.h"

using namespace ClangCodeModel::Internal;

UnsavedFileData::UnsavedFileData(const UnsavedFiles &unsavedFiles)
    : m_count(unsavedFiles.count())
    , m_files(0)
{
    if (m_count) {
        m_files = new CXUnsavedFile[m_count];
        unsigned idx = 0;
        for (UnsavedFiles::const_iterator it = unsavedFiles.begin(); it != unsavedFiles.end(); ++it, ++idx) {
            QByteArray contents = it.value();
            const char *contentChars = qstrdup(contents.constData());
            m_files[idx].Contents = contentChars;
            m_files[idx].Length = contents.size();

            const char *fileName = qstrdup(it.key().toUtf8().constData());
            m_files[idx].Filename = fileName;
        }
    }
}

UnsavedFileData::~UnsavedFileData()
{
    for (unsigned i = 0; i < m_count; ++i) {
        delete[] m_files[i].Contents;
        delete[] m_files[i].Filename;
    }

    delete[] m_files;
}
