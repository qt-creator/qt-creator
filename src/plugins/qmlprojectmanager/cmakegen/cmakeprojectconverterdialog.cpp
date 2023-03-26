// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprojectconverterdialog.h"
#include "../qmlprojectmanagertr.h"

#include <coreplugin/documentmanager.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>

using namespace Utils;

namespace QmlProjectManager {
namespace GenerateCmake {

const QRegularExpression projectNameRegexp("^(?!(import))(?!(QtQml))(?!(QtQuick))(?:[A-Z][a-zA-Z0-9-_]*)$");

static bool projectNameValidationFunction(FancyLineEdit *editor, QString *)
{
    return editor->text().count(projectNameRegexp);
}

static bool dirValidationFunction(FancyLineEdit *editor, QString *)
{
    return FilePath::fromString(editor->text()).isWritableDir();
}

const QString EXPLANATION_TEXT = Tr::tr("This process creates a copy of the existing project. The new project's folder structure is adjusted for CMake build process and necessary related new files are generated.\n\nThe new project can be opened in Qt Creator using the main CMakeLists.txt file.");
const QString PROJECT_NAME_LABEL = Tr::tr("Name:");
const QString PARENT_DIR_LABEL = Tr::tr("Create in:");

CmakeProjectConverterDialog::CmakeProjectConverterDialog(const QmlProjectManager::QmlProject *oldProject)
    : QDialog()
{
    const FilePath defaultDir = Core::DocumentManager::projectsDirectory();
    const QString defaultName = uniqueProjectName(defaultDir, oldProject->displayName());

    QLabel *mainLabel = new QLabel(EXPLANATION_TEXT, this);
    mainLabel->setWordWrap(true);

    mainLabel->setMargin(20);
    mainLabel->setMinimumWidth(600);

    m_errorLabel = new InfoLabel();
    m_errorLabel->setType(InfoLabel::InfoType::None);

    m_nameEditor = new FancyLineEdit();
    m_nameEditor->setValidationFunction(projectNameValidationFunction);
    m_nameEditor->setText(defaultName);

    m_dirSelector = new PathChooser();
    m_dirSelector->setExpectedKind(PathChooser::Directory);
    m_dirSelector->setValidationFunction(dirValidationFunction);
    m_dirSelector->setFilePath(defaultDir);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setDefault(true);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_nameEditor, &FancyLineEdit::textChanged, this, &CmakeProjectConverterDialog::pathValidChanged);
    connect(m_dirSelector->lineEdit(), &FancyLineEdit::textChanged, this, &CmakeProjectConverterDialog::pathValidChanged);

    QGroupBox *form = new QGroupBox();
    QFormLayout *formLayout = new QFormLayout(form);
    formLayout->addRow(PROJECT_NAME_LABEL, m_nameEditor);
    formLayout->addRow(PARENT_DIR_LABEL, m_dirSelector);

    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(mainLabel);
    dialogLayout->addWidget(form);
    dialogLayout->addWidget(m_errorLabel);
    dialogLayout->addWidget(buttons);

    pathValidChanged();
}

void CmakeProjectConverterDialog::pathValidChanged()
{
    bool valid = isValid();

    if (valid)
        m_newProjectDir = m_dirSelector->filePath().pathAppended(m_nameEditor->text());
    else
        m_newProjectDir = FilePath();

    const QString error = errorText();
    m_errorLabel->setType(error.isEmpty() ? InfoLabel::None : InfoLabel::Warning);
    m_errorLabel->setText(error);
    m_okButton->setEnabled(valid);
}

const FilePath CmakeProjectConverterDialog::newPath() const
{
    return m_newProjectDir;
}

const QStringList blackListedStarts = {"import","QtQml","QtQuick"};

const QString CmakeProjectConverterDialog::startsWithBlacklisted(const QString &text) const
{
    for (const QString &badWord : blackListedStarts) {
        if (text.startsWith(badWord))
            return badWord;
    }

    return {};
}

const QString ERROR_TEXT_NAME_EMPTY = Tr::tr("Name is empty.");
const QString ERROR_TEXT_NAME_BAD_START = Tr::tr("Name must not start with \"%1\".");
const QString ERROR_TEXT_NAME_LOWERCASE_START = Tr::tr("Name must begin with a capital letter");
const QString ERROR_TEXT_NAME_BAD_CHARACTERS = Tr::tr("Name must contain only letters, numbers or characters - _.");

const QString ERROR_DIR_NOT_DIR = Tr::tr("Target is not a directory.");
const QString ERROR_DIR_NOT_WRITABLE = Tr::tr("Cannot write to target directory.");
const QString ERROR_DIR_EXISTS = Tr::tr("Project directory already exists.");

const QString CmakeProjectConverterDialog::errorText() const
{
    QString text;

    if (!m_nameEditor->isValid()) {
        QString name = m_nameEditor->text();

        if (name.isEmpty())
            return ERROR_TEXT_NAME_EMPTY;

        const QString badStart = startsWithBlacklisted(text);
        if (!badStart.isEmpty())
            return ERROR_TEXT_NAME_BAD_START.arg(badStart);

        if (name[0].isLower())
            return ERROR_TEXT_NAME_LOWERCASE_START;

        return ERROR_TEXT_NAME_BAD_CHARACTERS;

    }

    if (!m_dirSelector->isValid()) {
        FilePath path = m_dirSelector->filePath();
        if (!path.isDir())
            return ERROR_DIR_NOT_DIR;
        if (!path.isWritableDir())
            return ERROR_DIR_NOT_WRITABLE;
    }

    if (m_dirSelector->filePath().pathAppended(m_nameEditor->text()).exists())
        return ERROR_DIR_EXISTS;

    return text;
}

const QString CmakeProjectConverterDialog::uniqueProjectName(const FilePath &dir, const QString &oldName) const
{
    for (unsigned i = 0; ; ++i) {
        QString name = oldName;
        if (i)
            name += QString::number(i);
        if (!dir.pathAppended(name).exists())
            return name;
    }
    return oldName;
}

bool CmakeProjectConverterDialog::isValid()
{
    FilePath newPath = m_dirSelector->filePath().pathAppended(m_nameEditor->text());
    return m_dirSelector->isValid() && m_nameEditor->isValid() && !newPath.exists();
}

} //GenerateCmake
} //QmlProjectManager
