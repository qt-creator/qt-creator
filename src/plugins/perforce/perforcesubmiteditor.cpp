/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "perforcesubmiteditor.h"
#include "perforcesubmiteditorwidget.h"
#include "perforceplugin.h"
#include "perforceconstants.h"

#include <vcsbase/submitfilemodel.h>
#include <utils/qtcassert.h>

#include <QDebug>

namespace Perforce {
namespace Internal {

enum { FileSpecRole = Qt::UserRole + 1 };

PerforceSubmitEditor::PerforceSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters, QWidget *parent) :
    VcsBaseSubmitEditor(parameters, new PerforceSubmitEditorWidget(parent)),
    m_fileModel(new VcsBase::SubmitFileModel(this))
{
    setDisplayName(tr("Perforce Submit"));
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
    if (Perforce::Constants::debug)
        qDebug() << Q_FUNC_INFO << text;
    return text.toLocal8Bit();
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

    const QString newLine = QString(QLatin1Char('\n'));
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
    const QString newLine = QString(QLatin1Char('\n'));
    const QString tab = QString(QLatin1Char('\t'));

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
