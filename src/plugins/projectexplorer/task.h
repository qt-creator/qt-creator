// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/id.h>
#include <utils/filepath.h>

#include <QIcon>
#include <QMetaType>
#include <QStringList>
#include <QTextLayout>

namespace TextEditor { class TextMark; }

namespace ProjectExplorer {

class TaskHub;

// Documentation inside.
class PROJECTEXPLORER_EXPORT Task
{
public:
    // Note: DisruptingError requests a pop-up of the issues pane and will be converted to
    //       Error afterwards.
    enum TaskType : char {
        Unknown,
        Error,
        Warning,
        DisruptingError,
    };

    enum DescriptionTag { WithSummary = 1, WithLinks = 2 };
    using DescriptionTags = QFlags<DescriptionTag>;

    Task() = default;
    Task(TaskType type, const QString &description,
         const Utils::FilePath &file, int line, Utils::Id category,
         const QIcon &icon = QIcon());

    static Task compilerMissingTask();

    bool isNull() const;
    void clear();
    QString description(DescriptionTags tags = WithSummary) const;
    QString formattedDescription(DescriptionTags tags, const QString &extraHeading = {}) const;

    void addLinkDetail(
        const QString &link, const QString &linkText = {}, int linkPos = 0, int linkLength = -1);

    void setFile(const Utils::FilePath &file);
    bool hasFile() const { return !m_file.isEmpty(); }
    Utils::FilePath file() const { return m_file; }
    Utils::FilePaths fileCandidates() const { return m_fileCandidates; }

    unsigned id() const { return m_id; }
    Utils::Id category() const { return m_category; }

    bool isError() const { return m_type == Error; }
    bool isWarning() const { return m_type == Warning; }
    bool hasKnownType() const { return m_type != Unknown; }
    TaskType type() const { return m_type; }
    void setType(TaskType type);

    const QStringList &details() const { return m_details; }
    bool hasDetails() const { return !m_details.isEmpty(); }
    void addToDetails(const QString &line) { m_details << line; }
    void clearDetails() { m_details.clear(); }
    void setDetails(const QStringList &details) { m_details = details; }

    const QList<QTextLayout::FormatRange> &formats() const { return m_formats; }
    void setFormats(const QList<QTextLayout::FormatRange> &formats) { m_formats = formats; }

    int line() const;
    void setLine(int line);
    void updateLine(int line) { m_movedLine = line; }

    int column() const { return m_column; }
    void setColumn(int col) { m_column = col; }

    QString summary() const { return m_summary; }
    void setSummary(const QString &summary) { m_summary = summary; }
    void addToSummary(const QString &s) { m_summary += s; }

    QString origin() const { return m_origin; }
    void setOrigin(const QString &origin) { m_origin = origin; }

    QIcon icon() const;
    void setIcon(const QIcon &icon);

    bool isFlashworthy() const { return m_flashworthy; }
    void preventFlashing() { m_flashworthy = false; }

    bool shouldCreateTextMark() const { return m_addTextMark; }
    void preventTextMarkCreation() { m_addTextMark = false; }
    void createTextMarkIfApplicable();
    bool hasMark() const { return m_mark.get(); }

    friend PROJECTEXPLORER_EXPORT bool operator==(const Task &t1, const Task &t2);
    friend PROJECTEXPLORER_EXPORT bool operator<(const Task &a, const Task &b);
    friend PROJECTEXPLORER_EXPORT size_t qHash(const Task &task);

private:
    unsigned int m_id = 0;
    TaskType m_type = Unknown;
    QString m_summary;
    QStringList m_details;
    Utils::FilePath m_file;
    Utils::FilePaths m_fileCandidates;
    int m_line = -1;
    int m_movedLine = -1; // contains a line number if the line was moved in the editor
    int m_column = 0;
    Utils::Id m_category;
    QString m_origin;
    bool m_flashworthy = true;
    bool m_addTextMark = true;

    // Having a container of QTextLayout::FormatRange in Task isn't that great
    // It would be cleaner to split up the text into
    // the logical hunks and then assemble them again
    // (That is different consumers of tasks could show them in
    // different ways!)
    // But then again, the wording of the text most likely
    // doesn't work if you split it up, nor are our parsers
    // anywhere near being that good
    QList<QTextLayout::FormatRange> m_formats;

    std::shared_ptr<TextEditor::TextMark> m_mark;
    mutable QIcon m_icon;
    static unsigned int s_nextId;
};

class PROJECTEXPLORER_EXPORT CompileTask : public Task
{
public:
    CompileTask(TaskType type,
                 const QString &description,
                 const Utils::FilePath &file = {},
                 int line = -1,
                 int column = 0);
};

class PROJECTEXPLORER_EXPORT BuildSystemTask : public Task
{
public:
    BuildSystemTask(TaskType type,
                    const QString &description,
                    const Utils::FilePath &file = {},
                    int line = -1);
};

class PROJECTEXPLORER_EXPORT DeploymentTask : public Task
{
public:
    DeploymentTask(TaskType type, const QString &description);
};

class PROJECTEXPLORER_EXPORT OtherTask : public Task
{
public:
    OtherTask(TaskType type, const QString &description);
};

using Tasks = QList<Task>;

PROJECTEXPLORER_EXPORT QString toHtml(const Tasks &issues);
PROJECTEXPLORER_EXPORT bool containsType(const Tasks &issues, Task::TaskType);

} // ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Task)
