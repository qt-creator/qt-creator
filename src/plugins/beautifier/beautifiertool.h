// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <texteditor/command.h>

#include <utils/aspects.h>

#include <QHash>
#include <QMap>
#include <QObject>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QVersionNumber>

#include <memory>

namespace Core {
class IDocument;
class IEditor;
}

namespace Beautifier::Internal  {

class BeautifierTool : public QObject
{
public:
    BeautifierTool();

    static const QList<BeautifierTool *> &allTools();

    virtual QString id() const = 0;
    virtual void updateActions(Core::IEditor *editor) = 0;

    /**
     * Returns the tool's command to format an entire file.
     *
     * @note    The received command may be invalid.
     */
    virtual TextEditor::Command textCommand() const = 0;

    virtual bool isApplicable(const Core::IDocument *document) const = 0;

    static QString msgCannotGetConfigurationFile(const QString &command);
    static QString msgFormatCurrentFile();
    static QString msgFormatSelectedText();
    static QString msgFormatAtCursor();
    static QString msgFormatLines();
    static QString msgDisableFormattingSelectedText();
    static QString msgCommandPromptDialogTitle(const QString &command);
    static void showError(const QString &error);
};

class AbstractSettings : public Utils::AspectContainer
{
public:
    explicit AbstractSettings(const QString &name, const QString &ending);
    ~AbstractSettings() override;

    void read();
    void save();

    virtual void createDocumentationFile() const;
    virtual QStringList completerWords();

    QStringList styles() const;
    QString style(const QString &key) const;
    bool styleExists(const QString &key) const;
    bool styleIsReadOnly(const QString &key);
    void setStyle(const QString &key, const QString &value);
    void removeStyle(const QString &key);
    void replaceStyle(const QString &oldKey, const QString &newKey, const QString &value);
    virtual Utils::FilePath styleFileName(const QString &key) const;

    Utils::FilePathAspect command{this};
    Utils::StringAspect supportedMimeTypes{this};

    Utils::FilePath documentationFilePath;

    QVersionNumber version() const;

    bool isApplicable(const Core::IDocument *document) const;

    QStringList options();
    QString documentation(const QString &option) const;

protected:
    void setVersionRegExp(const QRegularExpression &versionRegExp);

    QMap<QString, QString> m_styles;
    QString m_ending;
    Utils::FilePath m_styleDir;

    void readDocumentation();
    virtual void readStyles();

private:
    QStringList m_stylesToRemove;
    QSet<QString> m_changedStyles;
    QHash<QString, int> m_options;
    QStringList m_docu;
    QStringList m_supportedMimeTypes;
    mutable QVersionNumber m_version;
    QRegularExpression m_versionRegExp;
};

} // Beautifier::Internal
