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

#include "perforcesubmiteditor.h"
#include "perforcesubmiteditorwidget.h"
#include "perforceplugin.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submitfilemodel.h>
#include <utils/qtcassert.h>

namespace Perforce {
namespace Internal {

enum { FileSpecRole = Qt::UserRole + 1 };

PerforceSubmitEditor::PerforceSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters) :
    VcsBaseSubmitEditor(parameters, new PerforceSubmitEditorWidget),
    m_fileModel(new VcsBase::SubmitFileModel(this))
{
    document()->setPreferredDisplayName(tr("Perforce Submit"));
    setFileModel(m_fileModel);
}

PerforceSubmitEditorWidget *PerforceSubmitEditor::submitEditorWidget()
{
    return static_cast<PerforceSubmitEditorWidget *>(widget());
}

QByteArray PerforceSubmitEditor::fileContents() const
{
    const_cast<PerforceSubmitEditor*>(this)->updateEntries();
    QString text;
    QTextStream out(&text);
    QMapIterator<QString, QString> it(m_entries);
    while (it.hasNext()) {
        it.next();
        out << it.key() << ":" << it.value();
    }
    return text.toLocal8Bit();
}

bool PerforceSubmitEditor::setFileContents(const QByteArray &contents)
{
    if (!parseText(QString::fromUtf8(contents)))
        return false;
    updateFields();
    return true;
}

bool PerforceSubmitEditor::parseText(QString text)
{
    QRegExp formField(QLatin1String("^\\S+:"));
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
    m_fileModel->filterFiles(knownProjectFiles);
}

void PerforceSubmitEditor::updateFields()
{
    PerforceSubmitEditorWidget *widget = submitEditorWidget();
    widget->setData(m_entries.value(QLatin1String("Change")).trimmed(),
                    m_entries.value(QLatin1String("Client")).trimmed(),
                    m_entries.value(QLatin1String("User")).trimmed());

    const QChar newLine = QLatin1Char('\n');
    QStringList lines = m_entries.value(QLatin1String("Description")).split(newLine);
    lines.removeFirst(); // that is the line break after 'Description:'
    lines.removeLast(); // that is the empty line at the end

    const QRegExp leadingTabPattern = QRegExp(QLatin1String("^\\t"));
    QTC_CHECK(leadingTabPattern.isValid());

    lines.replaceInStrings(leadingTabPattern, QString());
    widget->setDescriptionText(lines.join(newLine));

    lines = m_entries.value(QLatin1String("Files")).split(newLine);
    // split up "file#add" and store complete spec line as user data
    foreach (const QString &specLine, lines) {
        const QStringList list = specLine.split(QLatin1Char('#'));
        if (list.size() == 2) {
            const QString file = list.at(0).trimmed();
            const QString state = list.at(1).trimmed();
            m_fileModel->addFile(file, state).at(0)->setData(specLine, FileSpecRole);
        }
    }
}

void PerforceSubmitEditor::updateEntries()
{
    const QChar newLine = QLatin1Char('\n');
    const QChar tab = QLatin1Char('\t');

    QStringList lines = submitEditorWidget()->descriptionText().split(newLine);

    while (!lines.empty() && lines.last().isEmpty())
            lines.removeLast();
    // Description
    lines.replaceInStrings(QRegExp(QLatin1String("^")), tab);
    m_entries.insert(QLatin1String("Description"), newLine + lines.join(newLine) + QLatin1String("\n\n"));
    QString files = newLine;
    // Re-build the file spec '<tab>file#add' from the user data
    const int count = m_fileModel->rowCount();
    for (int r = 0; r < count; r++) {
        const QStandardItem *item = m_fileModel->item(r, 0);
        if (item->checkState() == Qt::Checked) {
            files += item->data(FileSpecRole).toString();
            files += newLine;
        }
    }
    files += newLine;
    m_entries.insert(QLatin1String("Files"), files);
}

} // Internal
} // Perforce
