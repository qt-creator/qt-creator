// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QString>
#include <QStringList>
#include <QList>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

class HighlighterSettings
{
public:
    HighlighterSettings() = default;

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, QSettings *s);

    void setDefinitionFilesPath(const Utils::FilePath &path) { m_definitionFilesPath = path; }
    const Utils::FilePath &definitionFilesPath() const { return m_definitionFilesPath; }

    void setIgnoredFilesPatterns(const QString &patterns);
    QString ignoredFilesPatterns() const;
    bool isIgnoredFilePattern(const QString &fileName) const;

    bool equals(const HighlighterSettings &highlighterSettings) const;

    friend bool operator==(const HighlighterSettings &a, const HighlighterSettings &b)
    { return a.equals(b); }

    friend bool operator!=(const HighlighterSettings &a, const HighlighterSettings &b)
    { return !a.equals(b); }

private:
    void assignDefaultIgnoredPatterns();
    void assignDefaultDefinitionsPath();

    void setExpressionsFromList(const QStringList &patterns);
    QStringList listFromExpressions() const;

    Utils::FilePath m_definitionFilesPath;
    QList<QRegularExpression> m_ignoredFiles;
};

} // namespace TextEditor
