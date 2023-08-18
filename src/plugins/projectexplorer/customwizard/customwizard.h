// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include <coreplugin/basefilewizardfactory.h>

#include <QSharedPointer>
#include <QList>
#include <QMap>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace Utils { class Wizard; }

namespace ProjectExplorer {
class CustomWizard;
class BaseProjectWizardDialog;

namespace Internal {
class CustomWizardPrivate;
class CustomWizardContext;
class CustomWizardParameters;
}

// Documentation inside.
class PROJECTEXPLORER_EXPORT ICustomWizardMetaFactory
{
public:
    ICustomWizardMetaFactory(const QString &klass, Core::IWizardFactory::WizardKind kind);
    virtual ~ICustomWizardMetaFactory();

    virtual CustomWizard *create() const = 0;
    QString klass() const { return m_klass; }
    int kind() const { return m_kind; }

private:
    QString m_klass;
    Core::IWizardFactory::WizardKind m_kind;
};

// Convenience template to create wizard factory classes.
template <class Wizard> class CustomWizardMetaFactory : public ICustomWizardMetaFactory
{
public:
    CustomWizardMetaFactory(const QString &klass, Core::IWizardFactory::WizardKind kind) : ICustomWizardMetaFactory(klass, kind) { }
    CustomWizardMetaFactory(Core::IWizardFactory::WizardKind kind) : ICustomWizardMetaFactory(QString(), kind) { }
    CustomWizard *create() const override { return new Wizard; }
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT CustomWizard : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    using FieldReplacementMap = QMap<QString, QString>;

    CustomWizard();
    ~CustomWizard() override;

    // Can be reimplemented to create custom wizards. initWizardDialog() needs to be
    // called.
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const override;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;

    // Create all wizards. As other plugins might register factories for derived
    // classes, call it in extensionsInitialized().
    static void createWizards();

    static void setVerbose(int);
    static int verbose();

protected:
    using CustomWizardParametersPtr = QSharedPointer<Internal::CustomWizardParameters>;
    using CustomWizardContextPtr = QSharedPointer<Internal::CustomWizardContext>;

    // generate files in path
    Core::GeneratedFiles generateWizardFiles(QString *errorMessage) const;
    // Create replacement map as static base fields + QWizard fields
    FieldReplacementMap replacementMap(const QWizard *w) const;
    bool writeFiles(const Core::GeneratedFiles &files, QString *errorMessage) const override;

    CustomWizardParametersPtr parameters() const;
    CustomWizardContextPtr context() const;

    static CustomWizard *createWizard(const CustomWizardParametersPtr &p);

private:
    void setParameters(const CustomWizardParametersPtr &p);

    Internal::CustomWizardPrivate *d;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT CustomProjectWizard : public CustomWizard
{
    Q_OBJECT

public:
    CustomProjectWizard();

    static bool postGenerateOpen(const Core::GeneratedFiles &l, QString *errorMessage = nullptr);

signals:
    void projectLocationChanged(const Utils::FilePath &path);

protected:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const override;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;

    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage) const override;

    void initProjectWizardDialog(BaseProjectWizardDialog *w, const Utils::FilePath &defaultPath,
                                 const QList<QWizardPage *> &extensionPages) const;

private:
    void handleProjectParametersChanged(const QString &project, const Utils::FilePath &path);
};

} // namespace ProjectExplorer
