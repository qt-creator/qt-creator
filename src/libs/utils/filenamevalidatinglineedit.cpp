// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filenamevalidatinglineedit.h"

#include "qtcassert.h"
#include "utilstr.h"

#include <QRegularExpression>

/*!
  \class Utils::FileNameValidatingLineEdit
  \inmodule QtCreator

  \brief The FileNameValidatingLineEdit class is a control that lets the user
  choose a (base) file name, based on a QLineEdit.

  The class has
   some validation logic for embedding into QWizardPage.
*/

namespace Utils {

#define WINDOWS_DEVICES_PATTERN "(CON|AUX|PRN|NUL|COM[1-9]|LPT[1-9])(\\..*)?"

// Naming a file like a device name will break on Windows, even if it is
// "com1.txt". Since we are cross-platform, we generally disallow such file
//  names.
static const QRegularExpression &windowsDeviceNoSubDirPattern()
{
    static const QRegularExpression rc(QString("^" WINDOWS_DEVICES_PATTERN "$"),
                                       QRegularExpression::CaseInsensitiveOption);
    QTC_ASSERT(rc.isValid(), return rc);
    return rc;
}

static const QRegularExpression &windowsDeviceSubDirPattern()
{
    static const QRegularExpression rc(QString("^.*[/\\\\]" WINDOWS_DEVICES_PATTERN "$"),
                                       QRegularExpression::CaseInsensitiveOption);
    QTC_ASSERT(rc.isValid(), return rc);
    return rc;
}

// ----------- FileNameValidatingLineEdit
FileNameValidatingLineEdit::FileNameValidatingLineEdit(QWidget *parent) :
    FancyLineEdit(parent),
    m_allowDirectories(false),
    m_forceFirstCapitalLetter(false)
{
    setValidationFunction([this](FancyLineEdit *edit) {
        if (const Result<> res = validateFileNameExtension(edit->text(), requiredExtensions()); !res)
            return res;
        if (const Result<> res = validateFileName(edit->text(), allowDirectories()); !res)
            return res;
        return ResultOk;
    });
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

Result<> FileNameValidatingLineEdit::validateFileName(const QString &name, bool allowDirectories)
{
    static const char notAllowedCharsSubDir[]   = ",^@={}[]~!?:&*\"|#%<>$\"'();`' ";
    static const char notAllowedCharsNoSubDir[] = ",^@={}[]~!?:&*\"|#%<>$\"'();`' " SLASHES;

    static const char *notAllowedSubStrings[] = {".."};

    if (name.isEmpty())
        return ResultError(Tr::tr("Name is empty."));

    // Characters
    const char *notAllowedChars = allowDirectories ? notAllowedCharsSubDir : notAllowedCharsNoSubDir;
    for (const char *c = notAllowedChars; *c; c++) {
        if (name.contains(QLatin1Char(*c))) {
            const QChar qc = QLatin1Char(*c);
            if (qc.isSpace())
                return ResultError(Tr::tr("Name contains white space."));
            return ResultError(Tr::tr("Invalid character \"%1\".").arg(qc));
        }
    }

    // Substrings
    const int notAllowedSubStringCount = sizeof(notAllowedSubStrings)/sizeof(const char *);
    for (int s = 0; s < notAllowedSubStringCount; s++) {
        const QLatin1String notAllowedSubString(notAllowedSubStrings[s]);
        if (name.contains(notAllowedSubString))
            return ResultError(Tr::tr("Invalid characters \"%1\".").arg(QString(notAllowedSubString)));
    }

    // Windows devices
    bool matchesWinDevice = name.contains(windowsDeviceNoSubDirPattern());
    if (!matchesWinDevice && allowDirectories)
        matchesWinDevice = name.contains(windowsDeviceSubDirPattern());
    if (matchesWinDevice) {
        return ResultError(Tr::tr("Name matches MS Windows device"
                                  " (CON, AUX, PRN, NUL,"
                                  " COM1, COM2, ..., COM9,"
                                  " LPT1, LPT2, ..., LPT9)"));
    }
    return ResultOk;
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

Result<> FileNameValidatingLineEdit::validateFileNameExtension(const QString &fileName,
                                                               const QStringList &requiredExtensions)
{
    // file extension
    if (requiredExtensions.isEmpty())
        return ResultOk;

    for (const QString &requiredExtension : requiredExtensions) {
        QString extension = QLatin1Char('.') + requiredExtension;
        if (fileName.endsWith(extension, Qt::CaseSensitive) && extension.size() < fileName.size())
            return ResultOk;
    }

    if (requiredExtensions.size() == 1)
        return ResultError(Tr::tr("File extension %1 is required:").arg(requiredExtensions.first()));

    return ResultError(Tr::tr("File extensions %1 are required:").arg(requiredExtensions.join(", ")));
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
