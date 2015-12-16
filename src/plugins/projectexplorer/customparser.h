/****************************************************************************
**
** Copyright (C) 2015 Andre Hartmann.
** Contact: aha_1980@gmx.de
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

#ifndef CUSTOMPARSER_H
#define CUSTOMPARSER_H

#include "ioutputparser.h"

#include <projectexplorer/task.h>

#include <QRegularExpression>

namespace ProjectExplorer {

class CustomParserExpression
{
public:
    enum CustomParserChannel {
        ParseNoChannel = 0,
        ParseStdErrChannel = 1,
        ParseStdOutChannel = 2,
        ParseBothChannels = 3
    };

    bool operator ==(const CustomParserExpression &other) const;

    QString pattern() const;
    void setPattern(const QString &pattern);
    QRegularExpressionMatch match(const QString &line) const { return m_regExp.match(line); }

    CustomParserExpression::CustomParserChannel channel() const;
    void setChannel(CustomParserExpression::CustomParserChannel channel);

    QString example() const;
    void setExample(const QString &example);

    int fileNameCap() const;
    void setFileNameCap(int fileNameCap);
    int lineNumberCap() const;
    void setLineNumberCap(int lineNumberCap);
    int messageCap() const;
    void setMessageCap(int messageCap);

private:
    QRegularExpression m_regExp;
    CustomParserExpression::CustomParserChannel m_channel = ParseBothChannels;
    QString m_example;
    int m_fileNameCap = 1;
    int m_lineNumberCap = 2;
    int m_messageCap = 3;
};

class CustomParserSettings
{
public:
    bool operator ==(const CustomParserSettings &other) const;
    bool operator !=(const CustomParserSettings &other) const { return !operator==(other); }

    CustomParserExpression error;
    CustomParserExpression warning;
};

class CustomParser : public ProjectExplorer::IOutputParser
{
public:
    CustomParser(const CustomParserSettings &settings = CustomParserSettings());
    ~CustomParser();
    void stdError(const QString &line);
    void stdOutput(const QString &line);

    void setSettings(const CustomParserSettings &settings);

private:
    bool hasMatch(const QString &line, CustomParserExpression::CustomParserChannel channel,
                  const CustomParserExpression &expression, Task::TaskType taskType);
    bool parseLine(const QString &rawLine, CustomParserExpression::CustomParserChannel channel);

    CustomParserExpression m_error;
    CustomParserExpression m_warning;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::CustomParserExpression::CustomParserChannel);

#endif // CUSTOMPARSER_H
