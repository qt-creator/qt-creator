/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangformatsettings.h"

#include "clangformatconstants.h"

#include "../beautifierconstants.h"

#include <QDateTime>
#include <QXmlStreamWriter>

#include <coreplugin/icore.h>

namespace Beautifier {
namespace Internal {
namespace ClangFormat {

namespace {
const char USE_PREDEFINED_STYLE[]        = "usePredefinedStyle";
const char PREDEFINED_STYLE[]            = "predefinedStyle";
const char CUSTOM_STYLE[]                = "customStyle";
const char FORMAT_ENTIRE_FILE_FALLBACK[] = "formatEntireFileFallback";
}

ClangFormatSettings::ClangFormatSettings() :
    AbstractSettings(Constants::ClangFormat::SETTINGS_NAME, ".clang-format")
{
    setCommand("clang-format");
    m_settings.insert(USE_PREDEFINED_STYLE, QVariant(true));
    m_settings.insert(PREDEFINED_STYLE, "LLVM");
    m_settings.insert(CUSTOM_STYLE, QVariant());
    m_settings.insert(FORMAT_ENTIRE_FILE_FALLBACK, QVariant(true));
    read();
}

QString ClangFormatSettings::documentationFilePath() const
{
    return Core::ICore::userResourcePath() + '/' + Beautifier::Constants::SETTINGS_DIRNAME + '/'
            + Beautifier::Constants::DOCUMENTATION_DIRNAME + '/'
            + Constants::ClangFormat::SETTINGS_NAME + ".xml";
}

void ClangFormatSettings::createDocumentationFile() const
{
    QFile file(documentationFilePath());
    const QFileInfo fi(file);
    if (!fi.exists())
        fi.dir().mkpath(fi.absolutePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument("1.0", true);
    stream.writeComment("Created " + QDateTime::currentDateTime().toString(Qt::ISODate));
    stream.writeStartElement(Constants::DOCUMENTATION_XMLROOT);

    const QStringList lines = {
        "BasedOnStyle {string: LLVM, Google, Chromium, Mozilla, WebKit}",
        "AccessModifierOffset {int}",
        "AlignEscapedNewlinesLeft {bool}",
        "AlignTrailingComments {bool}",
        "AllowAllParametersOfDeclarationOnNextLine {bool}",
        "AllowShortFunctionsOnASingleLine {bool}",
        "AllowShortIfStatementsOnASingleLine {bool}",
        "AllowShortLoopsOnASingleLine {bool}",
        "AlwaysBreakBeforeMultilineStrings {bool}",
        "AlwaysBreakTemplateDeclarations {bool}",
        "BinPackParameters {bool}",
        "BreakBeforeBinaryOperators {bool}",
        "BreakBeforeBraces {BraceBreakingStyle: BS_Attach, BS_Linux, BS_Stroustrup, BS_Allman, BS_GNU}",
        "BreakBeforeTernaryOperators {bool}",
        "BreakConstructorInitializersBeforeComma {bool}",
        "ColumnLimit {unsigned}",
        "CommentPragmas {string}",
        "ConstructorInitializerAllOnOneLineOrOnePerLine {bool}",
        "ConstructorInitializerIndentWidth {unsigned}",
        "ContinuationIndentWidth {unsigned}",
        "Cpp11BracedListStyle {bool}",
        "IndentCaseLabels {bool}",
        "IndentFunctionDeclarationAfterType {bool}",
        "IndentWidth {unsigned}",
        "Language {LanguageKind: LK_None, LK_Cpp, LK_JavaScript, LK_Proto}",
        "MaxEmptyLinesToKeep {unsigned}",
        "NamespaceIndentation {NamespaceIndentationKind: NI_None, NI_Inner, NI_All}",
        "ObjCSpaceAfterProperty {bool}",
        "ObjCSpaceBeforeProtocolList {bool}",
        "PenaltyBreakBeforeFirstCallParameter {unsigned}",
        "PenaltyBreakComment {unsigned}",
        "PenaltyBreakFirstLessLess {unsigned}",
        "PenaltyBreakString {unsigned}",
        "PenaltyExcessCharacter {unsigned}",
        "PenaltyReturnTypeOnItsOwnLine {unsigned}",
        "PointerBindsToType {bool}",
        "SpaceBeforeAssignmentOperators {bool}",
        "SpaceBeforeParens {SpaceBeforeParensOptions: SBPO_Never, SBPO_ControlStatements, SBPO_Always}",
        "SpaceInEmptyParentheses {bool}",
        "SpacesBeforeTrailingComments {unsigned}",
        "SpacesInAngles {bool}",
        "SpacesInCStyleCastParentheses {bool}",
        "SpacesInContainerLiterals {bool}",
        "SpacesInParentheses {bool}",
        "Standard {LanguageStandard: LS_Cpp03, LS_Cpp11, LS_Auto}",
        "TabWidth {unsigned}",
        "UseTab {UseTabStyle: UT_Never, UT_ForIndentation, UT_Always}"
    };

    for (const QString& line : lines) {
        const int firstSpace = line.indexOf(' ');
        const QString keyword = line.left(firstSpace);
        const QString options = line.right(line.size() - firstSpace).trimmed();
        const QString text = "<p><span class=\"option\">" + keyword
                + "</span> <span class=\"param\">" + options
                + "</span></p><p>" + tr("No description available.") + "</p>";
        stream.writeStartElement(Constants::DOCUMENTATION_XMLENTRY);
        stream.writeTextElement(Constants::DOCUMENTATION_XMLKEY, keyword);
        stream.writeTextElement(Constants::DOCUMENTATION_XMLDOC, text);
        stream.writeEndElement();
    }

    stream.writeEndElement();
    stream.writeEndDocument();
}

QStringList ClangFormatSettings::completerWords()
{
    return {
        "LLVM",
        "Google",
        "Chromium",
        "Mozilla",
        "WebKit",
        "BS_Attach",
        "BS_Linux",
        "BS_Stroustrup",
        "BS_Allman",
        "NI_None",
        "NI_Inner",
        "NI_All",
        "LS_Cpp03",
        "LS_Cpp11",
        "LS_Auto",
        "UT_Never",
        "UT_ForIndentation",
        "UT_Always"
    };
}

bool ClangFormatSettings::usePredefinedStyle() const
{
    return m_settings.value(USE_PREDEFINED_STYLE).toBool();
}

void ClangFormatSettings::setUsePredefinedStyle(bool usePredefinedStyle)
{
    m_settings.insert(USE_PREDEFINED_STYLE, QVariant(usePredefinedStyle));
}

QString ClangFormatSettings::predefinedStyle() const
{
    return m_settings.value(PREDEFINED_STYLE).toString();
}

void ClangFormatSettings::setPredefinedStyle(const QString &predefinedStyle)
{
    const QStringList test = predefinedStyles();
    if (test.contains(predefinedStyle))
        m_settings.insert(PREDEFINED_STYLE, QVariant(predefinedStyle));
}

QString ClangFormatSettings::customStyle() const
{
    return m_settings.value(CUSTOM_STYLE).toString();
}

void ClangFormatSettings::setCustomStyle(const QString &customStyle)
{
    m_settings.insert(CUSTOM_STYLE, QVariant(customStyle));
}

bool ClangFormatSettings::formatEntireFileFallback() const
{
    return m_settings.value(FORMAT_ENTIRE_FILE_FALLBACK).toBool();
}

void ClangFormatSettings::setFormatEntireFileFallback(bool formatEntireFileFallback)
{
    m_settings.insert(FORMAT_ENTIRE_FILE_FALLBACK, QVariant(formatEntireFileFallback));
}

QStringList ClangFormatSettings::predefinedStyles() const
{
    return {"LLVM", "Google", "Chromium", "Mozilla", "WebKit", "File"};
}

QString ClangFormatSettings::styleFileName(const QString &key) const
{
    return m_styleDir.absolutePath() + '/' + key + '/' + m_ending;
}

void ClangFormatSettings::readStyles()
{
    const QStringList dirs = m_styleDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const QString &dir : dirs) {
        QFile file(m_styleDir.absoluteFilePath(dir + '/' + m_ending));
        if (file.open(QIODevice::ReadOnly))
            m_styles.insert(dir, QString::fromLocal8Bit(file.readAll()));
    }
}

} // namespace ClangFormat
} // namespace Internal
} // namespace Beautifier
