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

#include "customwizardparameters.h"
#include "customwizardpreprocessor.h"
#include "customwizardscriptgenerator.h"

#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>
#include <cpptools/cpptoolsconstants.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamAttribute>
#include <QTemporaryFile>
#include <QScriptEngine>

#include <QIcon>

enum { debug = 0 };

using namespace Core;

static const char customWizardElementC[] = "wizard";
static const char iconElementC[] = "icon";
static const char descriptionElementC[] = "description";
static const char displayNameElementC[] = "displayname";
static const char wizardEnabledAttributeC[] = "enabled";
static const char idAttributeC[] = "id";
static const char kindAttributeC[] = "kind";
static const char klassAttributeC[] = "class";
static const char firstPageAttributeC[] = "firstpage";
static const char langAttributeC[] = "xml:lang";
static const char categoryAttributeC[] = "category";
static const char displayCategoryElementC[] = "displaycategory";
static const char featuresRequiredC[] = "featuresRequired";
static const char platformIndependentC[] = "platformIndependent";
static const char fieldPageTitleElementC[] = "fieldpagetitle";
static const char fieldsElementC[] = "fields";
static const char fieldElementC[] = "field";
static const char comboEntriesElementC[] = "comboentries";
static const char comboEntryElementC[] = "comboentry";
static const char comboEntryTextElementC[] = "comboentrytext";

static const char generatorScriptElementC[] = "generatorscript";
static const char generatorScriptBinaryAttributeC[] = "binary";
static const char generatorScriptWorkingDirectoryAttributeC[] = "workingdirectory";
static const char generatorScriptArgumentElementC[] = "argument";
static const char generatorScriptArgumentValueAttributeC[] = "value";
static const char generatorScriptArgumentOmitEmptyAttributeC[] = "omit-empty";
static const char generatorScriptArgumentWriteFileAttributeC[] = "write-file";

static const char fieldDescriptionElementC[] = "fielddescription";
static const char fieldNameAttributeC[] = "name";
static const char fieldMandatoryAttributeC[] = "mandatory";
static const char fieldControlElementC[] = "fieldcontrol";

static const char filesElementC[] = "files";
static const char filesGeneratorScriptAttributeC[] = "generatorscript";
static const char fileElementC[] = "file";
static const char fileSourceAttributeC[] = "source";
static const char fileTargetAttributeC[] = "target";
static const char fileBinaryAttributeC[] = "binary";

static const char rulesElementC[] = "validationrules";
static const char ruleElementC[] = "validationrule";
static const char ruleConditionAttributeC[] = "condition";
static const char ruleMessageElementC[] = "message";

enum ParseState {
    ParseBeginning,
    ParseWithinWizard,
    ParseWithinFields,
    ParseWithinField,
    ParseWithinFieldDescription,
    ParseWithinFieldControl,
    ParseWithinComboEntries,
    ParseWithinComboEntry,
    ParseWithinComboEntryText,
    ParseWithinFiles,
    ParseWithinFile,
    ParseWithinScript,
    ParseWithinScriptArguments,
    ParseWithinValidationRules,
    ParseWithinValidationRule,
    ParseWithinValidationRuleMessage,
    ParseError
};

