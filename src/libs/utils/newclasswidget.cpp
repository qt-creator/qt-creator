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

#include "newclasswidget.h"
#include "ui_newclasswidget.h"


#include <QFileDialog>
#include <QDebug>
#include <QRegExp>

enum { debugNewClassWidget = 0 };

/*! \class Utils::NewClassWidget

    \brief Utility widget for 'New Class' wizards

    Utility widget for 'New Class' wizards. Prompts the user
    to enter a class name (optionally derived from some base class) and file
    names for header, source and form files. Has some smart logic to derive
    the file names from the class name. */

namespace Utils {

struct NewClassWidgetPrivate {
    NewClassWidgetPrivate();

    Ui::NewClassWidget m_ui;
    QString m_headerExtension;
    QString m_sourceExtension;
    QString m_formExtension;
    bool m_valid;
    bool m_classEdited;
    // Store the "visible" values to prevent the READ accessors from being
    // fooled by a temporarily hidden widget
    bool m_baseClassInputVisible;
    bool m_formInputVisible;
    bool m_headerInputVisible;
    bool m_sourceInputVisible;
    bool m_pathInputVisible;
    bool m_qobjectCheckBoxVisible;
    bool m_formInputCheckable;
    QRegExp m_classNameValidator;
};

NewClassWidgetPrivate:: NewClassWidgetPrivate() :
    m_headerExtension(QLatin1String("h")),
    m_sourceExtension(QLatin1String("cpp")),
    m_formExtension(QLatin1String("ui")),
    m_valid(false),
    m_classEdited(false),
    m_baseClassInputVisible(true),
    m_formInputVisible(true),
    m_headerInputVisible(true),
    m_sourceInputVisible(true),
    m_pathInputVisible(true),
    m_qobjectCheckBoxVisible(false),
    m_formInputCheckable(false)

{
}

// --------------------- NewClassWidget
NewClassWidget::NewClassWidget(QWidget *parent) :
    QWidget(parent),
    d(new NewClassWidgetPrivate)
{
    d->m_ui.setupUi(this);

    d->m_ui.baseClassComboBox->setEditable(false);

    setNamesDelimiter(QLatin1String("::"));

    connect(d->m_ui.classLineEdit, SIGNAL(updateFileName(QString)),
            this, SLOT(slotUpdateFileNames(QString)));
    connect(d->m_ui.classLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(classNameEdited()));
    connect(d->m_ui.baseClassComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(suggestClassNameFromBase()));
    connect(d->m_ui.baseClassComboBox, SIGNAL(editTextChanged(QString)),
            this, SLOT(slotValidChanged()));
    connect(d->m_ui.classLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));
    connect(d->m_ui.headerFileLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));
    connect(d->m_ui.sourceFileLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));
    connect(d->m_ui.formFileLineEdit, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));
    connect(d->m_ui.pathChooser, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));
    connect(d->m_ui.generateFormCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotValidChanged()));

    connect(d->m_ui.classLineEdit, SIGNAL(validReturnPressed()),
            this, SLOT(slotActivated()));
    connect(d->m_ui.headerFileLineEdit, SIGNAL(validReturnPressed()),
            this, SLOT(slotActivated()));
    connect(d->m_ui.sourceFileLineEdit, SIGNAL(validReturnPressed()),
            this, SLOT(slotActivated()));
    connect(d->m_ui.formFileLineEdit, SIGNAL(validReturnPressed()),
            this, SLOT(slotActivated()));
    connect(d->m_ui.formFileLineEdit, SIGNAL(validReturnPressed()),
            this, SLOT(slotActivated()));
    connect(d->m_ui.pathChooser, SIGNAL(returnPressed()),
             this, SLOT(slotActivated()));

    connect(d->m_ui.generateFormCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(slotFormInputChecked()));
    connect(d->m_ui.baseClassComboBox, SIGNAL(editTextChanged(QString)),
            this, SLOT(slotBaseClassEdited(QString)));
    d->m_ui.generateFormCheckBox->setChecked(true);
    setFormInputCheckable(false, true);
    setClassType(NoClassType);
}

