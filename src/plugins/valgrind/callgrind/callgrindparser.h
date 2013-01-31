/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef LIBVALGRIND_CALLGRIND_PARSER_H
#define LIBVALGRIND_CALLGRIND_PARSER_H

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

    // get and take ownership of the parsing results. If this method is not called the repository
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

#endif // LIBVALGRIND_CALLGRIND_PARSER_H
