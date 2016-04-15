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
const char kUsePredefinedStyle[] = "usePredefinedStyle";
const char kPredefinedStyle[] = "predefinedStyle";
const char kCustomStyle[] = "customStyle";
const char kFormatEntireFileFallback[] = "formatEntireFileFallback";
}

ClangFormatSettings::ClangFormatSettings() :
    AbstractSettings(QLatin1String(Constants::ClangFormat::SETTINGS_NAME),
                     QLatin1String(".clang-format"))
{
    setCommand(QLatin1String("clang-format"));
    m_settings.insert(QLatin1String(kUsePredefinedStyle), QVariant(true));
    m_settings.insert(QLatin1String(kPredefinedStyle), QLatin1String("LLVM"));
    m_settings.insert(QLatin1String(kCustomStyle), QVariant());
    m_settings.insert(QLatin1String(kFormatEntireFileFallback), QVariant(true));
    read();
}

QString ClangFormatSettings::documentationFilePath() const
{
    return Core::ICore::userResourcePath() + QLatin1Char('/')
            + QLatin1String(Beautifier::Constants::SETTINGS_DIRNAME) + QLatin1Char('/')
            + QLatin1String(Beautifier::Constants::DOCUMENTATION_DIRNAME) + QLatin1Char('/')
            + QLatin1String(Constants::ClangFormat::SETTINGS_NAME) + QLatin1String(".xml");
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
    stream.writeStartDocument(QLatin1String("1.0"), true);
    stream.writeComment(QLatin1String("Created ")
                        + QDateTime::currentDateTime().toString(Qt::ISODate));
    stream.writeStartElement(QLatin1String(Constants::DOCUMENTATION_XMLROOT));

    const QStringList lines = QStringList()
            << QLatin1String("BasedOnStyle {string: LLVM, Google, Chromium, Mozilla, WebKit}")
            << QLatin1String("AccessModifierOffset {int}")
            << QLatin1String("AlignEscapedNewlinesLeft {bool}")
            << QLatin1String("AlignTrailingComments {bool}")
            << QLatin1String("AllowAllParametersOfDeclarationOnNextLine {bool}")
            << QLatin1String("AllowShortFunctionsOnASingleLine {bool}")
            << QLatin1String("AllowShortIfStatementsOnASingleLine {bool}")
            << QLatin1String("AllowShortLoopsOnASingleLine {bool}")
            << QLatin1String("AlwaysBreakBeforeMultilineStrings {bool}")
            << QLatin1String("AlwaysBreakTemplateDeclarations {bool}")
            << QLatin1String("BinPackParameters {bool}")
            << QLatin1String("BreakBeforeBinaryOperators {bool}")
            << QLatin1String("BreakBeforeBraces {BraceBreakingStyle: BS_Attach, BS_Linux, BS_Stroustrup, BS_Allman, BS_GNU}")
            << QLatin1String("BreakBeforeTernaryOperators {bool}")
            << QLatin1String("BreakConstructorInitializersBeforeComma {bool}")
            << QLatin1String("ColumnLimit {unsigned}")
            << QLatin1String("CommentPragmas {string}")
            << QLatin1String("ConstructorInitializerAllOnOneLineOrOnePerLine {bool}")
            << QLatin1String("ConstructorInitializerIndentWidth {unsigned}")
            << QLatin1String("ContinuationIndentWidth {unsigned}")
            << QLatin1String("Cpp11BracedListStyle {bool}")
            << QLatin1String("IndentCaseLabels {bool}")
            << QLatin1String("IndentFunctionDeclarationAfterType {bool}")
            << QLatin1String("IndentWidth {unsigned}")
            << QLatin1String("Language {LanguageKind: LK_None, LK_Cpp, LK_JavaScript, LK_Proto}")
            << QLatin1String("MaxEmptyLinesToKeep {unsigned}")
            << QLatin1String("NamespaceIndentation {NamespaceIndentationKind: NI_None, NI_Inner, NI_All}")
            << QLatin1String("ObjCSpaceAfterProperty {bool}")
            << QLatin1String("ObjCSpaceBeforeProtocolList {bool}")
            << QLatin1String("PenaltyBreakBeforeFirstCallParameter {unsigned}")
            << QLatin1String("PenaltyBreakComment {unsigned}")
            << QLatin1String("PenaltyBreakFirstLessLess {unsigned}")
            << QLatin1String("PenaltyBreakString {unsigned}")
            << QLatin1String("PenaltyExcessCharacter {unsigned}")
            << QLatin1String("PenaltyReturnTypeOnItsOwnLine {unsigned}")
            << QLatin1String("PointerBindsToType {bool}")
            << QLatin1String("SpaceBeforeAssignmentOperators {bool}")
            << QLatin1String("SpaceBeforeParens {SpaceBeforeParensOptions: SBPO_Never, SBPO_ControlStatements, SBPO_Always}")
            << QLatin1String("SpaceInEmptyParentheses {bool}")
            << QLatin1String("SpacesBeforeTrailingComments {unsigned}")
            << QLatin1String("SpacesInAngles {bool}")
            << QLatin1String("SpacesInCStyleCastParentheses {bool}")
            << QLatin1String("SpacesInContainerLiterals {bool}")
            << QLatin1String("SpacesInParentheses {bool}")
            << QLatin1String("Standard {LanguageStandard: LS_Cpp03, LS_Cpp11, LS_Auto}")
            << QLatin1String("TabWidth {unsigned}")
            << QLatin1String("UseTab {UseTabStyle: UT_Never, UT_ForIndentation, UT_Always}");

    foreach (const QString& line, lines) {
        const int firstSpace = line.indexOf(QLatin1Char(' '));
        const QString keyword = line.left(firstSpace);
        const QString options = line.right(line.size() - firstSpace).trimmed();
        const QString text = QLatin1String("<p><span class=\"option\">") + keyword
                + QLatin1String("</span> <span class=\"param\">") + options
                + QLatin1String("</span></p><p>") + tr("No description available.")
                + QLatin1String("</p>");
        stream.writeStartElement(QLatin1String(Constants::DOCUMENTATION_XMLENTRY));
        stream.writeTextElement(QLatin1String(Constants::DOCUMENTATION_XMLKEY), keyword);
        stream.writeTextElement(QLatin1String(Constants::DOCUMENTATION_XMLDOC), text);
        stream.writeEndElement();
    }

    stream.writeEndElement();
    stream.writeEndDocument();
}

