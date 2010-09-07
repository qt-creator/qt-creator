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

#ifndef CUSTOMPROJECTWIZARDPARAMETERS_H
#define CUSTOMPROJECTWIZARDPARAMETERS_H

#include <coreplugin/basefilewizard.h>

#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QIODevice;
class QDebug;
class QTemporaryFile;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

struct CustomWizardField {
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

struct CustomWizardFile {
    CustomWizardFile();

    QString source;
    QString target;
    bool openEditor;
    bool openProject;
};

// Argument to the generator script containing placeholders to
// be replaced by field values or file names
// as in '--class-name=%ClassName%' or '--description=%Description%'.
struct GeneratorScriptArgument {
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

struct CustomWizardParameters
{
public:
    enum ParseResult { ParseOk, ParseDisabled, ParseFailed };

    CustomWizardParameters();
    void clear();
    ParseResult parse(QIODevice &device, const QString &configFileFullPath,
                      Core::BaseFileWizardParameters *bp, QString *errorMessage);
    ParseResult parse(const QString &configFileFullPath,
                      Core::BaseFileWizardParameters *bp, QString *errorMessage);
    QString toString() const;

    QString directory;
    QString klass;
    QList<CustomWizardFile> files;
    QStringList filesGeneratorScript; // Complete binary, such as 'cmd /c myscript.pl'.
    QString filesGeneratorScriptWorkingDirectory;
    QList<GeneratorScriptArgument> filesGeneratorScriptArguments;

    QString fieldPageTitle;
    QList<CustomWizardField> fields;
    int firstPageId;
};

// Context used for one wizard run, shared between CustomWizard
// and the CustomWizardPage as it is used for the QLineEdit-type fields'
// default texts as well. Contains basic replacement fields
// like  '%CppSourceSuffix%', '%CppHeaderSuffix%' (settings-dependent)
// reset() should be called before each wizard run to refresh them.
// CustomProjectWizard additionally inserts '%ProjectName%' from
// the intro page to have it available for default texts.

struct CustomWizardContext {
    typedef QMap<QString, QString> FieldReplacementMap;
    typedef QSharedPointer<QTemporaryFile> TemporaryFilePtr;
    typedef QList<TemporaryFilePtr> TemporaryFilePtrList;

    void reset();

    // Replace field values delimited by '%' with special modifiers:
    // %Field% -> simple replacement
    // %Field:l% -> lower case replacement, 'u' upper case,
    // 'c' capitalize first letter. Return value indicates whether non-empty
    // replacements where encountered
    static bool replaceFields(const FieldReplacementMap &fm, QString *s);

    // Special replaceFields() overload used for the arguments of a generator
    // script: Write the expanded field values out to temporary files and
    // inserts file names instead of the expanded fields in string 's'.
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

#endif // CUSTOMPROJECTWIZARDPARAMETERS_H
