/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "classnamevalidatinglineedit.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QRegExp>

/*!
    \class Utils::ClassNameValidatingLineEdit

    \brief The ClassNameValidatingLineEdit class implements a line edit that
    validates a C++ class name and emits a signal
    to derive suggested file names from it.
*/

namespace Utils {

struct ClassNameValidatingLineEditPrivate {
    ClassNameValidatingLineEditPrivate();

    QRegExp m_nameRegexp;
    QString m_namespaceDelimiter;
    bool m_namespacesEnabled;
    bool m_lowerCaseFileName;
    bool m_forceFirstCapitalLetter;
};

// Match something like "Namespace1::Namespace2::ClassName".
ClassNameValidatingLineEditPrivate:: ClassNameValidatingLineEditPrivate() :
    m_namespaceDelimiter(QLatin1String("::")),
    m_namespacesEnabled(false),
    m_lowerCaseFileName(true),
    m_forceFirstCapitalLetter(false)
{
}

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
            *errorMessage = tr("The class name must not contain namespace delimiters.");
        return false;
    } else if (value.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("Please enter a class name.");
        return false;
    } else if (!d->m_nameRegexp.exactMatch(value)) {
        if (errorMessage)
            *errorMessage = tr("The class name contains invalid characters.");
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
    const QString pattern(QLatin1String("%1(%2%1)*"));
    d->m_nameRegexp.setPattern(pattern.arg(QLatin1String("[a-zA-Z_][a-zA-Z0-9_]*"))
                               .arg(QRegExp::escape(d->m_namespaceDelimiter)));
}

QString ClassNameValidatingLineEdit::createClassName(const QString &name)
{
    // Remove spaces and convert the adjacent characters to uppercase
    QString className = name;
    QRegExp spaceMatcher(QLatin1String(" +(\\w)"), Qt::CaseSensitive, QRegExp::RegExp2);
    QTC_CHECK(spaceMatcher.isValid());
    int pos;
    while ((pos = spaceMatcher.indexIn(className)) != -1) {
        className.replace(pos, spaceMatcher.matchedLength(),
                          spaceMatcher.cap(1).toUpper());
    }

    // Filter out any remaining invalid characters
    className.remove(QRegExp(QLatin1String("[^a-zA-Z0-9_]")));

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