NewClassWidget::~NewClassWidget()
{
    delete d;
}

void NewClassWidget::classNameEdited()
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << d->m_headerExtension << d->m_sourceExtension;
    d->m_classEdited = true;
}

void NewClassWidget::suggestClassNameFromBase()
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << d->m_headerExtension << d->m_sourceExtension;
    if (d->m_classEdited)
        return;
    // Suggest a class unless edited ("QMainWindow"->"MainWindow")
    QString base = baseClassName();
    if (base.startsWith(QLatin1Char('Q'))) {
        base.remove(0, 1);
        setClassName(base);
    }
}

QStringList NewClassWidget::baseClassChoices() const
{
    QStringList rc;
    const int count = d->m_ui.baseClassComboBox->count();
    for (int i = 0; i <  count; i++)
        rc.push_back(d->m_ui.baseClassComboBox->itemText(i));
    return rc;
}

void NewClassWidget::setBaseClassChoices(const QStringList &choices)
{
    d->m_ui.baseClassComboBox->clear();
    d->m_ui.baseClassComboBox->addItems(choices);
}

void NewClassWidget::setBaseClassInputVisible(bool visible)
{
    d->m_baseClassInputVisible = visible;
    d->m_ui.baseClassLabel->setVisible(visible);
    d->m_ui.baseClassComboBox->setVisible(visible);
}

void NewClassWidget::setBaseClassEditable(bool editable)
{
    d->m_ui.baseClassComboBox->setEditable(editable);
}

bool NewClassWidget::isBaseClassInputVisible() const
{
    return  d->m_baseClassInputVisible;
}

bool NewClassWidget::isBaseClassEditable() const
{
    return  d->m_ui.baseClassComboBox->isEditable();
}

void NewClassWidget::setFormInputVisible(bool visible)
{
    d->m_formInputVisible = visible;
    d->m_ui.formLabel->setVisible(visible);
    d->m_ui.formFileLineEdit->setVisible(visible);
}

bool NewClassWidget::isFormInputVisible() const
{
    return d->m_formInputVisible;
}

void NewClassWidget::setHeaderInputVisible(bool visible)
{
    d->m_headerInputVisible = visible;
    d->m_ui.headerLabel->setVisible(visible);
    d->m_ui.headerFileLineEdit->setVisible(visible);
}

bool NewClassWidget::isHeaderInputVisible() const
{
    return d->m_headerInputVisible;
}

void NewClassWidget::setSourceInputVisible(bool visible)
{
    d->m_sourceInputVisible = visible;
    d->m_ui.sourceLabel->setVisible(visible);
    d->m_ui.sourceFileLineEdit->setVisible(visible);
}

bool NewClassWidget::isSourceInputVisible() const
{
    return d->m_sourceInputVisible;
}

void NewClassWidget::setFormInputCheckable(bool checkable)
{
    setFormInputCheckable(checkable, false);
}

void NewClassWidget::setFormInputCheckable(bool checkable, bool force)
{
    if (!force && checkable == d->m_formInputCheckable)
        return;
    d->m_formInputCheckable = checkable;
    d->m_ui.generateFormLabel->setVisible(checkable);
    d->m_ui.generateFormCheckBox->setVisible(checkable);
}

void NewClassWidget::setFormInputChecked(bool v)
{
    d->m_ui.generateFormCheckBox->setChecked(v);
}

bool NewClassWidget::formInputCheckable() const
{
    return d->m_formInputCheckable;
}

bool NewClassWidget::formInputChecked() const
{
    return d->m_ui.generateFormCheckBox->isChecked();
}

void NewClassWidget::slotFormInputChecked()
{
    const bool checked = formInputChecked();
    d->m_ui.formLabel->setEnabled(checked);
    d->m_ui.formFileLineEdit->setEnabled(checked);
}

void NewClassWidget::setPathInputVisible(bool visible)
{
    d->m_pathInputVisible = visible;
    d->m_ui.pathLabel->setVisible(visible);
    d->m_ui.pathChooser->setVisible(visible);
}

bool NewClassWidget::isPathInputVisible() const
{
    return d->m_pathInputVisible;
}

