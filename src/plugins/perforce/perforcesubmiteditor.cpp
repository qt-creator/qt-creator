// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perforcesubmiteditor.h"
#include "perforcesubmiteditorwidget.h"
#include "perforcetr.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submitfilemodel.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Perforce::Internal {

enum { FileSpecRole = Qt::UserRole + 1 };

PerforceSubmitEditor::PerforceSubmitEditor() :
    VcsBaseSubmitEditor(new PerforceSubmitEditorWidget),
    m_fileModel(new VcsBase::SubmitFileModel(this))
{
    document()->setPreferredDisplayName(Tr::tr("Perforce Submit"));
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
    for (auto it = m_entries.cbegin(), end  = m_entries.cend(); it != end; ++it)
        out << it.key() << ":" << it.value();
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
    QRegularExpression formField(QLatin1String("^\\S+:"));
    const QString newLine = QString(QLatin1Char('\n'));

    int matchLen;
    QTextStream stream(&text, QIODevice::ReadOnly);
    QString line;
    QString key;
    QString value;
    line = stream.readLine();
    while (!stream.atEnd()) {
        const QRegularExpressionMatch match = formField.match(line);
        if (match.hasMatch()) {
            matchLen = match.capturedLength();
            key = line.left(matchLen-1);
            value = line.mid(matchLen) + newLine;
            while (!stream.atEnd()) {
                line = stream.readLine();
                if (line.indexOf(formField) != -1)
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

    const QRegularExpression leadingTabPattern("^\\t");
    QTC_CHECK(leadingTabPattern.isValid());

    lines.replaceInStrings(leadingTabPattern, QString());
    widget->setDescriptionText(lines.join(newLine));

    lines = m_entries.value(QLatin1String("Files")).split(newLine);
    // split up "file#add" and store complete spec line as user data
    for (const QString &specLine : std::as_const(lines)) {
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
    lines.replaceInStrings(QRegularExpression("^"), tab);
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

} // Perforce::Internal
