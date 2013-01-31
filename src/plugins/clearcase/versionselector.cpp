/**************************************************************************
**
** Copyright (c) 2013 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "versionselector.h"
#include "ui_versionselector.h"

#include <QRegExp>
#include <QTextStream>

namespace ClearCase {
namespace Internal {

VersionSelector::VersionSelector(const QString &fileName, const QString &message, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VersionSelector)
{
    ui->setupUi(this);
    ui->headerLabel->setText(ui->headerLabel->text().arg(fileName));
    ui->loadedText->setHtml(tr("<html><head/><body><p><b>NOTE: You will not be able to check in "
                               "this file without merging the changes (not supported by the "
                               "plugin)</b></p></body></html>"));
    m_stream = new QTextStream(message.toLocal8Bit(), QIODevice::ReadOnly | QIODevice::Text);
    QString line;
    while (!m_stream->atEnd() && !line.contains(QLatin1String("1) Loaded version")))
        line = m_stream->readLine();
    if (!readValues())
        return;
    ui->loadedLabel->setText(m_versionID);
    ui->loadedCreatedByLabel->setText(m_createdBy);
    ui->loadedCreatedOnLabel->setText(m_createdOn);
    ui->loadedText->insertPlainText(m_message + QLatin1Char(' '));

    line = m_stream->readLine(); // 2) Version after update
    if (!readValues())
        return;
    ui->updatedLabel->setText(m_versionID);
    ui->updatedCreatedByLabel->setText(m_createdBy);
    ui->updatedCreatedOnLabel->setText(m_createdOn);
    ui->updatedText->setPlainText(m_message);
}

VersionSelector::~VersionSelector()
{
    delete m_stream;
    delete ui;
}

bool VersionSelector::readValues()
{
    QString line;
    line = m_stream->readLine();
    QRegExp id(QLatin1String("Version ID: (.*)"));
    if (id.indexIn(line) == -1)
        return false;
    m_versionID = id.cap(1);
    line = m_stream->readLine();
    QRegExp owner(QLatin1String("Created by: (.*)"));
    if (owner.indexIn(line) == -1)
        return false;
    m_createdBy = owner.cap(1);
    line = m_stream->readLine();
    QRegExp dateTimeRE(QLatin1String("Created on: (.*)"));
    if (dateTimeRE.indexIn(line) == -1)
        return false;
    m_createdOn = dateTimeRE.cap(1);
    QStringList messageLines;
    do
    {
        line = m_stream->readLine().trimmed();
        if (line.isEmpty())
            break;
        messageLines << line;
    } while (!m_stream->atEnd());
    m_message = messageLines.join(QLatin1String(" "));
    return true;
}

bool VersionSelector::isUpdate() const
{
    return (ui->updatedRadioButton->isChecked());
}

} // namespace Internal
} // namespace ClearCase