void NewClassWidget::setClassName(const QString &suggestedName)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << suggestedName << d->m_headerExtension << d->m_sourceExtension;
    d->m_ui.classLineEdit->setText(ClassNameValidatingLineEdit::createClassName(suggestedName));
}

QString NewClassWidget::className() const
{
    return d->m_ui.classLineEdit->text();
}

QString NewClassWidget::baseClassName() const
{
    return d->m_ui.baseClassComboBox->currentText();
}

void NewClassWidget::setBaseClassName(const QString &c)
{
    const int index = d->m_ui.baseClassComboBox->findText(c);
    if (index != -1) {
        d->m_ui.baseClassComboBox->setCurrentIndex(index);
        suggestClassNameFromBase();
    }
}

QString NewClassWidget::sourceFileName() const
{
    return d->m_ui.sourceFileLineEdit->text();
}

QString NewClassWidget::headerFileName() const
{
    return d->m_ui.headerFileLineEdit->text();
}

QString NewClassWidget::formFileName() const
{
    return d->m_ui.formFileLineEdit->text();
}

QString NewClassWidget::path() const
{
    return d->m_ui.pathChooser->path();
}

void NewClassWidget::setPath(const QString &path)
{
     d->m_ui.pathChooser->setPath(path);
}

bool NewClassWidget::namespacesEnabled() const
{
    return  d->m_ui.classLineEdit->namespacesEnabled();
}

void NewClassWidget::setNamespacesEnabled(bool b)
{
    d->m_ui.classLineEdit->setNamespacesEnabled(b);
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

void NewClassWidget::setFormExtension(const QString &e)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << e;
    d->m_formExtension = fixSuffix(e);
}

bool NewClassWidget::allowDirectories() const
{
    return d->m_ui.headerFileLineEdit->allowDirectories();
}

void NewClassWidget::setAllowDirectories(bool v)
{
    // We keep all in sync
    if (allowDirectories() != v) {
        d->m_ui.sourceFileLineEdit->setAllowDirectories(v);
        d->m_ui.headerFileLineEdit->setAllowDirectories(v);
        d->m_ui.formFileLineEdit->setAllowDirectories(v);
    }
}

bool NewClassWidget::lowerCaseFiles() const
{
    return d->m_ui.classLineEdit->lowerCaseFileName();
}

void NewClassWidget::setLowerCaseFiles(bool v)
{
    d->m_ui.classLineEdit->setLowerCaseFileName(v);
}

NewClassWidget::ClassType NewClassWidget::classType() const
{
    return static_cast<ClassType>(d->m_ui.classTypeComboBox->currentIndex());
}

QString NewClassWidget::namesDelimiter() const
{
    return d->m_ui.classLineEdit->namespaceDelimiter();
}

void NewClassWidget::setClassType(ClassType ct)
{
    d->m_ui.classTypeComboBox->setCurrentIndex(ct);
}

void NewClassWidget::setNamesDelimiter(const QString &delimiter)
{
    d->m_ui.classLineEdit->setNamespaceDelimiter(delimiter);
    const QString escaped = QRegExp::escape(delimiter);
    d->m_classNameValidator = QRegExp(QLatin1String("[a-zA-Z_][a-zA-Z0-9_]*(")
                                      + escaped
                                      + QLatin1String("[a-zA-Z_][a-zA-Z0-9_]*)*"));
}

bool NewClassWidget::isClassTypeComboVisible() const
{
    return d->m_ui.classTypeLabel->isVisible();
}

void NewClassWidget::setClassTypeComboVisible(bool v)
{
    d->m_ui.classTypeLabel->setVisible(v);
    d->m_ui.classTypeComboBox->setVisible(v);
}

