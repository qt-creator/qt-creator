/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "../projectexplorer_export.h"

#include <coreplugin/generatedfile.h>

#include <utils/wizard.h>
#include <utils/macroexpander.h>

#include <QVariant>

namespace ProjectExplorer {

class JsonWizardGenerator;

// Documentation inside.
class PROJECTEXPLORER_EXPORT JsonWizard : public Utils::Wizard
{
    Q_OBJECT

public:
    class GeneratorFile {
    public:
        GeneratorFile() : generator(nullptr) { }
        GeneratorFile(const Core::GeneratedFile &f, JsonWizardGenerator *g) :
            file(f), generator(g)
        { }

        bool isValid() const { return generator; }

        Core::GeneratedFile file;
        JsonWizardGenerator *generator;
    };
    typedef QList<GeneratorFile> GeneratorFiles;
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

    QList<JsonWizardGenerator *> m_generators;

    GeneratorFiles m_files;
    Utils::MacroExpander m_expander;
};

} // namespace ProjectExplorer
