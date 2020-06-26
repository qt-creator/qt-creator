/****************************************************************************
**
** Copyright (C) 2016 Andre Hartmann.
** Contact: aha_1980@gmx.de
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

#include "ioutputparser.h"
#include "projectconfiguration.h"

#include <projectexplorer/task.h>
#include <utils/detailswidget.h>

#include <QRegularExpression>
#include <QVariantMap>

namespace ProjectExplorer {
class Target;

namespace Internal {

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

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

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

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    Utils::Id id;
    QString displayName;
    CustomParserExpression error;
    CustomParserExpression warning;
};

class CustomParser : public ProjectExplorer::OutputTaskParser
{
public:
    CustomParser(const CustomParserSettings &settings = CustomParserSettings());

    void setSettings(const CustomParserSettings &settings);

    static CustomParser *createFromId(Utils::Id id);
    static Utils::Id id();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    Result hasMatch(const QString &line, CustomParserExpression::CustomParserChannel channel,
                    const CustomParserExpression &expression, Task::TaskType taskType);
    Result parseLine(const QString &rawLine, CustomParserExpression::CustomParserChannel channel);

    CustomParserExpression m_error;
    CustomParserExpression m_warning;
};

class CustomParsersSelectionWidget : public Utils::DetailsWidget
{
    Q_OBJECT
public:
    CustomParsersSelectionWidget(QWidget *parent = nullptr);

    void setSelectedParsers(const QList<Utils::Id> &parsers);
    QList<Utils::Id> selectedParsers() const;

signals:
    void selectionChanged();

private:
    void updateSummary();
};

class CustomParsersAspect : public ProjectConfigurationAspect
{
    Q_OBJECT
public:
    CustomParsersAspect(Target *target);

    void setParsers(const QList<Utils::Id> &parsers) { m_parsers = parsers; }
    const QList<Utils::Id> parsers() const { return m_parsers; }

private:
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    QList<Utils::Id> m_parsers;
};

} // namespace Internal
} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Internal::CustomParserExpression::CustomParserChannel);
