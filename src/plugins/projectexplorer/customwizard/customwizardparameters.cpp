/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "customwizardparameters.h"

#include <QtCore/QDebug>
#include <QtCore/QLocale>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamAttribute>
#include <QtGui/QIcon>

enum { debug = 0 };

static const char customWizardElementC[] = "wizard";
static const char iconElementC[] = "icon";
static const char descriptionElementC[] = "description";
static const char displayNameElementC[] = "displayName";
static const char idAttributeC[] = "id";
static const char kindAttributeC[] = "kind";
static const char klassAttributeC[] = "class";
static const char firstPageAttributeC[] = "firstpage";
static const char langAttributeC[] = "xml:lang";
static const char categoryAttributeC[] = "category";
static const char displayCategoryElementC[] = "displayCategory";
static const char fieldPageTitleElementC[] = "fieldpagetitle";
static const char fieldsElementC[] = "fields";
static const char fieldElementC[] = "field";

static const char fieldDescriptionElementC[] = "fielddescription";
static const char fieldNameAttributeC[] = "name";
static const char fieldMandatoryAttributeC[] = "mandatory";
static const char fieldControlElementC[] = "fieldcontrol";

static const char filesElementC[] = "files";
static const char fileElementC[] = "file";
static const char fileNameSourceAttributeC[] = "source";
static const char fileNameTargetAttributeC[] = "target";

enum ParseState {
    ParseBeginning,
    ParseWithinWizard,
    ParseWithinFields,
    ParseWithinField,
    ParseWithinFiles,
    ParseWithinFile,
    ParseError
};

namespace ProjectExplorer {
namespace Internal {

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

CustomWizardParameters::CustomWizardParameters() :
        firstPageId(-1)
{
}

void CustomWizardParameters::clear()
{
    directory.clear();
    files.clear();
    fields.clear();
    firstPageId = -1;
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
static inline void assignLanguageElementText(QXmlStreamReader &reader,
                                             const QString &desiredLanguage,
                                             QString *target)
{
    const QStringRef elementLanguage = reader.attributes().value(langAttributeC);
    if (elementLanguage.isEmpty() || elementLanguage == desiredLanguage) {
        *target = reader.readElementText();
    } else {
        // Language mismatch: forward to end element.
        skipOverElementText(reader);
    }
}

// Copy&paste from above to call a setter of BaseFileParameters.
// Implementation of a sophisticated mem_fun pattern is left
// as an exercise to the reader.
static inline void assignLanguageElementText(QXmlStreamReader &reader,
                                             const QString &desiredLanguage,
                                             Core::BaseFileWizardParameters *bp,
                                             void (Core::BaseFileWizardParameters::*setter)(const QString &))
{
    const QStringRef elementLanguage = reader.attributes().value(langAttributeC);
    if (elementLanguage.isEmpty() || elementLanguage == desiredLanguage) {
        (bp->*setter)(reader.readElementText());
    } else {
        // Language mismatch: forward to end element.
        skipOverElementText(reader);
    }
}

// Read level sub-elements of "wizard"
static bool parseCustomProjectElement(QXmlStreamReader &reader,
                                      const QString &configFileFullPath,
                                      const QString &language,
                                      CustomWizardParameters *p,
                                      Core::BaseFileWizardParameters *bp)
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
                                  &Core::BaseFileWizardParameters::setDescription);
        return true;
    }
    if (elementName == QLatin1String(displayNameElementC)) {
        assignLanguageElementText(reader, language, bp,
                                  &Core::BaseFileWizardParameters::setDisplayName);
        return true;
    }
    if (elementName == QLatin1String(displayCategoryElementC)) {
        assignLanguageElementText(reader, language, bp,
                                  &Core::BaseFileWizardParameters::setDisplayCategory);
        return true;
    }
    if (elementName == QLatin1String(fieldPageTitleElementC)) {
        assignLanguageElementText(reader, language, &p->fieldPageTitle);
        return true;
    }
    return false;
}

// Read sub-elements of "fields"
static bool parseFieldElement(QXmlStreamReader &reader,
                              const QString &language,
                              CustomWizardField *m)
{
    const QStringRef elementName = reader.name();
    if (elementName == QLatin1String(fieldDescriptionElementC)) {
        assignLanguageElementText(reader, language, &m->description);
        return true;
    }
    // Copy widget control attributes
    if (elementName == QLatin1String(fieldControlElementC)) {
        foreach(const QXmlStreamAttribute &attribute, reader.attributes())
            m->controlAttributes.insert(attribute.name().toString(), attribute.value().toString());
        skipOverElementText(reader);
        return true;
    }
    return false;
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
        break;
    case ParseWithinFields:
        if (name == QLatin1String(fieldElementC))
            return ParseWithinField;
        break;
    case ParseWithinFiles:
        if (name == QLatin1String(fileElementC))
            return ParseWithinFile;
        break;
    case ParseWithinField:
    case ParseWithinFile:
    case ParseError:
        break;
    }
    return ParseError;
};

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
    case ParseError:
        break;
    }
    return ParseError;
};

// Parse kind attribute
static inline Core::IWizard::WizardKind kindAttribute(const QXmlStreamReader &r)
{
    const QStringRef value = r.attributes().value(QLatin1String(kindAttributeC));
    if (!value.isEmpty()) {
        if (value == QLatin1String("file"))
            return Core::IWizard::FileWizard;
        if (value == QLatin1String("class"))
            return Core::IWizard::ClassWizard;
    }
    return Core::IWizard::ProjectWizard;
}

