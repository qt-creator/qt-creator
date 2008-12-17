/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "perforcesubmiteditor.h"
#include "perforcesubmiteditorwidget.h"
#include "perforceplugin.h"
#include "perforceconstants.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>

namespace Perforce {
namespace Internal {

PerforceSubmitEditor::PerforceSubmitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters, QWidget *parent) :
    VCSBaseSubmitEditor(parameters, new PerforceSubmitEditorWidget(parent))
{
    setDisplayName(tr("Perforce Submit"));
}

PerforceSubmitEditorWidget *PerforceSubmitEditor::submitEditorWidget()
{
    return static_cast<PerforceSubmitEditorWidget *>(widget());
}

QStringList PerforceSubmitEditor::vcsFileListToFileList(const QStringList &rawList) const
{
    QStringList rc;
    foreach (const QString &rf, rawList)
        rc.push_back(fileFromChangeLine(rf));
    return rc;
}

QString PerforceSubmitEditor::fileContents() const
{
    const_cast<PerforceSubmitEditor*>(this)->updateEntries();
    QString text;
    QTextStream out(&text);
    QMapIterator<QString, QString> it(m_entries);
    while (it.hasNext()) {
        it.next();
        out << it.key() << ":" << it.value();
    }
    if (Perforce::Constants::debug)
        qDebug() << Q_FUNC_INFO << text;
    return text;
}

bool PerforceSubmitEditor::setFileContents(const QString &contents)
{
    if (Perforce::Constants::debug)
        qDebug() << Q_FUNC_INFO << contents;
    if (!parseText(contents))
        return false;
    updateFields();
    return true;
}

bool PerforceSubmitEditor::parseText(QString text)
{
    const QRegExp formField(QLatin1String("^\\S+:"));
    const QString newLine = QString(QLatin1Char('\n'));

    int match;
    int matchLen;
    QTextStream stream(&text, QIODevice::ReadOnly);
    QString line;
    QString key;
    QString value;
    line = stream.readLine();
    while (!stream.atEnd()) {
        match = formField.indexIn(line);
        if (match == 0) {
            matchLen = formField.matchedLength();
            key = line.left(matchLen-1);
            value = line.mid(matchLen) + newLine;
            while (!stream.atEnd()) {
                line = stream.readLine();
                if (formField.indexIn(line) != -1)
                    break;
                value += line + newLine;
            }
            m_entries.insert(key, value);
        } else {
            line = stream.readLine();
        }
    }
    return true;
}

void PerforceSubmitEditor::restrictToProjectFiles(const QStringList &knownProjectFiles)
{
    QStringList allFiles = submitEditorWidget()->fileList();
    const int oldSize = allFiles.size();
    for (int i = oldSize - 1; i >= 0; i--)
        if (!knownProjectFiles.contains(fileFromChangeLine(allFiles.at(i))))
            allFiles.removeAt(i);
    if (allFiles.size() != oldSize)
        submitEditorWidget()->setFileList(allFiles);
    if (Perforce::Constants::debug)
        qDebug() << Q_FUNC_INFO << oldSize << "->" << allFiles.size();
}

QString PerforceSubmitEditor::fileFromChangeLine(const QString &line)
{
    QString rc = line;
    // " foo.cpp#add"
    const int index = rc.lastIndexOf(QLatin1Char('#'));
    if (index != -1)
        rc.truncate(index);
    return rc.trimmed();
}

void PerforceSubmitEditor::updateFields()
{
    PerforceSubmitEditorWidget *widget = submitEditorWidget();
    widget->setData(m_entries.value(QLatin1String("Change")).trimmed(),
                    m_entries.value(QLatin1String("Client")).trimmed(),
                    m_entries.value(QLatin1String("User")).trimmed());

    const QString newLine = QString(QLatin1Char('\n'));
    QStringList lines = m_entries.value(QLatin1String("Description")).split(newLine);
    lines.removeFirst(); // that is the line break after 'Description:'
    lines.removeLast(); // that is the empty line at the end

    const QRegExp leadingTabPattern = QRegExp(QLatin1String("^\\t"));
    QTC_ASSERT(leadingTabPattern.isValid(), /**/);

    lines.replaceInStrings(leadingTabPattern, QString());
    widget->setDescriptionText(lines.join(newLine));

    lines = m_entries.value(QLatin1String("Files")).split(newLine);
    lines.replaceInStrings(leadingTabPattern, QString());
    QStringList fileList;
    foreach (const QString &line, lines)
        if (!line.isEmpty())
            fileList.push_back(line);
    widget->setFileList(fileList);
}

void PerforceSubmitEditor::updateEntries()
{
    const QString newLine = QString(QLatin1Char('\n'));
    const QString tab = QString(QLatin1Char('\t'));

    QStringList lines = submitEditorWidget()->trimmedDescriptionText().split(newLine);
    while (lines.last().isEmpty())
        lines.removeLast();
    // Description
    lines.replaceInStrings(QRegExp(QLatin1String("^")), tab);
    m_entries.insert(QLatin1String("Description"), newLine + lines.join(newLine) + QLatin1String("\n\n"));
    QString files = newLine;
    // Files
    const QStringList fileList = submitEditorWidget()->fileList();
    const int count = fileList.size();
    for (int i = 0; i < count; i++) {
        files += tab;
        files += fileList.at(i);
        files += newLine;
    }
    files += newLine;
    m_entries.insert(QLatin1String("Files"), files);
}

} // Internal
} // Perforce
