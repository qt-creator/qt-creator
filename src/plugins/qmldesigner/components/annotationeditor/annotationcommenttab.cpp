/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "annotationcommenttab.h"
#include "ui_annotationcommenttab.h"

#include "richtexteditor/richtexteditor.h"

#include <QCryptographicHash>
#include "QStringListModel"

#include "projectexplorer/session.h"
#include "projectexplorer/target.h"
#include "qmldesignerplugin.h"
#include "qmlprojectmanager/qmlproject.h"

namespace QmlDesigner {

AnnotationCommentTab::AnnotationCommentTab(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AnnotationCommentTab)
{
    ui->setupUi(this);

    m_editor = new RichTextEditor{this};

    connect(m_editor, &RichTextEditor::insertingImage, this, [this](QString &filePath) {
        filePath = backupFile(filePath);
    });

    Utils::FilePath projPath = ProjectExplorer::SessionManager::startupProject()->projectFilePath();

    m_editor->setDocumentBaseUrl(QUrl::fromLocalFile(projPath.toString()));
    m_editor->setImageActionVisible(true);

    ui->formLayout->setWidget(3, QFormLayout::FieldRole, m_editor);

    ui->titleEdit->setModel(new QStringListModel{QStringList{"Description",
                                                             "Display Condition",
                                                             "helper lines",
                                                             "position marker",
                                                             "highlight",
                                                             "project author",
                                                             "project confirmed",
                                                             "project developer",
                                                             "project distributor",
                                                             "project modified",
                                                             "project type",
                                                             "project version",
                                                             "Screen Description",
                                                             "Section",
                                                             "normalcolor",
                                                             "focuscolor",
                                                             "selectedcolor",
                                                             "pressedcolor"}});

    connect(ui->titleEdit, &QComboBox::currentTextChanged,
            this, &AnnotationCommentTab::commentTitleChanged);
}

AnnotationCommentTab::~AnnotationCommentTab()
{
    delete ui;
}

Comment AnnotationCommentTab::currentComment() const
{
    Comment result;

    result.setTitle(ui->titleEdit->currentText().trimmed());
    result.setAuthor(ui->authorEdit->text().trimmed());
    result.setText(m_editor->richText().trimmed());

    if (m_comment.sameContent(result))
        result.setTimestamp(m_comment.timestamp());
    else
        result.updateTimestamp();

    return result;
}

Comment AnnotationCommentTab::originalComment() const
{
    return m_comment;
}

void AnnotationCommentTab::setComment(const Comment &comment)
{
    m_comment = comment;
    resetUI();
}

void AnnotationCommentTab::resetUI()
{
    ui->titleEdit->setCurrentText(m_comment.title());
    ui->authorEdit->setText(m_comment.author());
    m_editor->setRichText(m_comment.deescapedText());

    if (m_comment.timestamp() > 0)
        ui->timeLabel->setText(m_comment.timestampStr());
    else
        ui->timeLabel->setText("");
}

void AnnotationCommentTab::resetComment()
{
    m_comment = currentComment();
}

void AnnotationCommentTab::commentTitleChanged(const QString &text)
{
    emit titleChanged(text, this);
}

QString AnnotationCommentTab::backupFile(const QString &filePath)
{
    const QDir projDir(
        ProjectExplorer::SessionManager::startupProject()->projectDirectory().toString());

    const QString imageSubDir(".AnnotationImages");
    const QDir imgDir(projDir.absolutePath() + QDir::separator() + imageSubDir);

    ensureDir(imgDir);

    const QFileInfo oldFile(filePath);
    QFileInfo newFile(imgDir, oldFile.fileName());

    QString newName = newFile.baseName() + "_%1." + newFile.completeSuffix();

    for (size_t i = 1; true; ++i) {
        if (!newFile.exists()) {
            QFile(oldFile.absoluteFilePath()).copy(newFile.absoluteFilePath());
            break;
        } else if (compareFileChecksum(oldFile.absoluteFilePath(),
                                       newFile.absoluteFilePath()) == 0) {
            break;
        }

        newFile.setFile(imgDir, newName.arg(i));
    }

    return projDir.relativeFilePath(newFile.absoluteFilePath());
}

void AnnotationCommentTab::ensureDir(const QDir &dir)
{
    if (!dir.exists()) {
        dir.mkdir(".");
    }
}

int AnnotationCommentTab::compareFileChecksum(const QString &firstFile, const QString &secondFile)
{
    QCryptographicHash sum1(QCryptographicHash::Md5);

    {
        QFile f1(firstFile);
        if (f1.open(QFile::ReadOnly)) {
            sum1.addData(&f1);
        }
    }

    QCryptographicHash sum2(QCryptographicHash::Md5);

    {
        QFile f2(secondFile);
        if (f2.open(QFile::ReadOnly)) {
            sum2.addData(&f2);
        }
    }

    return sum1.result().compare(sum2.result());
}

} //namespace QmlDesigner
