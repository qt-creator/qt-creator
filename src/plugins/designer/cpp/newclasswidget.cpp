// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "newclasswidget.h"

#include "../designertr.h"

#include <utils/classnamevalidatinglineedit.h>
#include <utils/filenamevalidatinglineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QFileDialog>
#include <QDebug>

using namespace Utils;

enum { debugNewClassWidget = 0 };

/*! \class Utils::NewClassWidget

    \brief The NewClassWidget class is a utility widget for 'New Class' wizards.

    This widget prompts the user
    to enter a class name (optionally derived from some base class) and file
    names for header, source and form files. Has some smart logic to derive
    the file names from the class name. */

namespace Designer {
namespace Internal {

struct NewClassWidgetPrivate {
    NewClassWidgetPrivate();

    QString m_headerExtension;
    QString m_sourceExtension;
    QString m_formExtension;
    bool m_valid = false;

    ClassNameValidatingLineEdit *m_classLineEdit;
    FileNameValidatingLineEdit *m_headerFileLineEdit;
    FileNameValidatingLineEdit *m_sourceFileLineEdit;
    FileNameValidatingLineEdit *m_formFileLineEdit;
    PathChooser *m_pathChooser;
};

NewClassWidgetPrivate:: NewClassWidgetPrivate() :
    m_headerExtension(QLatin1Char('h')),
    m_sourceExtension(QLatin1String("cpp")),
    m_formExtension(QLatin1String("ui"))
{
}

// --------------------- NewClassWidget
NewClassWidget::NewClassWidget(QWidget *parent) :
    QWidget(parent),
    d(new NewClassWidgetPrivate)
{
    d->m_classLineEdit = new ClassNameValidatingLineEdit;
    d->m_classLineEdit->setNamespacesEnabled(true);
    d->m_headerFileLineEdit = new FileNameValidatingLineEdit;
    d->m_sourceFileLineEdit = new FileNameValidatingLineEdit;
    d->m_formFileLineEdit = new FileNameValidatingLineEdit;
    d->m_pathChooser = new PathChooser;

    setNamesDelimiter(QLatin1String("::"));

    using namespace Layouting;
    Form {
        Tr::tr("&Class name:"), d->m_classLineEdit, br,
        Tr::tr("&Header file:"), d->m_headerFileLineEdit, br,
        Tr::tr("&Source file:"), d->m_sourceFileLineEdit, br,
        Tr::tr("&Form file:"), d->m_formFileLineEdit, br,
        Tr::tr("&Path:"), d->m_pathChooser, br,
        noMargin
    }.attachTo(this);

    connect(d->m_classLineEdit, &ClassNameValidatingLineEdit::updateFileName,
            this, &NewClassWidget::slotUpdateFileNames);
    connect(d->m_classLineEdit, &FancyLineEdit::validChanged,
            this, &NewClassWidget::slotValidChanged);
    connect(d->m_headerFileLineEdit, &FancyLineEdit::validChanged,
            this, &NewClassWidget::slotValidChanged);
    connect(d->m_sourceFileLineEdit, &FancyLineEdit::validChanged,
            this, &NewClassWidget::slotValidChanged);
    connect(d->m_formFileLineEdit, &FancyLineEdit::validChanged,
            this, &NewClassWidget::slotValidChanged);
    connect(d->m_pathChooser, &PathChooser::validChanged,
            this, &NewClassWidget::slotValidChanged);

    connect(d->m_classLineEdit, &FancyLineEdit::validReturnPressed,
            this, &NewClassWidget::slotActivated);
    connect(d->m_headerFileLineEdit, &FancyLineEdit::validReturnPressed,
            this, &NewClassWidget::slotActivated);
    connect(d->m_sourceFileLineEdit, &FancyLineEdit::validReturnPressed,
            this, &NewClassWidget::slotActivated);
    connect(d->m_formFileLineEdit, &FancyLineEdit::validReturnPressed,
            this, &NewClassWidget::slotActivated);
    connect(d->m_formFileLineEdit, &FancyLineEdit::validReturnPressed,
            this, &NewClassWidget::slotActivated);
    connect(d->m_pathChooser, &PathChooser::returnPressed,
             this, &NewClassWidget::slotActivated);
}

NewClassWidget::~NewClassWidget()
{
    delete d;
}

void NewClassWidget::setClassName(const QString &suggestedName)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << suggestedName << d->m_headerExtension << d->m_sourceExtension;
    d->m_classLineEdit->setText(
                Utils::ClassNameValidatingLineEdit::createClassName(suggestedName));
}

QString NewClassWidget::className() const
{
    return d->m_classLineEdit->text();
}

QString NewClassWidget::sourceFileName() const
{
    return d->m_sourceFileLineEdit->text();
}

QString NewClassWidget::headerFileName() const
{
    return d->m_headerFileLineEdit->text();
}

QString NewClassWidget::formFileName() const
{
    return d->m_formFileLineEdit->text();
}

FilePath NewClassWidget::filePath() const
{
    return d->m_pathChooser->filePath();
}

void NewClassWidget::setFilePath(const FilePath &path)
{
     d->m_pathChooser->setFilePath(path);
}

QString NewClassWidget::sourceExtension() const
{
    return d->m_sourceExtension;
}

void NewClassWidget::setSourceExtension(const QString &e)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << e;
    d->m_sourceExtension = fixSuffix(e);
}

