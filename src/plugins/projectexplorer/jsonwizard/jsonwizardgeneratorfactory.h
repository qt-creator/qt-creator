// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include "jsonwizard.h"

#include <utils/id.h>

#include <QList>
#include <QObject>

namespace Utils { class MacroExpander; }

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT JsonWizardGenerator
{
public:
    virtual ~JsonWizardGenerator() = default;

    virtual Core::GeneratedFiles fileList(Utils::MacroExpander *expander,
                                          const Utils::FilePath &wizardDir, const Utils::FilePath &projectDir,
                                          QString *errorMessage) = 0;
    virtual bool formatFile(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);
    virtual Utils::Result writeFile(const JsonWizard *wizard, Core::GeneratedFile *file);
    virtual bool postWrite(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);
    virtual bool polish(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);
    virtual bool allDone(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);

    virtual bool canKeepExistingFiles() const { return true; }

    enum OverwriteResult { OverwriteOk,  OverwriteError,  OverwriteCanceled };
    static OverwriteResult promptForOverwrite(JsonWizard::GeneratorFiles *files, QString *errorMessage);

    static bool formatFiles(const JsonWizard *wizard, QList<JsonWizard::GeneratorFile> *files, QString *errorMessage);
    static Utils::Result writeFiles(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files);
    static bool postWrite(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage);
    static bool polish(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage);
    static bool allDone(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage);
};

class PROJECTEXPLORER_EXPORT JsonWizardGeneratorFactory : public QObject
{
    Q_OBJECT

public:
    JsonWizardGeneratorFactory();
    ~JsonWizardGeneratorFactory() override;

    bool canCreate(Utils::Id typeId) const { return m_typeIds.contains(typeId); }
    QList<Utils::Id> supportedIds() const { return m_typeIds; }

    virtual JsonWizardGenerator *create(Utils::Id typeId, const QVariant &data,
                                        const QString &path, Utils::Id platform,
                                        const QVariantMap &variables) = 0;

    // Basic syntax check for the data taken from the wizard.json file:
    virtual bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) = 0;

protected:
    // This will add "PE.Wizard.Generator." in front of the suffixes and set those as supported typeIds
    void setTypeIdsSuffixes(const QStringList &suffixes);
    void setTypeIdsSuffix(const QString &suffix);

private:
    QList<Utils::Id> m_typeIds;
};

template <typename Generator>
class JsonWizardGeneratorTypedFactory : public JsonWizardGeneratorFactory
{
public:
    JsonWizardGeneratorTypedFactory(const QString &suffix) { setTypeIdsSuffix(suffix); }

    JsonWizardGenerator *create(Utils::Id typeId, const QVariant &data,
                                const QString &path, Utils::Id platform,
                                const QVariantMap &variables) final
    {
        Q_UNUSED(path)
        Q_UNUSED(platform)
        Q_UNUSED(variables)
        QTC_ASSERT(canCreate(typeId), return nullptr);

        auto gen = new Generator;
        QString errorMessage;
        gen->setup(data, &errorMessage);

        if (!errorMessage.isEmpty()) {
            qWarning() << "JsonWizardGeneratorTypedFactory for " << typeId << "setup error:"
                       << errorMessage;
            delete gen;
            return nullptr;
        }
        return gen;
    }

    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) final
    {
        QTC_ASSERT(canCreate(typeId), return false);
        QScopedPointer<Generator> gen(new Generator);
        return gen->setup(data, errorMessage);
    }
};

} // namespace ProjectExplorer
