// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basefilewizard.h"

#include "basefilewizardfactory.h"
#include "coreplugintr.h"
#include "ifilewizardextension.h"

#include <QMessageBox>

using namespace Utils;

namespace Core {

static QList<IFileWizardExtension *> g_fileWizardExtensions;

IFileWizardExtension::IFileWizardExtension()
{
    g_fileWizardExtensions.append(this);
}

IFileWizardExtension::~IFileWizardExtension()
{
    g_fileWizardExtensions.removeOne(this);
}

BaseFileWizard::BaseFileWizard(const BaseFileWizardFactory *factory,
                               const QVariantMap &extraValues,
                               QWidget *parent) :
    Wizard(parent),
    m_extraValues(extraValues),
    m_factory(factory)
{
    for (IFileWizardExtension *extension : std::as_const(g_fileWizardExtensions))
        m_extensionPages += extension->extensionPages(factory);

    if (!m_extensionPages.empty())
        m_firstExtensionPage = m_extensionPages.front();
}

void BaseFileWizard::initializePage(int id)
{
    Wizard::initializePage(id);
    if (page(id) == m_firstExtensionPage) {
        generateFileList();

        for (IFileWizardExtension *ex : std::as_const(g_fileWizardExtensions))
            ex->firstExtensionPageShown(m_files, m_extraValues);
    }
}

QList<QWizardPage *> BaseFileWizard::extensionPages()
{
    return m_extensionPages;
}

void BaseFileWizard::accept()
{
    if (m_files.isEmpty())
        generateFileList();

    QString errorMessage;

    // Compile result list and prompt for overwrite
    switch (BaseFileWizardFactory::promptOverwrite(&m_files, &errorMessage)) {
    case BaseFileWizardFactory::OverwriteCanceled:
        reject();
        return;
    case BaseFileWizardFactory::OverwriteError:
        QMessageBox::critical(nullptr, Tr::tr("Existing files"), errorMessage);
        reject();
        return;
    case BaseFileWizardFactory::OverwriteOk:
        break;
    }

    for (IFileWizardExtension *ex : std::as_const(g_fileWizardExtensions)) {
        for (int i = 0; i < m_files.count(); i++) {
            ex->applyCodeStyle(&m_files[i]);
        }
    }

    // Write
    if (!m_factory->writeFiles(m_files, &errorMessage)) {
        QMessageBox::critical(parentWidget(), Tr::tr("File Generation Failure"), errorMessage);
        reject();
        return;
    }

    bool removeOpenProjectAttribute = false;
    // Run the extensions
    for (IFileWizardExtension *ex : std::as_const(g_fileWizardExtensions)) {
        bool remove;
        if (!ex->processFiles(m_files, &remove, &errorMessage)) {
            if (!errorMessage.isEmpty())
                QMessageBox::critical(parentWidget(), Tr::tr("File Generation Failure"), errorMessage);
            reject();
            return;
        }
        removeOpenProjectAttribute |= remove;
    }

    if (removeOpenProjectAttribute) {
        for (int i = 0; i < m_files.count(); i++) {
            if (m_files[i].attributes() & GeneratedFile::OpenProjectAttribute)
                m_files[i].setAttributes(GeneratedFile::OpenEditorAttribute);
        }
    }

    // Post generation handler
    if (!m_factory->postGenerateFiles(this, m_files, &errorMessage))
        if (!errorMessage.isEmpty())
            QMessageBox::critical(nullptr, Tr::tr("File Generation Failure"), errorMessage);

    Wizard::accept();
}

void BaseFileWizard::reject()
{
    m_files.clear();
    Wizard::reject();
}

void BaseFileWizard::generateFileList()
{
    QString errorMessage;
    m_files = m_factory->generateFiles(this, &errorMessage);
    if (m_files.empty()) {
        QMessageBox::critical(parentWidget(), Tr::tr("File Generation Failure"), errorMessage);
        reject();
    }
}

} // namespace Core