// Guess suitable type information with some smartness
static inline NewClassWidget::ClassType classTypeForBaseClass(const QString &baseClass)
{
    if (!baseClass.startsWith(QLatin1Char('Q')))
        return NewClassWidget::NoClassType;
    // QObject types: QObject as such and models.
    if (baseClass == QLatin1String("QObject") ||
        (baseClass.startsWith(QLatin1String("QAbstract")) && baseClass.endsWith(QLatin1String("Model"))))
        return NewClassWidget::ClassInheritsQObject;
    // Widgets.
    if (baseClass == QLatin1String("QWidget") || baseClass == QLatin1String("QMainWindow")
        || baseClass == QLatin1String("QDialog"))
        return NewClassWidget::ClassInheritsQWidget;
    // Declarative Items
    if (baseClass == QLatin1String("QDeclarativeItem"))
        return NewClassWidget::ClassInheritsQDeclarativeItem;
    if (baseClass == QLatin1String("QQuickItem"))
        return NewClassWidget::ClassInheritsQQuickItem;
    return NewClassWidget::NoClassType;
}

void NewClassWidget::slotBaseClassEdited(const QString &baseClass)
{
    // Set type information with some smartness.
    const ClassType currentClassType = classType();
    const ClassType recommendedClassType = classTypeForBaseClass(baseClass);
    if (recommendedClassType != NoClassType && currentClassType != recommendedClassType)
        setClassType(recommendedClassType);
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
    if (!d->m_ui.classLineEdit->isValid()) {
        if (error)
            *error = d->m_ui.classLineEdit->errorMessage();
        return false;
    }

    if (isBaseClassInputVisible() && isBaseClassEditable()) {
        const QString baseClass = d->m_ui.baseClassComboBox->currentText().trimmed();
        if (!baseClass.isEmpty() && !d->m_classNameValidator.exactMatch(baseClass)) {
            if (error)
                *error = tr("Invalid base class name");
            return false;
        }
    }

    if (isHeaderInputVisible() && !d->m_ui.headerFileLineEdit->isValid()) {
        if (error)
            *error = tr("Invalid header file name: '%1'").arg(d->m_ui.headerFileLineEdit->errorMessage());
        return false;
    }

    if (isSourceInputVisible() && !d->m_ui.sourceFileLineEdit->isValid()) {
        if (error)
            *error = tr("Invalid source file name: '%1'").arg(d->m_ui.sourceFileLineEdit->errorMessage());
        return false;
    }

    if (isFormInputVisible() &&
        (!d->m_formInputCheckable || d->m_ui.generateFormCheckBox->isChecked())) {
        if (!d->m_ui.formFileLineEdit->isValid()) {
            if (error)
                *error = tr("Invalid form file name: '%1'").arg(d->m_ui.formFileLineEdit->errorMessage());
            return false;
        }
    }

    if (isPathInputVisible()) {
        if (!d->m_ui.pathChooser->isValid()) {
            if (error)
                *error =  d->m_ui.pathChooser->errorMessage();
            return false;
        }
    }
    return true;
}

void NewClassWidget::triggerUpdateFileNames()
{
    d->m_ui.classLineEdit->triggerChanged();
}

void NewClassWidget::slotUpdateFileNames(const QString &baseName)
{
    if (debugNewClassWidget)
        qDebug() << Q_FUNC_INFO << baseName << d->m_headerExtension << d->m_sourceExtension;
    const QChar dot = QLatin1Char('.');
    d->m_ui.sourceFileLineEdit->setText(baseName + dot + d->m_sourceExtension);
    d->m_ui.headerFileLineEdit->setText(baseName + dot + d->m_headerExtension);
    d->m_ui.formFileLineEdit->setText(baseName + dot + d->m_formExtension);
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
static QString expandFileName(const QDir &dir, const QString name, const QString &extension)
{
    if (name.isEmpty())
        return QString();
    return dir.absoluteFilePath(ensureSuffix(name, extension));
}

QStringList NewClassWidget::files() const
{
    QStringList rc;
    const QDir dir = QDir(path());
    if (isHeaderInputVisible())
        rc.push_back(expandFileName(dir, headerFileName(), headerExtension()));
    if (isSourceInputVisible())
        rc.push_back(expandFileName(dir, sourceFileName(), sourceExtension()));
    if (isFormInputVisible())
        rc.push_back(expandFileName(dir, formFileName(), formExtension()));
    return rc;
}

} // namespace Utils