QString NewClassWidget::headerExtension() const
{
    return d->m_headerExtension;
}

void NewClassWidget::setHeaderExtension(const QString &e)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << e;
    d->m_headerExtension = fixSuffix(e);
}

QString NewClassWidget::formExtension() const
{
    return d->m_formExtension;
}


void NewClassWidget::setLowerCaseFiles(bool v)
{
    d->m_classLineEdit->setLowerCaseFileName(v);
}

void NewClassWidget::setNamesDelimiter(const QString &delimiter)
{
    d->m_classLineEdit->setNamespaceDelimiter(delimiter);
}

void NewClassWidget::slotValidChanged()
{
    const bool newValid = isValid();
    if (newValid != d->m_valid) {
        d->m_valid = newValid;
        emit validChanged();
    }
}

bool NewClassWidget::isValid(QString *error) const
{
    if (!d->m_classLineEdit->isValid()) {
        if (error)
            *error = d->m_classLineEdit->errorMessage();
        return false;
    }

    if (!d->m_headerFileLineEdit->isValid()) {
        if (error)
            *error = Tr::tr("Invalid header file name: \"%1\"").
                arg(d->m_headerFileLineEdit->errorMessage());
        return false;
    }

    if (!d->m_sourceFileLineEdit->isValid()) {
        if (error)
            *error = Tr::tr("Invalid source file name: \"%1\"").
                arg(d->m_sourceFileLineEdit->errorMessage());
        return false;
    }

    if (!d->m_formFileLineEdit->isValid()) {
        if (error)
            *error = Tr::tr("Invalid form file name: \"%1\"").
                arg(d->m_formFileLineEdit->errorMessage());
        return false;
    }

    if (!d->m_pathChooser->isValid()) {
        if (error)
            *error =  d->m_pathChooser->errorMessage();
        return false;
    }
    return true;
}

void NewClassWidget::slotUpdateFileNames(const QString &baseName)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << baseName << d->m_headerExtension << d->m_sourceExtension;
    const QChar dot = QLatin1Char('.');
    d->m_sourceFileLineEdit->setText(baseName + dot + d->m_sourceExtension);
    d->m_headerFileLineEdit->setText(baseName + dot + d->m_headerExtension);
    d->m_formFileLineEdit->setText(baseName + dot + d->m_formExtension);
}

void NewClassWidget::slotActivated()
{
    if (d->m_valid)
        emit activated();
}

QString NewClassWidget::fixSuffix(const QString &suffix)
{
    QString s = suffix;
    if (s.startsWith(QLatin1Char('.')))
        s.remove(0, 1);
    return s;
}

// Utility to add a suffix to a file unless the user specified one
static QString ensureSuffix(QString f, const QString &extension)
{
    const QChar dot = QLatin1Char('.');
    if (f.contains(dot))
        return f;
    f += dot;
    f += extension;
    return f;
}

// If a non-empty name was passed, expand to directory and suffix
static FilePath expandFileName(const FilePath &dir, const QString &name, const QString &extension)
{
    if (name.isEmpty())
        return {};
    return dir / ensureSuffix(name, extension);
}

Utils::FilePaths NewClassWidget::files() const
{
    const FilePath dir = filePath();
    return {
        expandFileName(dir, headerFileName(), headerExtension()),
        expandFileName(dir, sourceFileName(), sourceExtension()),
        expandFileName(dir, formFileName(), formExtension()),
    };
}

} // namespace Internal
} // namespace Designer
