/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "filenamevalidatinglineedit.h"
#include "qtcassert.h"

#include <QtCore/QRegExp>
#include <QtCore/QDebug>

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
static const QRegExp &windowsDeviceNoSubDirPattern()
{
    static const QRegExp rc(QLatin1String(WINDOWS_DEVICES),
                      Qt::CaseInsensitive);
    QTC_ASSERT(rc.isValid(), return rc);
    return rc;
}

static const QRegExp &windowsDeviceSubDirPattern()
{
    static const QRegExp rc(QLatin1String(".*[/\\\\](" WINDOWS_DEVICES ")"), Qt::CaseInsensitive);
    QTC_ASSERT(rc.isValid(), return rc);
    return rc;
}

// ----------- FileNameValidatingLineEdit
FileNameValidatingLineEdit::FileNameValidatingLineEdit(QWidget *parent) :
    BaseValidatingLineEdit(parent),
    m_allowDirectories(false),
    m_unused(0)
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

/* Validate a file base name, check for forbidden characters/strings. */

#ifdef Q_OS_WIN
#  define SLASHES "/\\"
#else
#  define SLASHES "/"
#endif

static const char *notAllowedCharsSubDir   = "?:&*\"|#%<> ";
static const char *notAllowedCharsNoSubDir = "?:&*\"|#%<> "SLASHES;

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
                if (QChar(*c).isSpace()) {
                    *errorMessage = tr("Name contains white space.");
                } else {
                    *errorMessage = tr("Invalid character '%1'.").arg(*c);
                }
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
    return validateFileName(value, m_allowDirectories, errorMessage);
}

} // namespace Utils
