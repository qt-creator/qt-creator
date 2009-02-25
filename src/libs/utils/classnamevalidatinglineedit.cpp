/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "classnamevalidatinglineedit.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QRegExp>

namespace Core {
namespace Utils {

struct ClassNameValidatingLineEditPrivate {
    ClassNameValidatingLineEditPrivate();

    const QRegExp m_nameRegexp;
    const QString m_namespaceDelimiter;
    bool m_namespacesEnabled;
};

// Match something like "Namespace1::Namespace2::ClassName".
ClassNameValidatingLineEditPrivate:: ClassNameValidatingLineEditPrivate() :
    m_nameRegexp(QLatin1String("[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)*")),
    m_namespaceDelimiter(QLatin1String("::")),
    m_namespacesEnabled(false)
{
    QTC_ASSERT(m_nameRegexp.isValid(), return);
}

// --------------------- ClassNameValidatingLineEdit
ClassNameValidatingLineEdit::ClassNameValidatingLineEdit(QWidget *parent) :
    Core::Utils::BaseValidatingLineEdit(parent),
    m_d(new ClassNameValidatingLineEditPrivate)
{
}

ClassNameValidatingLineEdit::~ClassNameValidatingLineEdit()
{
    delete m_d;
}

bool ClassNameValidatingLineEdit::namespacesEnabled() const
{
    return m_d->m_namespacesEnabled;
}

void ClassNameValidatingLineEdit::setNamespacesEnabled(bool b)
{
    m_d->m_namespacesEnabled = b;
}

bool ClassNameValidatingLineEdit::validate(const QString &value, QString *errorMessage) const
{
    if (!m_d->m_namespacesEnabled && value.contains(QLatin1Char(':'))) {
        if (errorMessage)
            *errorMessage = tr("The class name must not contain namespace delimiters.");
        return false;
    }
    if (!m_d->m_nameRegexp.exactMatch(value)) {
        if (errorMessage)
            *errorMessage = tr("The class name contains invalid characters.");
        return false;
    }
    return true;
}

void ClassNameValidatingLineEdit::slotChanged(const QString &t)
{
    Core::Utils::BaseValidatingLineEdit::slotChanged(t);
    if (isValid()) {
        // Suggest file names, strip namespaces
        QString fileName = t.toLower();
        if (m_d->m_namespacesEnabled) {
            const int namespaceIndex = fileName.lastIndexOf(m_d->m_namespaceDelimiter);
            if (namespaceIndex != -1)
                fileName.remove(0, namespaceIndex + m_d->m_namespaceDelimiter.size());
        }
        emit updateFileName(fileName);
    }
}

QString ClassNameValidatingLineEdit::createClassName(const QString &name)
{
    // Remove spaces and convert the adjacent characters to uppercase
    QString className = name;
    QRegExp spaceMatcher(QLatin1String(" +(\\w)"), Qt::CaseSensitive, QRegExp::RegExp2);
    QTC_ASSERT(spaceMatcher.isValid(), /**/);
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

} // namespace Utils
} // namespace Core
