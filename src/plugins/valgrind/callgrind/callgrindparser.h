/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QObject>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {

class ParseData;

/**
 * Parser for Valgrind --tool=callgrind output
 * most of the format is documented at http://kcachegrind.sourceforge.net/html/CallgrindFormat.html
 *
 * FIXME: most length asserts are not correct, see documentation 1.2:
 * "If a cost line specifies less event counts than given in the "events" line,
 * the rest is assumed to be zero."
 *
 */
class Parser : public QObject
{
    Q_OBJECT

public:
    explicit Parser(QObject *parent = 0);
    ~Parser();

    // get and take ownership of the parsing results. If this function is not called the repository
    // will be destroyed when the parser is destroyed. Subsequent calls return null.
    ParseData *takeData();

signals:
    void parserDataReady();

public Q_SLOTS:
    void parse(QIODevice *stream);

private:
    class Private;
    Private *const d;
};

} // Callgrind
} // Valgrind
