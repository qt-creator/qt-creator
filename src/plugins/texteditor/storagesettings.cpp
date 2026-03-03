// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "storagesettings.h"

#include "texteditorsettings.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>

#include <QRegularExpression>

using namespace Utils;

namespace TextEditor {

const char defaultTrailingWhitespaceBlacklist[] = "*.md, *.MD, Makefile";

StorageSettingsData::StorageSettingsData()
    : m_ignoreFileTypes(defaultTrailingWhitespaceBlacklist),
      m_cleanWhitespace(true),
      m_inEntireDocument(false),
      m_addFinalNewLine(false),
      m_cleanIndentation(true),
      m_skipTrailingWhitespace(true)
{
}

void StorageSettings::setData(const StorageSettingsData &data)
{
    cleanWhitespace.setValue(data.m_cleanWhitespace);
    inEntireDocument.setValue(data.m_inEntireDocument);
    addFinalNewLine.setValue(data.m_addFinalNewLine);
    cleanIndentation.setValue(data.m_cleanIndentation);
    skipTrailingWhitespace.setValue(data.m_skipTrailingWhitespace);
    ignoreFileTypes.setValue(data.m_ignoreFileTypes);
}

void StorageSettings::apply()
{
    AspectContainer::apply();
    emit TextEditorSettings::instance()->storageSettingsChanged(data());
}

StorageSettings::StorageSettings()
{
    setAutoApply(false);

    setSettingsGroup("textStorageSettings");

    cleanWhitespace.setSettingsKey("cleanWhitespace");
    cleanWhitespace.setDefaultValue(true);
    cleanWhitespace.setLabelText(Tr::tr("&Clean whitespace"));
    cleanWhitespace.setToolTip(Tr::tr("Removes trailing whitespace upon saving."));

    inEntireDocument.setSettingsKey("inEntireDocument");
    inEntireDocument.setDefaultValue(false);
    inEntireDocument.setEnabled(false);
    inEntireDocument.setLabelText(Tr::tr("In entire &document"));
    inEntireDocument.setToolTip(Tr::tr("Cleans whitespace in entire document instead of only for changed parts."));

    addFinalNewLine.setSettingsKey("addFinalNewLine");
    addFinalNewLine.setDefaultValue(false);
    addFinalNewLine.setLabelText(Tr::tr("&Ensure newline at end of file"));
    addFinalNewLine.setToolTip(Tr::tr("Always writes a newline character at the end of the file."));

    cleanIndentation.setSettingsKey("cleanIndentation");
    cleanIndentation.setDefaultValue(true);
    cleanIndentation.setEnabled(false);
    cleanIndentation.setLabelText(Tr::tr("Clean indentation"));
    cleanIndentation.setToolTip(Tr::tr("Corrects leading whitespace according to tab settings."));

    skipTrailingWhitespace.setSettingsKey("skipTrailingWhitespace");
    skipTrailingWhitespace.setDefaultValue(true);
    skipTrailingWhitespace.setEnabled(false);
    skipTrailingWhitespace.setLabelText(Tr::tr("Skip clean whitespace for file types:"));
    skipTrailingWhitespace.setToolTip(Tr::tr("For the file patterns listed, do not trim trailing whitespace."));

    ignoreFileTypes.setSettingsKey("ignoreFileTypes");
    ignoreFileTypes.setDefaultValue("*.md, *.MD, Makefile");
    ignoreFileTypes.setDisplayStyle(StringAspect::LineEditDisplay);
    ignoreFileTypes.setEnabled(false);
    ignoreFileTypes.setToolTip(Tr::tr("List of wildcard-aware file patterns, separated by commas or semicolons."));

    readSettings();

    cleanIndentation.setEnabler(&cleanWhitespace);
    inEntireDocument.setEnabler(&cleanWhitespace);
    skipTrailingWhitespace.setEnabler(&cleanWhitespace);

    ignoreFileTypes.setEnabler(&cleanWhitespace);

    auto update = [this] {
        ignoreFileTypes.setEnabled(cleanWhitespace.volatileValue()
                                && skipTrailingWhitespace.volatileValue());
    };

    update();

    connect(&cleanWhitespace, &BoolAspect::volatileValueChanged, this, update);
    connect(&cleanWhitespace, &BoolAspect::changed, this, update);
    connect(&cleanWhitespace, &BaseAspect::enabledChanged, this, update);

    connect(&skipTrailingWhitespace, &BoolAspect::volatileValueChanged, this, update);
    connect(&skipTrailingWhitespace, &BoolAspect::changed, this, update);
    connect(&skipTrailingWhitespace, &BaseAspect::enabledChanged, this, update);
}

StorageSettingsData StorageSettings::data() const
{
    StorageSettingsData d;
    d.m_cleanWhitespace = cleanWhitespace();
    d.m_inEntireDocument = inEntireDocument();
    d.m_addFinalNewLine = addFinalNewLine();
    d.m_cleanIndentation = cleanIndentation();
    d.m_skipTrailingWhitespace = skipTrailingWhitespace();
    d.m_ignoreFileTypes = ignoreFileTypes();
    return d;
}

bool StorageSettingsData::removeTrailingWhitespace(const QString &fileName) const
{
    // if the user has elected not to trim trailing whitespace altogether, then
    // early out here
    if (!m_skipTrailingWhitespace)
        return true;

    const QString ignoreFileTypesRegExp(R"(\s*((?>\*\.)?[\w\d\.\*]+)[,;]?\s*)");

    // use the ignore-files regex to extract the specified file patterns
    static const QRegularExpression re(ignoreFileTypesRegExp);
    QRegularExpressionMatchIterator iter = re.globalMatch(m_ignoreFileTypes);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString pattern = match.captured(1);

        QRegularExpression patternRegExp(QRegularExpression::wildcardToRegularExpression(pattern));
        QRegularExpressionMatch patternMatch = patternRegExp.match(fileName);
        if (patternMatch.hasMatch()) {
            // if the filename has a pattern we want to ignore, then we need to return
            // false ("don't remove trailing whitespace")
            return false;
        }
    }

    // the supplied pattern does not match, so we want to remove trailing whitespace
    return true;
}

StorageSettings &globalStorageSettings()
{
    static StorageSettings theGlobalStorageSettings;
    return theGlobalStorageSettings;
}

void setupStorageSettings()
{
    globalStorageSettings().readSettings();
}

} // namespace TextEditor
