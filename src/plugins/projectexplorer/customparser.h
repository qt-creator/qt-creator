// Copyright (C) 2016 Andre Hartmann.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ioutputparser.h"
#include "projectconfiguration.h"

#include <projectexplorer/task.h>
#include <utils/detailswidget.h>

#include <QRegularExpression>
#include <QVariantMap>

namespace ProjectExplorer {
class Target;

class PROJECTEXPLORER_EXPORT CustomParserExpression
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

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

private:
    QRegularExpression m_regExp;
    CustomParserExpression::CustomParserChannel m_channel = ParseBothChannels;
    QString m_example;
    int m_fileNameCap = 1;
    int m_lineNumberCap = 2;
    int m_messageCap = 3;
};

class PROJECTEXPLORER_EXPORT CustomParserSettings
{
public:
    bool operator ==(const CustomParserSettings &other) const;
    bool operator !=(const CustomParserSettings &other) const { return !operator==(other); }

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    Utils::Id id;
    QString displayName;
    CustomParserExpression error;
    CustomParserExpression warning;
};

class PROJECTEXPLORER_EXPORT CustomParsersAspect : public Utils::BaseAspect
{
    Q_OBJECT
public:
    CustomParsersAspect(Target *target);

    void setParsers(const QList<Utils::Id> &parsers) { m_parsers = parsers; }
    QList<Utils::Id> parsers() const { return m_parsers; }

    struct Data : BaseAspect::Data
    {
        QList<Utils::Id> parsers;
    };

private:
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    QList<Utils::Id> m_parsers;
};

namespace Internal {

class CustomParser : public ProjectExplorer::OutputTaskParser
{
public:
    CustomParser(const CustomParserSettings &settings = CustomParserSettings());

    void setSettings(const CustomParserSettings &settings);

    static CustomParser *createFromId(Utils::Id id);

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

} // namespace Internal
} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::CustomParserExpression::CustomParserChannel);
