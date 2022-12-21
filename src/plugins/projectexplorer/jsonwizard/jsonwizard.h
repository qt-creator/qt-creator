// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"
#include <projectexplorer/projectnodes.h>

#include <coreplugin/generatedfile.h>
#include <coreplugin/jsexpander.h>

#include <utils/wizard.h>
#include <utils/macroexpander.h>

#include <QDebug>
#include <QVariant>

namespace ProjectExplorer {

class JsonWizard;
class JsonWizardGenerator;

namespace Internal {

class JsonWizardJsExtension : public QObject
{
    Q_OBJECT
public:
    JsonWizardJsExtension(JsonWizard *wizard);

    Q_INVOKABLE QVariant value(const QString &name) const;

private:
    JsonWizard *m_wizard;
};

} // namespace Internal

// Documentation inside.
class PROJECTEXPLORER_EXPORT JsonWizard : public Utils::Wizard
{
    Q_OBJECT

public:
    class GeneratorFile {
    public:
        GeneratorFile() = default;
        GeneratorFile(const Core::GeneratedFile &f, JsonWizardGenerator *g) :
            file(f), generator(g)
        { }

        bool isValid() const { return generator; }

        Core::GeneratedFile file;
        JsonWizardGenerator *generator = nullptr;
    };
    using GeneratorFiles = QList<GeneratorFile>;
    Q_PROPERTY(GeneratorFiles generateFileList READ generateFileList)

    explicit JsonWizard(QWidget *parent = nullptr);
    ~JsonWizard() override;

    void addGenerator(JsonWizardGenerator *gen);

    Utils::MacroExpander *expander();

    GeneratorFiles generateFileList();
    void commitToFileList(const GeneratorFiles &list);

    QString stringValue(const QString &n) const;

    QVariant value(const QString &n) const;
    void setValue(const QString &key, const QVariant &value);

    class OptionDefinition {
    public:
        QString key() const { return m_key; }
        QString value(Utils::MacroExpander &expander) const;
        bool condition(Utils::MacroExpander &expander) const;

    private:
        QString m_key;
        QString m_value;
        QVariant m_condition;
        QVariant m_evaluate;

        friend QDebug &operator<<(QDebug &debug, const OptionDefinition &option);

        friend class JsonWizard;
    };
    static QList<OptionDefinition> parseOptions(const QVariant &v, QString *errorMessage);

    static bool boolFromVariant(const QVariant &v, Utils::MacroExpander *expander);
    static QString stringListToArrayString(const QStringList &list,
                                           const Utils::MacroExpander *expander);

    void removeAttributeFromAllFiles(Core::GeneratedFile::Attribute a);

    QHash<QString, QVariant> variables() const override;

signals:
    void preGenerateFiles(); // emitted before files are generated (can happen several times!)
    void postGenerateFiles(const JsonWizard::GeneratorFiles &files); // emitted after commitToFileList was called.
    void prePromptForOverwrite(const JsonWizard::GeneratorFiles &files); // emitted before overwriting checks are done.
    void preFormatFiles(const JsonWizard::GeneratorFiles &files); // emitted before files are formatted.
    void preWriteFiles(const JsonWizard::GeneratorFiles &files); // emitted before files are written to disk.
    void postProcessFiles(const JsonWizard::GeneratorFiles &files); // emitted before files are post-processed.
    void filesReady(const JsonWizard::GeneratorFiles &files); // emitted just after files are in final state on disk.
    void filesPolished(const JsonWizard::GeneratorFiles &files); // emitted just after additional files (e.g. settings) not directly related to the project were created.
    void allDone(const JsonWizard::GeneratorFiles &files); // emitted just after the wizard is done with the files. They are ready to be opened.
public slots:
    void accept() override;
    void reject() override;

private:
    void handleNewPages(int pageId);
    void handleError(const QString &message);

    QString stringify(const QVariant &v) const override;
    QString evaluate(const QVariant &v) const override ;
    void openFiles(const GeneratorFiles &files);
    void openProjectForNode(ProjectExplorer::Node *node);

    QList<JsonWizardGenerator *> m_generators;

    GeneratorFiles m_files;
    Utils::MacroExpander m_expander;
    Core::JsExpander m_jsExpander;
};

inline QDebug &operator<<(QDebug &debug, const JsonWizard::GeneratorFile &file)
{
    debug << "GeneratorFile{file: " << file.file << "}";

    return debug;
}

inline QDebug &operator<<(QDebug &debug, const JsonWizard::OptionDefinition &option)
{
    debug << "Option{"
      << "key:" << option.m_key
      << "; value:" << option.m_value
      << "; evaluate:" << option.m_evaluate
      << "; condition:" << option.m_condition
      << "}";
    return debug;
}

} // namespace ProjectExplorer
