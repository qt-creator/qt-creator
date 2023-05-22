// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classnamevalidatinglineedit.h"

#include "qtcassert.h"
#include "utilstr.h"

#include <QRegularExpression>

/*!
    \class Utils::ClassNameValidatingLineEdit
    \inmodule QtCreator

    \brief The ClassNameValidatingLineEdit class implements a line edit that
    validates a C++ class name and emits a signal
    to derive suggested file names from it.
*/

namespace Utils {

// Match something like "Namespace1::Namespace2::ClassName".

struct ClassNameValidatingLineEditPrivate
{
    QRegularExpression m_nameRegexp;
    QString m_namespaceDelimiter{"::"};
    bool m_namespacesEnabled = false;
    bool m_lowerCaseFileName = true;
    bool m_forceFirstCapitalLetter = false;
};

// --------------------- ClassNameValidatingLineEdit
ClassNameValidatingLineEdit::ClassNameValidatingLineEdit(QWidget *parent) :
    FancyLineEdit(parent),
    d(new ClassNameValidatingLineEditPrivate)
{
    setValidationFunction([this](FancyLineEdit *edit, QString *errorMessage) {
        return validateClassName(edit, errorMessage);
    });
    updateRegExp();
}

ClassNameValidatingLineEdit::~ClassNameValidatingLineEdit()
{
    delete d;
}

bool ClassNameValidatingLineEdit::namespacesEnabled() const
{
    return d->m_namespacesEnabled;
}

void ClassNameValidatingLineEdit::setNamespacesEnabled(bool b)
{
    d->m_namespacesEnabled = b;
}

/**
 * @return Language-specific namespace delimiter, e.g. '::' or '.'
 */
QString ClassNameValidatingLineEdit::namespaceDelimiter()
{
    return d->m_namespaceDelimiter;
}

/**
 * @brief Sets language-specific namespace delimiter, e.g. '::' or '.'
 * Do not use identifier characters in delimiter
 */
void ClassNameValidatingLineEdit::setNamespaceDelimiter(const QString &delimiter)
{
    d->m_namespaceDelimiter = delimiter;
}

bool ClassNameValidatingLineEdit::validateClassName(FancyLineEdit *edit, QString *errorMessage) const
{
    QTC_ASSERT(d->m_nameRegexp.isValid(), return false);

    const QString value = edit->text();
    if (!d->m_namespacesEnabled && value.contains(d->m_namespaceDelimiter)) {
        if (errorMessage)
            *errorMessage = Tr::tr("The class name must not contain namespace delimiters.");
        return false;
    } else if (value.isEmpty()) {
        if (errorMessage)
            *errorMessage = Tr::tr("Please enter a class name.");
        return false;
    } else if (!d->m_nameRegexp.match(value).hasMatch()) {
        if (errorMessage)
            *errorMessage = Tr::tr("The class name contains invalid characters.");
        return false;
    }
    return true;
}

void ClassNameValidatingLineEdit::handleChanged(const QString &t)
{
    if (isValid()) {
        // Suggest file names, strip namespaces
        QString fileName = d->m_lowerCaseFileName ? t.toLower() : t;
        if (d->m_namespacesEnabled) {
            const int namespaceIndex = fileName.lastIndexOf(d->m_namespaceDelimiter);
            if (namespaceIndex != -1)
                fileName.remove(0, namespaceIndex + d->m_namespaceDelimiter.size());
        }
        emit updateFileName(fileName);
    }
}

QString ClassNameValidatingLineEdit::fixInputString(const QString &string)
{
    if (!forceFirstCapitalLetter())
        return string;

    QString fixedString = string;
    if (!string.isEmpty() && string.at(0).isLower())
        fixedString[0] = string.at(0).toUpper();

    return fixedString;
}

void ClassNameValidatingLineEdit::updateRegExp() const
{
    const QString pattern = "^%1(%2%1)*$";
    d->m_nameRegexp.setPattern(pattern.arg(QString("[a-zA-Z_][a-zA-Z0-9_]*"),
                                           QRegularExpression::escape(d->m_namespaceDelimiter)));
}

QString ClassNameValidatingLineEdit::createClassName(const QString &name)
{
    // Remove spaces and convert the adjacent characters to uppercase
    QString className = name;
    const QRegularExpression spaceMatcher(" +(\\w)");
    QTC_CHECK(spaceMatcher.isValid());
    while (true) {
        const QRegularExpressionMatch match = spaceMatcher.match(className);
        if (!match.hasMatch())
            break;
        className.replace(match.capturedStart(), match.capturedLength(), match.captured(1).toUpper());
    }

    // Filter out any remaining invalid characters
    className.remove(QRegularExpression("[^a-zA-Z0-9_]"));

    // If the first character is numeric, prefix the name with a "_"
    if (className.at(0).isNumber()) {
        className.prepend(QLatin1Char('_'));
    } else {
        // Convert the first character to uppercase
        className.replace(0, 1, className.left(1).toUpper());
    }

    return className;
}

bool ClassNameValidatingLineEdit::lowerCaseFileName() const
{
    return d->m_lowerCaseFileName;
}

void ClassNameValidatingLineEdit::setLowerCaseFileName(bool v)
{
    d->m_lowerCaseFileName = v;
}

bool ClassNameValidatingLineEdit::forceFirstCapitalLetter() const
{
    return d->m_forceFirstCapitalLetter;
}

void ClassNameValidatingLineEdit::setForceFirstCapitalLetter(bool b)
{
    d->m_forceFirstCapitalLetter = b;
}

} // namespace Utils
