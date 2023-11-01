// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filewizardpage.h"

#include "utilstr.h"
#include "wizard.h" // TODO: only because of SHORT_TITLE_PROPERTY

/*!
  \class Utils::FileWizardPage
  \inmodule QtCreator

  \brief The FileWizardPage class is a standard wizard page for a single file
  letting the user choose name
  and path.

  The name and path labels can be changed. By default they are simply "Name:"
  and "Path:".
*/

#include <utils/filenamevalidatinglineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QApplication>
#include <QFormLayout>
#include <QLabel>

namespace Utils {

class FileWizardPagePrivate
{
public:
    FileWizardPagePrivate() = default;
    bool m_complete = false;

    QLabel *m_defaultSuffixLabel;
    QLabel *m_nameLabel;
    FileNameValidatingLineEdit *m_nameLineEdit;
    QLabel *m_pathLabel;
    PathChooser *m_pathChooser;
};

FileWizardPage::FileWizardPage(QWidget *parent) :
    WizardPage(parent),
    d(new FileWizardPagePrivate)
{
    setTitle(Tr::tr("Choose the Location"));

    d->m_defaultSuffixLabel = new QLabel;
    d->m_nameLabel = new QLabel;
    d->m_nameLineEdit = new FileNameValidatingLineEdit;
    d->m_nameLineEdit->setObjectName("nameLineEdit"); // used by Squish
    d->m_pathLabel = new QLabel;
    d->m_pathChooser = new PathChooser;
    d->m_pathChooser->setObjectName("fullPathChooser"); // used by Squish
    d->m_pathChooser->setExpectedKind(PathChooser::Directory);

    d->m_nameLabel->setText(Tr::tr("File name:"));
    d->m_pathLabel->setText(Tr::tr("Path:"));

    using namespace Layouting;

    Form {
        empty, d->m_defaultSuffixLabel, br,
        d->m_nameLabel, d->m_nameLineEdit, br,
        d->m_pathLabel, d->m_pathChooser
    }.attachTo(this);

    connect(d->m_pathChooser, &PathChooser::validChanged,
            this, &FileWizardPage::slotValidChanged);
    connect(d->m_nameLineEdit, &FancyLineEdit::validChanged,
            this, &FileWizardPage::slotValidChanged);

    connect(d->m_pathChooser, &PathChooser::returnPressed,
            this, &FileWizardPage::slotActivated);
    connect(d->m_nameLineEdit, &FancyLineEdit::validReturnPressed,
            this, &FileWizardPage::slotActivated);

    setProperty(SHORT_TITLE_PROPERTY, Tr::tr("Location"));

    registerFieldWithName(QLatin1String("Path"), d->m_pathChooser, "path", SIGNAL(textChanged(QString)));
    registerFieldWithName(QLatin1String("FileName"), d->m_nameLineEdit);
}

FileWizardPage::~FileWizardPage()
{
    delete d;
}

QString FileWizardPage::fileName() const
{
    return d->m_nameLineEdit->text();
}

FilePath FileWizardPage::filePath() const
{
    return d->m_pathChooser->filePath();
}

void FileWizardPage::setFilePath(const FilePath &filePath)
{
    d->m_pathChooser->setFilePath(filePath);
}

QString FileWizardPage::path() const
{
    return d->m_pathChooser->filePath().toString();
}

void FileWizardPage::setPath(const QString &path)
{
    d->m_pathChooser->setFilePath(FilePath::fromString(path));
}

void FileWizardPage::setFileName(const QString &name)
{
    d->m_nameLineEdit->setText(name);
}

void FileWizardPage::setAllowDirectoriesInFileSelector(bool allow)
{
    d->m_nameLineEdit->setAllowDirectories(allow);
}

bool FileWizardPage::isComplete() const
{
    return d->m_complete;
}

void FileWizardPage::setFileNameLabel(const QString &label)
{
    d->m_nameLabel->setText(label);
}

void FileWizardPage::setPathLabel(const QString &label)
{
    d->m_pathLabel->setText(label);
}

void FileWizardPage::setDefaultSuffix(const QString &suffix)
{
    if (suffix.isEmpty()) {
        const auto layout = qobject_cast<QFormLayout *>(this->layout());
        if (layout->rowCount() == 3)
            layout->removeRow(0);
    } else {
        d->m_defaultSuffixLabel->setText(
            Tr::tr("The default suffix if you do not explicitly specify a file extension is \".%1\".")
                .arg(suffix));
    }
}

bool FileWizardPage::forceFirstCapitalLetterForFileName() const
{
    return d->m_nameLineEdit->forceFirstCapitalLetter();
}

void FileWizardPage::setForceFirstCapitalLetterForFileName(bool b)
{
    d->m_nameLineEdit->setForceFirstCapitalLetter(b);
}

void FileWizardPage::slotValidChanged()
{
    const bool newComplete = d->m_pathChooser->isValid() && d->m_nameLineEdit->isValid();
    if (newComplete != d->m_complete) {
        d->m_complete = newComplete;
        emit completeChanged();
    }
}

void FileWizardPage::slotActivated()
{
    if (d->m_complete)
        emit activated();
}

bool FileWizardPage::validateBaseName(const QString &name, QString *errorMessage /* = 0*/)
{
    return FileNameValidatingLineEdit::validateFileName(name, false, errorMessage);
}

} // namespace Utils