namespace ProjectExplorer {
namespace Internal {

const char customWizardFileOpenEditorAttributeC[] = "openeditor";
const char customWizardFileOpenProjectAttributeC[] = "openproject";

CustomWizardField::CustomWizardField() :
    mandatory(false)
{
}

void CustomWizardField::clear()
{
    mandatory = false;
    name.clear();
    description.clear();
    controlAttributes.clear();
}

// Attribute map keys for combo entries
QString CustomWizardField::comboEntryValueKey(int i)
{
    return QLatin1String("comboValue") + QString::number(i);
}

QString CustomWizardField::comboEntryTextKey(int i)
{
    return QLatin1String("comboText") + QString::number(i);
}

CustomWizardFile::CustomWizardFile() :
        openEditor(false), openProject(false), binary(false)
{
}

/*!
    \class ProjectExplorer::CustomWizardValidationRule
    \brief A custom wizard validation rule based on JavaScript-expressions over
    the field placeholders.

    Placeholder replacement is performed on the condition and it is evaluated
    using JavaScript. So, for example '"%ProjectName%" != "untitled" would block
    default names. On failure, the message is displayed in a red warning label
    in the wizard page. Placeholder replacement is also performed on the message
    prior to displaying.

    \sa ProjectExplorer::CustomWizard, ProjectExplorer::CustomWizardFieldPage
*/

bool CustomWizardValidationRule::validateRules(const QList<CustomWizardValidationRule> &rules,
                                               const QMap<QString, QString> &replacementMap,
                                               QString *errorMessage)
{
    errorMessage->clear();
    if (rules.isEmpty())
        return true;
    QScriptEngine engine;
    foreach (const CustomWizardValidationRule &rule, rules)
    if (!rule.validate(engine, replacementMap)) {
        *errorMessage = rule.message;
        CustomWizardContext::replaceFields(replacementMap, errorMessage);
        return false;
    }
    return true;
}

bool CustomWizardValidationRule::validate(QScriptEngine &engine, const QMap<QString, QString> &replacementMap) const
{
    // Apply parameters and evaluate using JavaScript
    QString cond = condition;
    CustomWizardContext::replaceFields(replacementMap, &cond);
    bool valid = false;
    QString errorMessage;
    if (!evaluateBooleanJavaScriptExpression(engine, cond, &valid, &errorMessage)) {
        qWarning("Error in custom wizard validation expression '%s': %s",
                 qPrintable(cond), qPrintable(errorMessage));
        return false;
    }
    return valid;
}

CustomWizardParameters::CustomWizardParameters() :
        firstPageId(-1)
{
}

void CustomWizardParameters::clear()
{
    directory.clear();
    files.clear();
    fields.clear();
    filesGeneratorScript.clear();
    filesGeneratorScriptArguments.clear();
    firstPageId = -1;
    rules.clear();
}

// Resolve icon file path relative to config file directory.
static inline QIcon wizardIcon(const QString &configFileFullPath,
                               const QString &xmlIconFileName)
{
    const QFileInfo fi(xmlIconFileName);
    if (fi.isFile())
        return QIcon(fi.absoluteFilePath());
    if (!fi.isRelative())
        return QIcon();
    // Expand by config path
    const QFileInfo absFi(QFileInfo(configFileFullPath).absolutePath() +
                          QLatin1Char('/') + xmlIconFileName);
    if (absFi.isFile())
        return QIcon(absFi.absoluteFilePath());
    return QIcon();
}

// Forward a reader over element text
static inline bool skipOverElementText(QXmlStreamReader &reader)
{
    QXmlStreamReader::TokenType next = QXmlStreamReader::EndElement;
    do {
        next = reader.readNext();
    } while (next == QXmlStreamReader::Characters || next == QXmlStreamReader::EntityReference
             || next == QXmlStreamReader::ProcessingInstruction || next == QXmlStreamReader::Comment);
    return next == QXmlStreamReader::EndElement;
}

// Helper for reading text element depending on a language.
// Assign the element text to the string passed on if the language matches,
// that is, the element has no language attribute or there is an exact match.
// If there is no match, skip over the element text.
static inline bool assignLanguageElementText(QXmlStreamReader &reader,
                                             const QString &desiredLanguage,
                                             QString *target)
{
    const QStringRef elementLanguage = reader.attributes().value(QLatin1String(langAttributeC));
    if (elementLanguage.isEmpty()) {
        // Try to find a translation for our built-in Wizards
        *target = QCoreApplication::translate("ProjectExplorer::CustomWizard", reader.readElementText().toLatin1().constData());
        return true;
    }
    if (elementLanguage == desiredLanguage) {
        *target = reader.readElementText();
        return true;
    }
    // Language mismatch: forward to end element.
    skipOverElementText(reader);
    return false;
}

// Copy&paste from above to call a setter of BaseFileParameters.
// Implementation of a sophisticated mem_fun pattern is left
// as an exercise to the reader.
static inline bool assignLanguageElementText(QXmlStreamReader &reader,
                                             const QString &desiredLanguage,
                                             BaseFileWizardParameters *bp,
                                             void (BaseFileWizardParameters::*setter)(const QString &))
{
    const QStringRef elementLanguage = reader.attributes().value(QLatin1String(langAttributeC));
    if (elementLanguage.isEmpty()) {
        // Try to find a translation for our built-in Wizards
        const QString translated = QCoreApplication::translate("ProjectExplorer::CustomWizard", reader.readElementText().toLatin1().constData());
        (bp->*setter)(translated);
        return true;
    }
    if (elementLanguage == desiredLanguage) {
        (bp->*setter)(reader.readElementText());
        return true;
    }
    // Language mismatch: forward to end element.
    skipOverElementText(reader);
    return false;
}

// Read level sub-elements of "wizard"
static bool parseCustomProjectElement(QXmlStreamReader &reader,
                                      const QString &configFileFullPath,
                                      const QString &language,
                                      CustomWizardParameters *p,
                                      BaseFileWizardParameters *bp)
{
    const QStringRef elementName = reader.name();
    if (elementName == QLatin1String(iconElementC)) {
        const QString path = reader.readElementText();
        const QIcon icon = wizardIcon(configFileFullPath, path);
        if (icon.availableSizes().isEmpty()) {
            qWarning("Invalid icon path '%s' encountered in custom project template %s.",
                     qPrintable(path), qPrintable(configFileFullPath));
        } else {
                bp->setIcon(icon);
        }
        return true;
    }
    if (elementName == QLatin1String(descriptionElementC)) {
        assignLanguageElementText(reader, language, bp,
                                  &BaseFileWizardParameters::setDescription);
        return true;
    }
    if (elementName == QLatin1String(displayNameElementC)) {
        assignLanguageElementText(reader, language, bp,
                                  &BaseFileWizardParameters::setDisplayName);
        return true;
    }
    if (elementName == QLatin1String(displayCategoryElementC)) {
        assignLanguageElementText(reader, language, bp,
                                  &BaseFileWizardParameters::setDisplayCategory);
        return true;
    }
    if (elementName == QLatin1String(fieldPageTitleElementC)) {
        assignLanguageElementText(reader, language, &p->fieldPageTitle);
        return true;
    }
    return false;
}

static inline QMap<QString, QString> attributesToStringMap(const QXmlStreamAttributes &attributes)
{
    QMap<QString, QString> rc;
    foreach (const QXmlStreamAttribute &attribute, attributes)
        rc.insert(attribute.name().toString(), attribute.value().toString());
    return rc;
}

// Switch parser state depending on opening element name.
static ParseState nextOpeningState(ParseState in, const QStringRef &name)
{
    switch (in) {
    case ParseBeginning:
        if (name == QLatin1String(customWizardElementC))
            return ParseWithinWizard;
        break;
    case ParseWithinWizard:
        if (name == QLatin1String(fieldsElementC))
            return ParseWithinFields;
        if (name == QLatin1String(filesElementC))
            return ParseWithinFiles;
        if (name == QLatin1String(generatorScriptElementC))
            return ParseWithinScript;
        if (name == QLatin1String(rulesElementC))
            return ParseWithinValidationRules;
        break;
    case ParseWithinFields:
        if (name == QLatin1String(fieldElementC))
            return ParseWithinField;
        break;
    case ParseWithinField:
        if (name == QLatin1String(fieldDescriptionElementC))
            return ParseWithinFieldDescription;
        if (name == QLatin1String(fieldControlElementC))
            return ParseWithinFieldControl;
        break;
    case ParseWithinFieldControl:
        if (name == QLatin1String(comboEntriesElementC))
            return ParseWithinComboEntries;
        break;
    case ParseWithinComboEntries:
        if (name == QLatin1String(comboEntryElementC))
            return ParseWithinComboEntry;
        break;
    case ParseWithinComboEntry:
        if (name == QLatin1String(comboEntryTextElementC))
            return ParseWithinComboEntryText;
        break;
    case ParseWithinFiles:
        if (name == QLatin1String(fileElementC))
            return ParseWithinFile;
        break;
    case ParseWithinScript:
        if (name == QLatin1String(generatorScriptArgumentElementC))
            return ParseWithinScriptArguments;
        break;
    case ParseWithinValidationRules:
        if (name == QLatin1String(ruleElementC))
            return ParseWithinValidationRule;
        break;
    case ParseWithinValidationRule:
        if (name == QLatin1String(ruleMessageElementC))
            return ParseWithinValidationRuleMessage;
        break;
    case ParseWithinFieldDescription: // No subelements
    case ParseWithinComboEntryText:
    case ParseWithinFile:
    case ParseError:
    case ParseWithinScriptArguments:
    case ParseWithinValidationRuleMessage:
        break;
    }
    return ParseError;
}

// Switch parser state depending on closing element name.
static ParseState nextClosingState(ParseState in, const QStringRef &name)
{
    switch (in) {
    case ParseBeginning:
        break;
    case ParseWithinWizard:
        if (name == QLatin1String(customWizardElementC))
            return ParseBeginning;
        break;
    case ParseWithinFields:
        if (name == QLatin1String(fieldsElementC))
            return ParseWithinWizard;
        break;
    case ParseWithinField:
        if (name == QLatin1String(fieldElementC))
            return ParseWithinFields;
        break;
    case ParseWithinFiles:
        if (name == QLatin1String(filesElementC))
            return ParseWithinWizard;
        break;
    case ParseWithinFile:
        if (name == QLatin1String(fileElementC))
            return ParseWithinFiles;
        break;
    case ParseWithinFieldDescription:
        if (name == QLatin1String(fieldDescriptionElementC))
            return ParseWithinField;
        break;
    case ParseWithinFieldControl:
        if (name == QLatin1String(fieldControlElementC))
            return ParseWithinField;
        break;
    case ParseWithinComboEntries:
        if (name == QLatin1String(comboEntriesElementC))
            return ParseWithinFieldControl;
        break;
    case ParseWithinComboEntry:
        if (name == QLatin1String(comboEntryElementC))
            return ParseWithinComboEntries;
        break;
    case ParseWithinComboEntryText:
        if (name == QLatin1String(comboEntryTextElementC))
            return ParseWithinComboEntry;
        break;
    case ParseWithinScript:
        if (name == QLatin1String(generatorScriptElementC))
            return ParseWithinWizard;
        break;
    case ParseWithinScriptArguments:
        if (name == QLatin1String(generatorScriptArgumentElementC))
            return ParseWithinScript;
        break;
    case ParseWithinValidationRuleMessage:
        return ParseWithinValidationRule;
    case ParseWithinValidationRule:
        return ParseWithinValidationRules;
    case ParseWithinValidationRules:
        return ParseWithinWizard;
    case ParseError:
        break;
    }
    return ParseError;
}

// Parse kind attribute
static inline IWizard::WizardKind kindAttribute(const QXmlStreamReader &r)
{
    const QStringRef value = r.attributes().value(QLatin1String(kindAttributeC));
    if (!value.isEmpty()) {
        if (value == QLatin1String("file"))
            return IWizard::FileWizard;
        if (value == QLatin1String("class"))
            return IWizard::ClassWizard;
    }
    return IWizard::ProjectWizard;
}

static inline FeatureSet requiredFeatures(const QXmlStreamReader &reader)
{
    FeatureSet r;
    QString value = reader.attributes().value(QLatin1String(featuresRequiredC)).toString();
    QStringList stringList = value.split(QLatin1Char(','), QString::SkipEmptyParts);
    FeatureSet features;
    foreach (const QString &string, stringList) {
        Feature feature(Id::fromString(string));
        features |= feature;
    }
    return features;
}

static inline IWizard::WizardFlags wizardFlags(const QXmlStreamReader &reader)
{
    IWizard::WizardFlags flags;
    QString value = reader.attributes().value(QLatin1String(platformIndependentC)).toString();

    if (!value.isEmpty() && value == QLatin1String("true"))
        flags |= IWizard::PlatformIndependent;

    return flags;
}

static inline QString msgError(const QXmlStreamReader &reader,
                               const QString &fileName,
                               const QString &what)
{
    return QString::fromLatin1("Error in %1 at line %2, column %3: %4").
            arg(fileName).arg(reader.lineNumber()).arg(reader.columnNumber()).arg(what);
}

static inline bool booleanAttributeValue(const QXmlStreamReader &r, const char *nameC,
                                         bool defaultValue)
{
    const QStringRef attributeValue = r.attributes().value(QLatin1String(nameC));
    if (attributeValue.isEmpty())
        return defaultValue;
    return attributeValue == QLatin1String("true");
}

static inline int integerAttributeValue(const QXmlStreamReader &r, const char *name, int defaultValue)
{
    const QString sValue = r.attributes().value(QLatin1String(name)).toString();
    if (sValue.isEmpty())
        return defaultValue;
    bool ok;
    const int value = sValue.toInt(&ok);
    if (!ok) {
        qWarning("Invalid integer value specification '%s' for attribute '%s'.",
                 qPrintable(sValue), name);
        return defaultValue;
    }
    return value;
}

static inline QString attributeValue(const QXmlStreamReader &r, const char *name)
{
    return r.attributes().value(QLatin1String(name)).toString();
}

// Return locale language attribute "de_UTF8" -> "de", empty string for "C"
static inline QString languageSetting()
{
    QString name = ICore::userInterfaceLanguage();
    const int underScorePos = name.indexOf(QLatin1Char('_'));
    if (underScorePos != -1)
        name.truncate(underScorePos);
    if (name.compare(QLatin1String("C"), Qt::CaseInsensitive) == 0)
        name.clear();
    return name;
}

/*!
    \class ProjectExplorer::Internal::GeneratorScriptArgument
    \brief Argument to a custom wizard generator script.

    Contains placeholders to be replaced by field values or file names
    as in \c '--class-name=%ClassName%' or \c '--description=%Description%'.

    \sa ProjectExplorer::CustomWizard
*/

GeneratorScriptArgument::GeneratorScriptArgument(const QString &v) :
    value(v), flags(0)
{
}

// Main parsing routine
CustomWizardParameters::ParseResult
     CustomWizardParameters::parse(QIODevice &device,
                                   const QString &configFileFullPath,
                                   BaseFileWizardParameters *bp,
                                   QString *errorMessage)
{
    int comboEntryCount = 0;
    QXmlStreamReader reader(&device);
    QXmlStreamReader::TokenType token = QXmlStreamReader::EndDocument;
    ParseState state = ParseBeginning;
    clear();
    bp->clear();
    bp->setKind(IWizard::ProjectWizard);
    const QString language = languageSetting();
    CustomWizardField field;
    do {
        token = reader.readNext();
        switch (token) {
        case QXmlStreamReader::Invalid:
            *errorMessage = msgError(reader, configFileFullPath, reader.errorString());
            return ParseFailed;
        case QXmlStreamReader::StartElement:
            do {
                // Read out subelements applicable to current state
                if (state == ParseWithinWizard && parseCustomProjectElement(reader, configFileFullPath, language, this, bp))
                    break;
                // switch to next state
                state = nextOpeningState(state, reader.name());
                // Read attributes post state-switching
                switch (state) {
                case ParseError:
                    *errorMessage = msgError(reader, configFileFullPath,
                                             QString::fromLatin1("Unexpected start element %1").arg(reader.name().toString()));
                    return ParseFailed;
                case ParseWithinWizard:
                    if (!booleanAttributeValue(reader, wizardEnabledAttributeC, true))
                        return ParseDisabled;
                    bp->setId(attributeValue(reader, idAttributeC));
                    bp->setCategory(attributeValue(reader, categoryAttributeC));
                    bp->setKind(kindAttribute(reader));
                    bp->setRequiredFeatures(requiredFeatures(reader));
                    bp->setFlags(wizardFlags(reader));
                    klass = attributeValue(reader, klassAttributeC);
                    firstPageId = integerAttributeValue(reader, firstPageAttributeC, -1);
                    break;
                case ParseWithinField: // field attribute
                    field.name = attributeValue(reader, fieldNameAttributeC);
                    field.mandatory = booleanAttributeValue(reader, fieldMandatoryAttributeC, false);
                    break;
                case ParseWithinFieldDescription:
                    assignLanguageElementText(reader, language, &field.description);
                    state = ParseWithinField; // The above reads away the end tag, set state.
                    break;
                case ParseWithinFieldControl: // Copy widget control attributes
                    field.controlAttributes = attributesToStringMap(reader.attributes());
                    break;
                case ParseWithinComboEntries:
                    break;
                case ParseWithinComboEntry: // Combo entry with 'value' attribute
                    field.controlAttributes.insert(CustomWizardField::comboEntryValueKey(comboEntryCount),
                                                   attributeValue(reader, "value"));
                    break;
                case ParseWithinComboEntryText: {

                        // This reads away the end tag, set state here.
                        QString text;
                        if (assignLanguageElementText(reader, language, &text))
                            field.controlAttributes.insert(CustomWizardField::comboEntryTextKey(comboEntryCount),
                                                           text);
                        state = ParseWithinComboEntry;
                }
                     break;
                case ParseWithinFiles:
                    break;
                case ParseWithinFile: { // file attribute
                        CustomWizardFile file;
                        file.source = attributeValue(reader, fileSourceAttributeC);
                        file.target = attributeValue(reader, fileTargetAttributeC);
                        file.openEditor = booleanAttributeValue(reader, customWizardFileOpenEditorAttributeC, false);
                        file.openProject = booleanAttributeValue(reader, customWizardFileOpenProjectAttributeC, false);
                        file.binary = booleanAttributeValue(reader, fileBinaryAttributeC, false);
                        if (file.target.isEmpty())
                            file.target = file.source;
                        if (file.source.isEmpty())
                            qWarning("Skipping empty file name in custom project.");
                        else
                            files.push_back(file);
                    }
                    break;
                case ParseWithinScript:
                    filesGeneratorScript = fixGeneratorScript(configFileFullPath, attributeValue(reader, generatorScriptBinaryAttributeC));
                    if (filesGeneratorScript.isEmpty()) {
                        *errorMessage = QString::fromLatin1("No binary specified for generator script.");
                        return ParseFailed;
                    }
                    filesGeneratorScriptWorkingDirectory = attributeValue(reader, generatorScriptWorkingDirectoryAttributeC);
                    break;
                case ParseWithinScriptArguments: {
                    GeneratorScriptArgument argument(attributeValue(reader, generatorScriptArgumentValueAttributeC));
                    if (argument.value.isEmpty()) {
                        *errorMessage = QString::fromLatin1("No value specified for generator script argument.");
                        return ParseFailed;
                    }
                    if (booleanAttributeValue(reader, generatorScriptArgumentOmitEmptyAttributeC, false))
                        argument.flags |= GeneratorScriptArgument::OmitEmpty;
                    if (booleanAttributeValue(reader, generatorScriptArgumentWriteFileAttributeC, false))
                        argument.flags |= GeneratorScriptArgument::WriteFile;
                    filesGeneratorScriptArguments.push_back(argument);
                }
                    break;
                case ParseWithinValidationRule: {
                    CustomWizardValidationRule rule;
                    rule.condition = reader.attributes().value(QLatin1String(ruleConditionAttributeC)).toString();
                    rules.push_back(rule);
                }
                    break;
                case ParseWithinValidationRuleMessage:
                    QTC_ASSERT(!rules.isEmpty(), return ParseFailed);
                    // This reads away the end tag, set state here.
                    assignLanguageElementText(reader, language, &(rules.back().message));
                    state = ParseWithinValidationRule;
                    break;
                default:
                    break;
                }
            } while (false);
            break;
        case QXmlStreamReader::EndElement:
            state = nextClosingState(state, reader.name());
            switch (state) {
            case ParseError:
                *errorMessage = msgError(reader, configFileFullPath,
                                         QString::fromLatin1("Unexpected end element %1").arg(reader.name().toString()));
                return ParseFailed;
            case ParseWithinFields:  // Leaving a field element
                fields.push_back(field);
                field.clear();
                comboEntryCount = 0;
                break;
            case ParseWithinComboEntries:
                comboEntryCount++;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    } while (token != QXmlStreamReader::EndDocument);
    return ParseOk;
}

CustomWizardParameters::ParseResult
     CustomWizardParameters::parse(const QString &configFileFullPath,
                                   BaseFileWizardParameters *bp,
                                   QString *errorMessage)
{
    QFile configFile(configFileFullPath);
    if (!configFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        *errorMessage = QString::fromLatin1("Cannot open %1: %2").arg(configFileFullPath, configFile.errorString());
        return ParseFailed;
    }
    return parse(configFile, configFileFullPath, bp, errorMessage);
}

QString CustomWizardParameters::toString() const
{
    QString rc;
    QTextStream str(&rc);
    str << "Directory: " << directory << " Klass: '" << klass << "'\n";
    if (!filesGeneratorScriptArguments.isEmpty()) {
        str << "Script:";
        foreach (const QString &a, filesGeneratorScript)
            str << " '" << a << '\'';
        if (!filesGeneratorScriptWorkingDirectory.isEmpty())
            str << "\nrun in '" <<  filesGeneratorScriptWorkingDirectory << '\'';
        str << "\nArguments: ";
        foreach (const GeneratorScriptArgument &a, filesGeneratorScriptArguments) {
            str << " '" << a.value  << '\'';
            if (a.flags & GeneratorScriptArgument::OmitEmpty)
                str << " [omit empty]";
            if (a.flags & GeneratorScriptArgument::WriteFile)
                str << " [write file]";
            str << ',';
        }
        str << '\n';
    }
    foreach (const CustomWizardFile &f, files) {
        str << "  File source: " << f.source << " Target: " << f.target;
        if (f.openEditor)
            str << " [editor]";
        if (f.openProject)
            str << " [project]";
        if (f.binary)
            str << " [binary]";
        str << '\n';
    }
    foreach (const CustomWizardField &f, fields) {
        str << "  Field name: " << f.name;
        if (f.mandatory)
            str << '*';
        str << " Description: '" << f.description << '\'';
        if (!f.controlAttributes.isEmpty()) {
            typedef CustomWizardField::ControlAttributeMap::const_iterator AttrMapConstIt;
            str << " Control: ";
            const AttrMapConstIt cend = f.controlAttributes.constEnd();
            for (AttrMapConstIt it = f.controlAttributes.constBegin(); it != cend; ++it)
                str << '\'' << it.key() << "' -> '" << it.value() << "' ";
        }
        str << '\n';
    }
    foreach (const CustomWizardValidationRule &r, rules)
            str << "  Rule: '" << r.condition << "'->'" << r.message << '\n';
    return rc;
}

// ------------ CustomWizardContext

static inline QString passThrough(const QString &in) { return in; }

static inline QString headerGuard(const QString &in)
{
    QString result;
    foreach (const QChar c, in) {
        if (c.isLetterOrNumber())
            result.append(c.toUpper());
        else
            result.append(QLatin1Char('_'));
    }
    return result;
}

static inline QString structName(const QString &in)
{
    bool capNeeded = true;
    QString result;
    foreach (const QChar c, in) {
        if (c.isLetterOrNumber()) {
            if (capNeeded) {
                result.append(c.toUpper());
                capNeeded = false;
            } else {
                result.append(c);
            }
        } else {
            result.append(QLatin1Char('_'));
            capNeeded = true;
        }
    }
    return result;
}

// Do field replacements applying modifiers and string transformation
// for the value
template <class ValueStringTransformation>
bool replaceFieldHelper(ValueStringTransformation transform,
                        const CustomWizardContext::FieldReplacementMap &fm,
                        QString *s)
{
    bool nonEmptyReplacements = false;
    if (debug) {
        qDebug().nospace() << "CustomWizardContext::replaceFields with " <<
                fm << *s;
    }
    const QChar delimiter = QLatin1Char('%');
    const QChar modifierDelimiter = QLatin1Char(':');
    int pos = 0;
    while (pos < s->size()) {
        pos = s->indexOf(delimiter, pos);
        if (pos < 0)
            break;
        int nextPos = s->indexOf(delimiter, pos + 1);
        if (nextPos == -1)
            break;
        nextPos++; // Point past 2nd delimiter
        if (nextPos == pos + 2) {
            pos = nextPos; // Skip '%%'
            continue;
        }
        // Evaluate field specification for modifiers
        // "%field:l%"
        QString fieldSpec = s->mid(pos + 1, nextPos - pos - 2);
        const int fieldSpecSize = fieldSpec.size();
        char modifier = '\0';
        if (fieldSpecSize >= 3 && fieldSpec.at(fieldSpecSize - 2) == modifierDelimiter) {
            modifier = fieldSpec.at(fieldSpecSize - 1).toLatin1();
            fieldSpec.truncate(fieldSpecSize - 2);
        }
        const CustomWizardContext::FieldReplacementMap::const_iterator it = fm.constFind(fieldSpec);
        if (it == fm.constEnd()) {
            pos = nextPos; // Not found, skip
            continue;
        }
        // Assign
        QString replacement = it.value();
        switch (modifier) {
        case 'l':
            replacement = it.value().toLower();
            break;
        case 'u':
            replacement = it.value().toUpper();
            break;
        case 'h': // Create a header guard.
            replacement = headerGuard(it.value());
            break;
        case 's': // Create a struct or class name.
            replacement = structName(it.value());
            break;
        case 'c': // Capitalize first letter.
            replacement = it.value();
            if (!replacement.isEmpty())
                replacement[0] = replacement.at(0).toUpper();
            break;
        default:
            break;
        }
        if (!replacement.isEmpty())
            nonEmptyReplacements = true;
        // Apply transformation to empty values as well.
        s->replace(pos, nextPos - pos, transform(replacement));
        nonEmptyReplacements = true;
        pos += replacement.size();
    }
    return nonEmptyReplacements;
}

/*!
    \brief Performs field replacements.

    Replace field values delimited by '%' with special modifiers:
    \list
    \li %Field% -> simple replacement
    \li %Field:l% -> replace with everything changed to lower case
    \li %Field:u% -> replace with everything changed to upper case
    \li %Field:c% -> replace with first character capitalized
    \li  %Field:h% -> replace with something usable as header guard
    \li %Field:s% -> replace with something usable as structure or class name
    \endlist

    The return value indicates whether non-empty replacements were encountered.
*/

bool CustomWizardContext::replaceFields(const FieldReplacementMap &fm, QString *s)
{
    return replaceFieldHelper(passThrough, fm, s);
}

// Transformation to be passed to replaceFieldHelper(). Writes the
// value to a text file and returns the file name to be inserted
// instead of the expanded field in the parsed template,
// used for the arguments of a generator script.
class TemporaryFileTransform {
public:
    typedef CustomWizardContext::TemporaryFilePtr TemporaryFilePtr;
    typedef CustomWizardContext::TemporaryFilePtrList TemporaryFilePtrList;

    explicit TemporaryFileTransform(TemporaryFilePtrList *f);

    QString operator()(const QString &) const;

private:
    TemporaryFilePtrList *m_files;
    QString m_pattern;
};

TemporaryFileTransform::TemporaryFileTransform(TemporaryFilePtrList *f) :
    m_files(f), m_pattern(QDir::tempPath())
{
    if (!m_pattern.endsWith(QLatin1Char('/')))
        m_pattern += QLatin1Char('/');
    m_pattern += QLatin1String("qtcreatorXXXXXX.txt");
}

QString TemporaryFileTransform::operator()(const QString &value) const
{
    TemporaryFilePtr temporaryFile(new QTemporaryFile(m_pattern));
    QTC_ASSERT(temporaryFile->open(), return QString());

    temporaryFile->write(value.toLocal8Bit());
    const QString name = temporaryFile->fileName();
    temporaryFile->flush();
    temporaryFile->close();
    m_files->push_back(temporaryFile);
    return name;
}

/*!
    \class ProjectExplorer::Internal::CustomWizardContext
    \brief Context used for one custom wizard run.

    Shared between CustomWizard and the CustomWizardPage as it is used
    for the QLineEdit-type fields'
    default texts as well. Contains basic replacement fields
    like  \c '%CppSourceSuffix%', \c '%CppHeaderSuffix%' (settings-dependent)
    reset() should be called before each wizard run to refresh them.
    CustomProjectWizard additionally inserts \c '%ProjectName%' from
    the intro page to have it available for default texts.

    \sa ProjectExplorer::CustomWizard
*/

/*!
    \brief Special replaceFields() overload used for the arguments of a generator
    script.

    Write the expanded field values out to temporary files and
    inserts file names instead of the expanded fields in string 's'.
*/

bool CustomWizardContext::replaceFields(const FieldReplacementMap &fm, QString *s,
                                        TemporaryFilePtrList *files)
{
    return replaceFieldHelper(TemporaryFileTransform(files), fm, s);
}

void CustomWizardContext::reset()
{
    // Basic replacement fields: Suffixes.
    baseReplacements.clear();
    const MimeDatabase *mdb = ICore::mimeDatabase();
    baseReplacements.insert(QLatin1String("CppSourceSuffix"),
                            mdb->preferredSuffixByType(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)));
    baseReplacements.insert(QLatin1String("CppHeaderSuffix"),
                            mdb->preferredSuffixByType(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)));
    replacements.clear();
    path.clear();
    targetPath.clear();
}

QString CustomWizardContext::processFile(const FieldReplacementMap &fm, QString in)
{

    if (in.isEmpty())
        return in;

    if (!fm.isEmpty())
        replaceFields(fm, &in);

    QString out;
    QString errorMessage;
    if (!customWizardPreprocess(in, &out, &errorMessage)) {
        qWarning("Error preprocessing custom widget file: %s\nFile:\n%s",
                 qPrintable(errorMessage), qPrintable(in));
        return QString();
    }
    return out;
}

} // namespace Internal
} // namespace ProjectExplorer
