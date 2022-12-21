// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/iwizardfactory.h>

#include <utils/filepath.h>

#include <QStringList>
#include <QMap>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QIODevice;
class QDebug;
class QJSEngine;
QT_END_NAMESPACE

namespace Utils { class TemporaryFile; }

namespace ProjectExplorer {
namespace Internal {

class CustomWizardField {
public:
    // Parameters of the widget control are stored as map
    using ControlAttributeMap = QMap<QString, QString>;
    CustomWizardField();
    void clear();

    // Attribute map keys for combo entries
    static QString comboEntryValueKey(int i);
    static QString comboEntryTextKey(int i);

    QString description;
    QString name;
    ControlAttributeMap controlAttributes;
    bool mandatory;
};

class CustomWizardFile {
public:
    CustomWizardFile();

    QString source;
    QString target;
    bool openEditor;
    bool openProject;
    bool binary;
};

// Documentation inside.
class CustomWizardValidationRule {
public:
    // Validate a set of rules and return false + message on the first failing one.
    static bool validateRules(const QList<CustomWizardValidationRule> &rules,
                              const QMap<QString, QString> &replacementMap,
                              QString *errorMessage);
    bool validate(QJSEngine &, const QMap<QString, QString> &replacementMap) const;
    QString condition;
    QString message;
};

// Documentation inside.
class GeneratorScriptArgument {
public:
    enum Flags {
        // Omit this arguments if all field placeholders expanded to empty strings.
        OmitEmpty = 0x1,
        // Do use the actual field value, but write it to a temporary
        // text file and inserts its file name (suitable for multiline texts).
        WriteFile = 0x2 };

    explicit GeneratorScriptArgument(const QString &value = QString());

    QString value;
    unsigned flags;
};

class CustomWizardParameters
{
public:
    enum ParseResult { ParseOk, ParseDisabled, ParseFailed };

    CustomWizardParameters() = default;
    void clear();
    ParseResult parse(QIODevice &device, const QString &configFileFullPath,
                      QString *errorMessage);
    ParseResult parse(const QString &configFileFullPath, QString *errorMessage);

    Utils::Id id;
    QString directory;
    QString klass;
    QList<CustomWizardFile> files;
    QStringList filesGeneratorScript; // Complete binary, such as 'cmd /c myscript.pl'.
    QString filesGeneratorScriptWorkingDirectory;
    QList<GeneratorScriptArgument> filesGeneratorScriptArguments;

    QString fieldPageTitle;
    QList<CustomWizardField> fields;
    QList<CustomWizardValidationRule> rules;
    int firstPageId = -1;

    // Wizard Factory data:
    Core::IWizardFactory::WizardKind kind = Core::IWizardFactory::FileWizard;
    QIcon icon;
    QString description;
    QString displayName;
    QString category;
    QString displayCategory;
    QSet<Utils::Id> requiredFeatures;
    Core::IWizardFactory::WizardFlags flags;
};

// Documentation inside.
class CustomWizardContext {
public:
    using FieldReplacementMap = QMap<QString, QString>;
    using TemporaryFilePtr = QSharedPointer<Utils::TemporaryFile>;
    using TemporaryFilePtrList = QList<TemporaryFilePtr>;

    void reset();

    static bool replaceFields(const FieldReplacementMap &fm, QString *s);
    static bool replaceFields(const FieldReplacementMap &fm, QString *s,
                              TemporaryFilePtrList *files);

    static QString processFile(const FieldReplacementMap &fm, QString in);

    FieldReplacementMap baseReplacements;
    FieldReplacementMap replacements;

    Utils::FilePath path;
    // Where files should be created, that is, 'path' for simple wizards
    // or "path + project" for project wizards.
    Utils::FilePath targetPath;
};

extern const char customWizardFileOpenEditorAttributeC[];
extern const char customWizardFileOpenProjectAttributeC[];

} // namespace Internal
} // namespace ProjectExplorer
