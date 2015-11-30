/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CUSTOMWIZARDPARAMETERS_H
#define CUSTOMWIZARDPARAMETERS_H

#include <coreplugin/iwizardfactory.h>

#include <QStringList>
#include <QMap>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QIODevice;
class QDebug;
class QTemporaryFile;
class QJSEngine;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

class CustomWizardField {
public:
    // Parameters of the widget control are stored as map
    typedef QMap<QString, QString> ControlAttributeMap;
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

    CustomWizardParameters();
    void clear();
    ParseResult parse(QIODevice &device, const QString &configFileFullPath,
                      QString *errorMessage);
    ParseResult parse(const QString &configFileFullPath, QString *errorMessage);

    Core::Id id;
    QString directory;
    QString klass;
    QList<CustomWizardFile> files;
    QStringList filesGeneratorScript; // Complete binary, such as 'cmd /c myscript.pl'.
    QString filesGeneratorScriptWorkingDirectory;
    QList<GeneratorScriptArgument> filesGeneratorScriptArguments;

    QString fieldPageTitle;
    QList<CustomWizardField> fields;
    QList<CustomWizardValidationRule> rules;
    int firstPageId;

    // Wizard Factory data:
    Core::IWizardFactory::WizardKind kind;
    QIcon icon;
    QString description;
    QString displayName;
    QString category;
    QString displayCategory;
    Core::FeatureSet requiredFeatures;
    Core::IWizardFactory::WizardFlags flags;
};

// Documentation inside.
class CustomWizardContext {
public:
    typedef QMap<QString, QString> FieldReplacementMap;
    typedef QSharedPointer<QTemporaryFile> TemporaryFilePtr;
    typedef QList<TemporaryFilePtr> TemporaryFilePtrList;

    void reset();

    static bool replaceFields(const FieldReplacementMap &fm, QString *s);
    static bool replaceFields(const FieldReplacementMap &fm, QString *s,
                              TemporaryFilePtrList *files);

    static QString processFile(const FieldReplacementMap &fm, QString in);

    FieldReplacementMap baseReplacements;
    FieldReplacementMap replacements;

    QString path;
    // Where files should be created, that is, 'path' for simple wizards
    // or "path + project" for project wizards.
    QString targetPath;
};

extern const char customWizardFileOpenEditorAttributeC[];
extern const char customWizardFileOpenProjectAttributeC[];

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMWIZARDPARAMETERS_H
