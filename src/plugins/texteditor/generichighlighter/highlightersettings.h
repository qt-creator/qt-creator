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

#include <QString>
#include <QStringList>
#include <QList>
#include <QRegExp>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

class HighlighterSettings
{
public:
    HighlighterSettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, QSettings *s);

    void setDefinitionFilesPath(const QString &path) { m_definitionFilesPath = path; }
    const QString &definitionFilesPath() const { return m_definitionFilesPath; }

    void setFallbackDefinitionFilesPath(const QString &path){ m_fallbackDefinitionFilesPath = path;}
    const QString &fallbackDefinitionFilesPath() const { return m_fallbackDefinitionFilesPath; }

    void setUseFallbackLocation(const bool use) { m_useFallbackLocation = use; }
    bool useFallbackLocation() const { return m_useFallbackLocation; }

    void setIgnoredFilesPatterns(const QString &patterns);
    QString ignoredFilesPatterns() const;
    bool isIgnoredFilePattern(const QString &fileName) const;

    bool equals(const HighlighterSettings &highlighterSettings) const;

private:
    void assignDefaultIgnoredPatterns();
    void assignDefaultDefinitionsPath();

    void setExpressionsFromList(const QStringList &patterns);
    QStringList listFromExpressions() const;

    bool m_useFallbackLocation;
    QString m_definitionFilesPath;
    QString m_fallbackDefinitionFilesPath;
    QList<QRegExp> m_ignoredFiles;
};

inline bool operator==(const HighlighterSettings &a, const HighlighterSettings &b)
{ return a.equals(b); }

inline bool operator!=(const HighlighterSettings &a, const HighlighterSettings &b)
{ return !a.equals(b); }

namespace Internal {
QString findFallbackDefinitionsLocation();
}

} // namespace TextEditor
