/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#include "filenamevalidatinglineedit.h"

namespace Core {
namespace Utils {

FileNameValidatingLineEdit::FileNameValidatingLineEdit(QWidget *parent) :
    BaseValidatingLineEdit(parent)
{

}

/* Validate a file base name, check for forbidden characters/strings. */

static const char *notAllowedChars = "/?:&\\*\"|#%<> ";
static const char *notAllowedSubStrings[] = {".."};

// Naming a file like a device name will break on Windows, even if it is
// "com1.txt". Since we are cross-platform, we generally disallow such file
//  names.
static const char *notAllowedStrings[] = {"CON", "AUX", "PRN", "COM1", "COM2", "LPT1", "LPT2" };

bool FileNameValidatingLineEdit::validateFileName(const QString &name, QString *errorMessage /* = 0*/)
{
    if (name.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The name must not be empty");
        return false;
    }
    // Characters
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
    // Strings
    const int notAllowedStringCount = sizeof(notAllowedStrings)/sizeof(const char *);
    for (int s = 0; s < notAllowedStringCount; s++) {
        const QLatin1String notAllowedString(notAllowedStrings[s]);
        if (name == notAllowedString) {
            if (errorMessage)
                *errorMessage = tr("The name must not be '%1'.").arg(QString(notAllowedString));
            return false;
        }
    }
    return true;
}

bool  FileNameValidatingLineEdit::validate(const QString &value, QString *errorMessage) const
{
    return validateFileName(value, errorMessage);
}

} // namespace Utils
} // namespace Core