QStringList ClangFormatSettings::completerWords()
{
    QStringList words;
    words << QLatin1String("LLVM")
          << QLatin1String("Google")
          << QLatin1String("Chromium")
          << QLatin1String("Mozilla")
          << QLatin1String("WebKit")
          << QLatin1String("BS_Attach")
          << QLatin1String("BS_Linux")
          << QLatin1String("BS_Stroustrup")
          << QLatin1String("BS_Allman")
          << QLatin1String("NI_None")
          << QLatin1String("NI_Inner")
          << QLatin1String("NI_All")
          << QLatin1String("LS_Cpp03")
          << QLatin1String("LS_Cpp11")
          << QLatin1String("LS_Auto")
          << QLatin1String("UT_Never")
          << QLatin1String("UT_ForIndentation")
          << QLatin1String("UT_Always");
    return words;
}

bool ClangFormatSettings::usePredefinedStyle() const
{
    return m_settings.value(QLatin1String(kUsePredefinedStyle)).toBool();
}

void ClangFormatSettings::setUsePredefinedStyle(bool usePredefinedStyle)
{
    m_settings.insert(QLatin1String(kUsePredefinedStyle), QVariant(usePredefinedStyle));
}

QString ClangFormatSettings::predefinedStyle() const
{
    return m_settings.value(QLatin1String(kPredefinedStyle)).toString();
}

void ClangFormatSettings::setPredefinedStyle(const QString &predefinedStyle)
{
    const QStringList test = predefinedStyles();
    if (test.contains(predefinedStyle))
        m_settings.insert(QLatin1String(kPredefinedStyle), QVariant(predefinedStyle));
}

QString ClangFormatSettings::customStyle() const
{
    return m_settings.value(QLatin1String(kCustomStyle)).toString();
}

void ClangFormatSettings::setCustomStyle(const QString &customStyle)
{
    m_settings.insert(QLatin1String(kCustomStyle), QVariant(customStyle));
}

bool ClangFormatSettings::formatEntireFileFallback() const
{
    return m_settings.value(QLatin1String(kFormatEntireFileFallback)).toBool();
}

void ClangFormatSettings::setFormatEntireFileFallback(bool formatEntireFileFallback)
{
    m_settings.insert(QLatin1String(kFormatEntireFileFallback), QVariant(formatEntireFileFallback));
}

QStringList ClangFormatSettings::predefinedStyles() const
{
    return QStringList() << QLatin1String("LLVM")
                         << QLatin1String("Google")
                         << QLatin1String("Chromium")
                         << QLatin1String("Mozilla")
                         << QLatin1String("WebKit")
                         << QLatin1String("File");
}

QString ClangFormatSettings::styleFileName(const QString &key) const
{
    return m_styleDir.absolutePath() + QLatin1Char('/') + key + QLatin1Char('/') + m_ending;
}

void ClangFormatSettings::readStyles()
{
    const QStringList dirs = m_styleDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const QString &dir : dirs) {
        QFile file(m_styleDir.absoluteFilePath(dir + QLatin1Char('/') + m_ending));
        if (file.open(QIODevice::ReadOnly))
            m_styles.insert(dir, QString::fromLocal8Bit(file.readAll()));
    }
}

} // namespace ClangFormat
} // namespace Internal
} // namespace Beautifier