static inline QString msgError(const QXmlStreamReader &reader,
                               const QString &fileName,
                               const QString &what)
{
    return QString::fromLatin1("Error in %1 at line %2, column %3: %4").
            arg(fileName).arg(reader.lineNumber()).arg(reader.columnNumber()).arg(what);
}

static inline bool booleanAttributeValue(const QXmlStreamReader &r, const char *name)
{
    return r.attributes().value(QLatin1String(name)) == QLatin1String("true");
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
static inline QString localeLanguage()
{
    QLocale loc;
    QString name = loc.name();
    const int underScorePos = name.indexOf(QLatin1Char('_'));
    if (underScorePos != -1)
        name.truncate(underScorePos);
    if (name.compare(QLatin1String("C"), Qt::CaseInsensitive) == 0)
        name.clear();
    return name;
}

// Main parsing routine
bool CustomWizardParameters::parse(QIODevice &device,
                                   const QString &configFileFullPath,
                                   Core::BaseFileWizardParameters *bp,
                                   QString *errorMessage)
{
    QXmlStreamReader reader(&device);
    QXmlStreamReader::TokenType token = QXmlStreamReader::EndDocument;
    ParseState state = ParseBeginning;
    clear();
    bp->clear();
    bp->setKind(Core::IWizard::ProjectWizard);
    const QString language = localeLanguage();
    CustomWizardField field;
    do {
        token = reader.readNext();
        switch (token) {
        case QXmlStreamReader::Invalid:
            *errorMessage = msgError(reader, configFileFullPath, reader.errorString());
            return false;
        case QXmlStreamReader::StartElement:
            do {
                // Read out subelements applicable to current state
                if (state == ParseWithinWizard && parseCustomProjectElement(reader, configFileFullPath, language, this, bp))
                    break;
                if (state == ParseWithinField && parseFieldElement(reader, language, &field))
                    break;
                // switch to next state
                state = nextOpeningState(state, reader.name());
                // Read attributes post state-switching
                switch (state) {
                case ParseError:
                    *errorMessage = msgError(reader, configFileFullPath,
                                             QString::fromLatin1("Unexpected start element %1").arg(reader.name().toString()));
                    return false;
                case ParseWithinWizard:
                    bp->setId(attributeValue(reader, idAttributeC));
                    bp->setCategory(attributeValue(reader, categoryAttributeC));
                    bp->setKind(kindAttribute(reader));
                    klass = attributeValue(reader, klassAttributeC);
                    firstPageId = integerAttributeValue(reader, firstPageAttributeC, -1);
                    break;
                case ParseWithinField: // field attribute
                    field.name = attributeValue(reader, fieldNameAttributeC);
                    field.mandatory = booleanAttributeValue(reader, fieldMandatoryAttributeC);
                    break;
                case ParseWithinFile: { // file attribute
                        CustomWizardFile file;
                        file.source = attributeValue(reader, fileNameSourceAttributeC);
                        file.target = attributeValue(reader, fileNameTargetAttributeC);
                        if (file.target.isEmpty())
                            file.target = file.source;
                        if (file.source.isEmpty()) {
                            qWarning("Skipping empty file name in custom project.");
                        } else {
                            files.push_back(file);
                        }
                    }
                    break;
                default:
                    break;
                }
            } while (false);
            break;
        case QXmlStreamReader::EndElement:
            state = nextClosingState(state, reader.name());
            if (state == ParseError) {
                *errorMessage = msgError(reader, configFileFullPath,
                                         QString::fromLatin1("Unexpected end element %1").arg(reader.name().toString()));
                return false;
            }
            if (state == ParseWithinFields) { // Leaving a field element
                fields.push_back(field);
                field.clear();
            }
            break;
        default:
            break;
        }
    } while (token != QXmlStreamReader::EndDocument);
    return true;
}

bool CustomWizardParameters::parse(const QString &configFileFullPath,
                                   Core::BaseFileWizardParameters *bp,
                                   QString *errorMessage)
{
    QFile configFile(configFileFullPath);
    if (!configFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        *errorMessage = QString::fromLatin1("Cannot open %1: %2").arg(configFileFullPath, configFile.errorString());
        return false;
    }
    return parse(configFile, configFileFullPath, bp, errorMessage);
}

QDebug operator<<(QDebug d, const CustomWizardField &m)
{
    QDebug nsp = d.nospace();
    nsp << "Name: " << m.name;
    if (m.mandatory)
        nsp << '*';
    nsp << " Desc: " << m.description << " Control: " << m.controlAttributes;
    return d;
}

QDebug operator<<(QDebug d, const CustomWizardFile &f)
{
    d.nospace() << "source: " << f.source << " target: " << f.target;
    return d;
}

QDebug operator<<(QDebug d, const CustomWizardParameters &p)
{
    QDebug nsp = d.nospace();
    nsp << "Dir: " << p.directory << " klass:" << p.klass << " Files: " << p.files;
    if (!p.fields.isEmpty())
        nsp << " Fields: " << p.fields;
    nsp << "First page: " << p.firstPageId;
    return d;
}

} // namespace Internal
} // namespace ProjectExplorer
