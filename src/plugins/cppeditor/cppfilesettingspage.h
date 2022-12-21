// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditorconstants.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QDir>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CppEditor::Internal {

class CppFileSettings
{
public:
    QStringList headerPrefixes;
    QString headerSuffix = "h";
    QStringList headerSearchPaths = {"include",
                                     "Include",
                                     QDir::toNativeSeparators("../include"),
                                     QDir::toNativeSeparators("../Include")};
    QStringList sourcePrefixes;
    QString sourceSuffix = "cpp";
    QStringList sourceSearchPaths = {QDir::toNativeSeparators("../src"),
                                     QDir::toNativeSeparators("../Src"),
                                     ".."};
    QString licenseTemplatePath;
    bool headerPragmaOnce = false;
    bool lowerCaseFiles = Constants::LOWERCASE_CPPFILES_DEFAULT;

    void toSettings(QSettings *) const;
    void fromSettings(QSettings *);
    bool applySuffixesToMimeDB();

    // Convenience to return a license template completely formatted.
    // Currently made public in
    static QString licenseTemplate();

    bool equals(const CppFileSettings &rhs) const;
    bool operator==(const CppFileSettings &s) const { return equals(s); }
    bool operator!=(const CppFileSettings &s) const { return !equals(s); }
};

class CppFileSettingsPage : public Core::IOptionsPage
{
public:
    explicit CppFileSettingsPage(CppFileSettings *settings);
};

} // namespace CppEditor::Internal
