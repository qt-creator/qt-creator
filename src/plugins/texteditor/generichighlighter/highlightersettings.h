/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef HIGHLIGHTERSETTINGS_H
#define HIGHLIGHTERSETTINGS_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QRegExp>

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

    void setAlertWhenNoDefinition(const bool alert) { m_alertWhenNoDefinition = alert; }
    bool alertWhenNoDefinition() const { return m_alertWhenNoDefinition; }

    void setUseFallbackLocation(const bool use) { m_useFallbackLocation = use; }
    bool useFallbackLocation() const { return m_useFallbackLocation; }

    void setIgnoredFilesPatterns(const QString &patterns);
    QString ignoredFilesPatterns() const;
    bool isIgnoredFilePattern(const QString &fileName) const;

    bool equals(const HighlighterSettings &highlighterSettings) const;

private:
    void assignInitialIgnoredPatterns();

    void setExpressionsFromList(const QStringList &patterns);
    QStringList listFromExpressions() const;

    bool m_alertWhenNoDefinition;
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
QString findDefinitionsLocation();
}

} // namespace TextEditor

#endif // HIGHLIGHTERSETTINGS_H
