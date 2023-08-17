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
    virtual bool writeFile(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);
    virtual bool postWrite(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);
    virtual bool polish(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);
    virtual bool allDone(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);

    virtual bool canKeepExistingFiles() const { return true; }

    enum OverwriteResult { OverwriteOk,  OverwriteError,  OverwriteCanceled };
    static OverwriteResult promptForOverwrite(JsonWizard::GeneratorFiles *files, QString *errorMessage);

    static bool formatFiles(const JsonWizard *wizard, QList<JsonWizard::GeneratorFile> *files, QString *errorMessage);
    static bool writeFiles(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage);
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

namespace Internal {

class FileGeneratorFactory : public JsonWizardGeneratorFactory
{
    Q_OBJECT

public:
    FileGeneratorFactory();

    JsonWizardGenerator *create(Utils::Id typeId, const QVariant &data,
                                const QString &path, Utils::Id platform,
                                const QVariantMap &variables) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class ScannerGeneratorFactory : public JsonWizardGeneratorFactory
{
    Q_OBJECT

public:
    ScannerGeneratorFactory();

    JsonWizardGenerator *create(Utils::Id typeId, const QVariant &data,
                                const QString &path, Utils::Id platform,
                                const QVariantMap &variables) override;
    bool validateData(Utils::Id typeId, const QVariant &data, QString *errorMessage) override;
};

} // namespace Internal
} // namespace ProjectExplorer
