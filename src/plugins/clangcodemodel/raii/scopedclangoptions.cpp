/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "scopedclangoptions.h"

namespace ClangCodeModel {

/**
 * @class ClangCodeModel::ScopedClangOptions
 * @brief Converts QStringList to raw options, acceptable by clang-c parsing and indexing API
 */

ScopedClangOptions::ScopedClangOptions(const QStringList &options)
    : m_size(options.size())
    , m_rawOptions(new const char*[options.size()])
{
    for (int i = 0 ; i < m_size; ++i)
        m_rawOptions[i] = qstrdup(options[i].toUtf8());
}

ScopedClangOptions::~ScopedClangOptions()
{
    for (int i = 0; i < m_size; ++i)
        delete[] m_rawOptions[i];
    delete[] m_rawOptions;
}

const char **ScopedClangOptions::data() const
{
    return m_rawOptions;
}

int ScopedClangOptions::size() const
{
    return m_size;
}

/**
 * @class ClangCodeModel::SharedClangOptions
 * @brief Shared wrapper around \a {ClangCodeModel::ScopedClangOptions} ScopedClangOptions
 */

SharedClangOptions::SharedClangOptions()
    : d(0)
{
}

SharedClangOptions::SharedClangOptions(const QStringList &options)
    : d(new ScopedClangOptions(options))
{
}

/**
 * @return Replaces options with new options list
 */
void SharedClangOptions::reloadOptions(const QStringList &options)
{
    d = QSharedPointer<ScopedClangOptions>(new ScopedClangOptions(options));
}

/**
 * @return Pointer to clang raw options or NULL if uninitialized
 */
const char **SharedClangOptions::data() const
{
    return d ? d->data() : 0;
}

/**
 * @return Options count or 0 if uninitialized
 */
int SharedClangOptions::size() const
{
    return d ? d->size() : 0;
}

} // namespace ClangCodeModel
