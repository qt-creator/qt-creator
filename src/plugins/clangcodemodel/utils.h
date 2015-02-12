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

#ifndef UTILS_H
#define UTILS_H

#include "pchinfo.h"

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QMap>
#include <QPair>

/*
 * A header for globally visible typedefs. This is particularly useful
 * so we don't have to #include files simply because of a typedef. Still,
 * not every typedef should go in here, only the minimal subset of the
 * ones which are needed quite often.
 */
namespace ClangCodeModel {
namespace Internal {

typedef QMap<QString, QByteArray> UnsavedFiles;

/**
 * Utility method to create a PCH file from a header file.
 *
 * \returns a boolean indicating success (true) or failure (false), and a
 *          list of diagnostic messages.
 */
QPair<bool, QStringList> precompile(const PchInfo::Ptr &pchInfo);

void initializeClang();

} // Internal namespace
} // ClangCodeModel namespace

#endif // UTILS_H
