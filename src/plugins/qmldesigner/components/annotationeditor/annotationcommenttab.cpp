// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationcommenttab.h"
#include "ui_annotationcommenttab.h"

#include "defaultannotations.h"

#include <qmldesignerplugin.h>
#include <richtexteditor/richtexteditor.h>
#include <projectexplorer/target.h>
#include <qmlprojectmanager/qmlproject.h>
#include <utils/qtcassert.h>

#include <QCryptographicHash>

using namespace Utils;
namespace QmlDesigner {

AnnotationCommentTab::AnnotationCommentTab(QWidget *parent)
    : QWidget(parent)
    , ui(std::make_unique<Ui::AnnotationCommentTab>())
{
    ui->setupUi(this);

    m_editor = new RichTextEditor{this};

    connect(m_editor, &RichTextEditor::insertingImage, this, [this](QString &filePath) {
        filePath = backupFile(filePath);
    });

    m_editor->setImageActionVisible(false);

    const QmlDesigner::DesignDocument *designDocument = QmlDesigner::QmlDesignerPlugin::instance()
            ->documentManager().currentDesignDocument();

    FilePath projectPath;

    Q_ASSERT(designDocument);

    if (designDocument) {
        if (designDocument->currentTarget() && designDocument->currentTarget()->project()) {
            projectPath = designDocument->currentTarget()->project()->projectFilePath();
            m_editor->setImageActionVisible(true);
        }

        if (projectPath.isEmpty()) {
            projectPath = designDocument->fileName();
            m_editor->setImageActionVisible(false);
        }

        m_editor->setDocumentBaseUrl(QUrl::fromLocalFile(projectPath.toString()));
    }

    ui->formLayout->setWidget(3, QFormLayout::FieldRole, m_editor);

    connect(ui->titleEdit, &QComboBox::currentTextChanged, this, [this](QString const &text) {
        emit titleChanged(text, this);
    });
}

AnnotationCommentTab::~AnnotationCommentTab() {}

Comment AnnotationCommentTab::currentComment() const
{
    Comment result;

    result.setTitle(ui->titleEdit->currentText().trimmed());
    result.setAuthor(ui->authorEdit->text().trimmed());
    if (defaultAnnotations() && !defaultAnnotations()->isRichText(result)) {
        result.setText(m_editor->plainText().trimmed());
    } else
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

DefaultAnnotationsModel *AnnotationCommentTab::defaultAnnotations() const
{
    return m_defaults;
}

void AnnotationCommentTab::setDefaultAnnotations(DefaultAnnotationsModel *defaults)
{
    m_defaults = defaults;
    ui->titleEdit->setModel(m_defaults);
}

QString AnnotationCommentTab::backupFile(const QString &filePath)
{
    const QmlDesigner::DesignDocument *designDocument = QmlDesigner::QmlDesignerPlugin::instance()
            ->documentManager().currentDesignDocument();

    FilePath projectFolderPath;

    Q_ASSERT(designDocument);

    if (designDocument) {
        if (designDocument->hasProject())
            projectFolderPath = designDocument->projectFolder();
        if (projectFolderPath.isEmpty())
            projectFolderPath = designDocument->fileName().parentDir();
    }
    else
        return {};

    if (!projectFolderPath.isDir())
        return {};

    const QString imageSubDir(".AnnotationImages");
    const FilePath imgDir(projectFolderPath / imageSubDir);

    if (!imgDir.exists())
        imgDir.createDir();

    QTC_ASSERT(imgDir.isDir(), return {});

    const FilePath oldFile = FilePath::fromString(filePath);
    FilePath newFile = imgDir.resolvePath(oldFile.fileName());

    QString newNameTemplate = newFile.baseName() + "_%1." + newFile.completeSuffix();

    for (size_t i = 1; true; ++i) {
        if (!newFile.exists()) {
            oldFile.copyFile(newFile);
            break;
        } else if (compareFileChecksum(oldFile.absoluteFilePath().toString(), newFile.absoluteFilePath().toString()) == 0)
            break;

        newFile = imgDir / newNameTemplate.arg(i);
    }

    return newFile.relativeChildPath(projectFolderPath).toString();
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
