// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basefilewizard.h"

#include "basefilewizardfactory.h"
#include "coreplugintr.h"
#include "icore.h"
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

BaseFileWizard::BaseFileWizard(const BaseFileWizardFactory *factory, const QVariantMap &extraValues)
    : m_extraValues(extraValues), m_factory(factory)
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

    // Compile result list and prompt for overwrite
    Result<BaseFileWizardFactory::OverwriteResult> res =
        BaseFileWizardFactory::promptOverwrite(&m_files);

    if (!res) {
        QMessageBox::critical(ICore::dialogParent(), Tr::tr("Existing files"), res.error());
        reject();
        return;
    }

    if (*res == BaseFileWizardFactory::OverwriteCanceled) {
        reject();
        return;
    }

    for (IFileWizardExtension *ex : std::as_const(g_fileWizardExtensions)) {
        for (int i = 0; i < m_files.count(); i++) {
            ex->applyCodeStyle(&m_files[i]);
        }
    }

    // Write
    if (const Result<> res = m_factory->writeFiles(m_files); !res) {
        QMessageBox::critical(parentWidget(), Tr::tr("File Generation Failure"), res.error());
        reject();
        return;
    }

    bool removeOpenProjectAttribute = false;
    // Run the extensions
    for (IFileWizardExtension *ex : std::as_const(g_fileWizardExtensions)) {
        bool remove;
        if (const Result<> res = ex->processFiles(m_files, &remove); !res) {
            if (!res.error().isEmpty())
                QMessageBox::critical(parentWidget(), Tr::tr("File Generation Failure"), res.error());
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
    if (const Result<> res = m_factory->postGenerateFiles(this, m_files); !res) {
        if (!res.error().isEmpty())
            QMessageBox::critical(nullptr, Tr::tr("File Generation Failure"), res.error());
    }

    Wizard::accept();
}

void BaseFileWizard::reject()
{
    m_files.clear();
    Wizard::reject();
}

void BaseFileWizard::generateFileList()
{
    if (const Result<GeneratedFiles> res = m_factory->generateFiles(this)) {
        m_files = res.value();
    } else {
        QMessageBox::critical(parentWidget(), Tr::tr("File Generation Failure"), res.error());
        reject();
    }
}

} // namespace Core
