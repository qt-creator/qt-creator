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

#include "projectexplorer_export.h"
#include "buildstep.h"

#include <utils/fileutils.h>
#include <utils/outputformat.h>

#include <functional>

namespace Utils { class FileInProjectFinder; }

namespace ProjectExplorer {
class Task;

class PROJECTEXPLORER_EXPORT OutputTaskParser : public QObject
{
    Q_OBJECT
public:
    OutputTaskParser();
    ~OutputTaskParser() override;

    void addSearchDir(const Utils::FilePath &dir);
    void dropSearchDir(const Utils::FilePath &dir);
    const Utils::FilePaths searchDirectories() const;

    enum class Status { Done, InProgress, NotHandled };
    virtual Status handleLine(const QString &line, Utils::OutputFormat type) = 0;

    virtual bool hasFatalErrors() const { return false; }
    virtual void flush() {}

    void setRedirectionDetector(const OutputTaskParser *detector);
    bool needsRedirection() const;
    virtual bool hasDetectedRedirection() const { return false; }

    void setFileFinder(Utils::FileInProjectFinder *finder);

#ifdef WITH_TESTS
    void skipFileExistsCheck();
#endif

signals:
    void newSearchDir(const Utils::FilePath &dir);
    void searchDirExpired(const Utils::FilePath &dir);
    void addTask(const ProjectExplorer::Task &task, int linkedOutputLines = 0, int skipLines = 0);

protected:
    static QString rightTrimmed(const QString &in);
    Utils::FilePath absoluteFilePath(const Utils::FilePath &filePath);

private:
    class Private;
    Private * const d;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT IOutputParser : public QObject
{
    Q_OBJECT
public:
    IOutputParser();
    ~IOutputParser() override;

    void handleStdout(const QString &data);
    void handleStderr(const QString &data);

    bool hasFatalErrors() const;

    using Filter = std::function<QString(const QString &)>;
    void addFilter(const Filter &filter);

    // Forwards to line parsers. Add those before.
    void addSearchDir(const Utils::FilePath &dir);
    void dropSearchDir(const Utils::FilePath &dir);

    void flush();
    void clear();

    void addLineParser(OutputTaskParser *parser);
    void addLineParsers(const QList<OutputTaskParser *> &parsers);
    void setLineParsers(const QList<OutputTaskParser *> &parsers);

    void setFileFinder(const Utils::FileInProjectFinder &finder);

#ifdef WITH_TESTS
    QList<OutputTaskParser *> lineParsers() const;
#endif

signals:
    void addTask(const ProjectExplorer::Task &task, int linkedOutputLines = 0, int skipLines = 0);

private:
    void handleLine(const QString &line, Utils::OutputFormat type);
    QString filteredLine(const QString &line) const;
    void setupLineParser(OutputTaskParser *parser);
    Utils::OutputFormat outputTypeForParser(const OutputTaskParser *parser,
                                            Utils::OutputFormat type) const;

    class OutputChannelState;
    class IOutputParserPrivate;
    IOutputParserPrivate * const d;
};

} // namespace ProjectExplorer
