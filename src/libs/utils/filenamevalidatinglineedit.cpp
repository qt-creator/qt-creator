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

#include "filenamevalidatinglineedit.h"
#include "qtcassert.h"

#include <QRegExp>
#include <QDebug>

/*!
  \class Utils::FileNameValidatingLineEdit

  \brief A control that let's the user choose a (base) file name, based on a QLineEdit. Has
   some validation logic for embedding into QWizardPage.
*/

namespace Utils {

#define WINDOWS_DEVICES "CON|AUX|PRN|COM1|COM2|LPT1|LPT2|NUL"

// Naming a file like a device name will break on Windows, even if it is
// "com1.txt". Since we are cross-platform, we generally disallow such file
//  names.
static QRegExp &windowsDeviceNoSubDirPattern()
{
    static QRegExp rc(QLatin1String(WINDOWS_DEVICES), Qt::CaseInsensitive);
    QTC_ASSERT(rc.isValid(), return rc);
    return rc;
}

static QRegExp &windowsDeviceSubDirPattern()
{
    static QRegExp rc(QLatin1String(".*[/\\\\](" WINDOWS_DEVICES ")"), Qt::CaseInsensitive);
    QTC_ASSERT(rc.isValid(), return rc);
    return rc;
}

// ----------- FileNameValidatingLineEdit
FileNameValidatingLineEdit::FileNameValidatingLineEdit(QWidget *parent) :
    BaseValidatingLineEdit(parent),
    m_allowDirectories(false),
    m_forceFirstCapitalLetter(false)
{
}

bool FileNameValidatingLineEdit::allowDirectories() const
{
    return m_allowDirectories;
}

void FileNameValidatingLineEdit::setAllowDirectories(bool v)
{
    m_allowDirectories = v;
}

bool FileNameValidatingLineEdit::forceFirstCapitalLetter() const
{
    return m_forceFirstCapitalLetter;
}

void FileNameValidatingLineEdit::setForceFirstCapitalLetter(bool b)
{
    m_forceFirstCapitalLetter = b;
}

/* Validate a file base name, check for forbidden characters/strings. */


#define SLASHES "/\\"

static const char notAllowedCharsSubDir[]   = ",^@=+{}[]~!?:&*\"|#%<>$\"'();`' ";
static const char notAllowedCharsNoSubDir[] = ",^@=+{}[]~!?:&*\"|#%<>$\"'();`' " SLASHES;

static const char *notAllowedSubStrings[] = {".."};

bool FileNameValidatingLineEdit::validateFileName(const QString &name,
                                                  bool allowDirectories,
                                                  QString *errorMessage /* = 0*/)
{
    if (name.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("Name is empty.");
        return false;
    }
    // Characters
    const char *notAllowedChars = allowDirectories ? notAllowedCharsSubDir : notAllowedCharsNoSubDir;
    for (const char *c = notAllowedChars; *c; c++)
        if (name.contains(QLatin1Char(*c))) {
            if (errorMessage) {
                const QChar qc = QLatin1Char(*c);
                if (qc.isSpace())
                    *errorMessage = tr("Name contains white space.");
                else
                    *errorMessage = tr("Invalid character '%1'.").arg(qc);
            }
            return false;
        }
    // Substrings
    const int notAllowedSubStringCount = sizeof(notAllowedSubStrings)/sizeof(const char *);
    for (int s = 0; s < notAllowedSubStringCount; s++) {
        const QLatin1String notAllowedSubString(notAllowedSubStrings[s]);
        if (name.contains(notAllowedSubString)) {
            if (errorMessage)
                *errorMessage = tr("Invalid characters '%1'.").arg(QString(notAllowedSubString));
            return false;
        }
    }
    // Windows devices
    bool matchesWinDevice = windowsDeviceNoSubDirPattern().exactMatch(name);
    if (!matchesWinDevice && allowDirectories)
        matchesWinDevice = windowsDeviceSubDirPattern().exactMatch(name);
    if (matchesWinDevice) {
        if (errorMessage)
            *errorMessage = tr("Name matches MS Windows device. (%1).").
                            arg(windowsDeviceNoSubDirPattern().pattern().replace(QLatin1Char('|'), QLatin1Char(',')));
        return false;
    }
    return true;
}

bool  FileNameValidatingLineEdit::validate(const QString &value, QString *errorMessage) const
{
    return validateFileNameExtension(value, requiredExtensions(), errorMessage)
            && validateFileName(value, allowDirectories(), errorMessage);
}

QString FileNameValidatingLineEdit::fixInputString(const QString &string)
{
    if (!forceFirstCapitalLetter())
        return string;

    QString fixedString = string;
    if (!string.isEmpty() && string.at(0).isLower())
        fixedString[0] = string.at(0).toUpper();

    return fixedString;
}

bool FileNameValidatingLineEdit::validateFileNameExtension(const QString &fileName,
                                                           const QStringList &requiredExtensions,
                                                           QString *errorMessage)
{
    // file extension
    if (!requiredExtensions.isEmpty()) {
        foreach (const QString &requiredExtension, requiredExtensions) {
            QString extension = QLatin1String(".") + requiredExtension;
            if (fileName.endsWith(extension, Qt::CaseSensitive) && extension.count() < fileName.count())
                return true;
        }

        if (errorMessage) {
            if (requiredExtensions.count() == 1)
                *errorMessage = tr("File extension %1 is required:").arg(requiredExtensions.first());
            else
                *errorMessage = tr("File extensions %1 are required:").arg(requiredExtensions.join(QLatin1String(", ")));
        }

        return false;
    }

    return true;
}

QStringList FileNameValidatingLineEdit::requiredExtensions() const
{
    return m_requiredExtensionList;
}

void FileNameValidatingLineEdit::setRequiredExtensions(const QStringList &extensions)
{
    m_requiredExtensionList = extensions;
}

} // namespace Utils
