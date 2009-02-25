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

#include "filenamevalidatinglineedit.h"
#include "qtcassert.h"

#include <QtCore/QRegExp>
#include <QtCore/QDebug>

namespace Core {
namespace Utils {

// Naming a file like a device name will break on Windows, even if it is
// "com1.txt". Since we are cross-platform, we generally disallow such file
//  names.
static const QRegExp &windowsDeviceNoSubDirPattern()
{
    static const QRegExp rc(QLatin1String("CON|AUX|PRN|COM1|COM2|LPT1|LPT2|NUL"),
                      Qt::CaseInsensitive);
    QTC_ASSERT(rc.isValid(), return rc);
    return rc;
}

static const QRegExp &windowsDeviceSubDirPattern()
{
    static const QRegExp rc(QLatin1String(".*[/\\\\]CON|.*[/\\\\]AUX|.*[/\\\\]PRN|.*[/\\\\]COM1|.*[/\\\\]COM2|.*[/\\\\]LPT1|.*[/\\\\]LPT2|.*[/\\\\]NUL"),
                            Qt::CaseInsensitive);
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
            *errorMessage = tr("The name must not be empty");
        return false;
    }
    // Characters
    const char *notAllowedChars = allowDirectories ? notAllowedCharsSubDir : notAllowedCharsNoSubDir;
    for (const char *c = notAllowedChars; *c; c++)
        if (name.contains(QLatin1Char(*c))) {
            if (errorMessage)
                *errorMessage = tr("The name must not contain any of the characters '%1'.").arg(QLatin1String(notAllowedChars));
            return false;
        }
    // Substrings
    const int notAllowedSubStringCount = sizeof(notAllowedSubStrings)/sizeof(const char *);
    for (int s = 0; s < notAllowedSubStringCount; s++) {
        const QLatin1String notAllowedSubString(notAllowedSubStrings[s]);
        if (name.contains(notAllowedSubString)) {
            if (errorMessage)
                *errorMessage = tr("The name must not contain '%1'.").arg(QString(notAllowedSubString));
            return false;
        }
    }
    // Windows devices
    bool matchesWinDevice = windowsDeviceNoSubDirPattern().exactMatch(name);
    if (!matchesWinDevice && allowDirectories)
        matchesWinDevice = windowsDeviceSubDirPattern().exactMatch(name);
    if (matchesWinDevice) {
        if (errorMessage)
            *errorMessage = tr("The name must not match that of a MS Windows device. (%1).").
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
} // namespace Core
