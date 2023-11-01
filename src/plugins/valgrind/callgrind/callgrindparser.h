// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Utils { class FilePath; }

namespace Valgrind::Callgrind {

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
    Parser();
    ~Parser() override;

    // get and take ownership of the parsing results. If this function is not called the repository
    // will be destroyed when the parser is destroyed. Subsequent calls return null.
    ParseData *takeData();
    void parse(const Utils::FilePath &filePath);

signals:
    void parserDataReady();

private:
    class Private;
    Private *const d;
};

} // namespace Valgrind::Callgrind
