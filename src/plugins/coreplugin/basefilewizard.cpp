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

#include "basefilewizard.h"

#include "basefilewizardfactory.h"
#include "ifilewizardextension.h"

#include <extensionsystem/pluginmanager.h>

#include <QMessageBox>

using namespace Utils;

namespace Core {

BaseFileWizard::BaseFileWizard(const BaseFileWizardFactory *factory,
                               const QVariantMap &extraValues,
                               QWidget *parent) :
    Wizard(parent),
    m_extraValues(extraValues),
    m_factory(factory)
{
    // Compile extension pages, purge out unused ones
    QList<IFileWizardExtension *> extensionList
            = ExtensionSystem::PluginManager::getObjects<IFileWizardExtension>();

    for (auto it = extensionList.begin(); it != extensionList.end(); ) {
        const QList<QWizardPage *> extensionPages = (*it)->extensionPages(factory);
        if (extensionPages.empty()) {
            it = extensionList.erase(it);
        } else {
            m_extensionPages += extensionPages;
            ++it;
        }
    }

    if (!m_extensionPages.empty())
        m_firstExtensionPage = m_extensionPages.front();
}

void BaseFileWizard::initializePage(int id)
{
    Wizard::initializePage(id);
    if (page(id) == m_firstExtensionPage) {
        generateFileList();

        QList<IFileWizardExtension *> extensionList
                = ExtensionSystem::PluginManager::getObjects<IFileWizardExtension>();
        foreach (IFileWizardExtension *ex, extensionList)
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
    switch (m_factory->promptOverwrite(&m_files, &errorMessage)) {
    case BaseFileWizardFactory::OverwriteCanceled:
        reject();
        return;
    case BaseFileWizardFactory::OverwriteError:
        QMessageBox::critical(0, tr("Existing files"), errorMessage);
        reject();
        return;
    case BaseFileWizardFactory::OverwriteOk:
        break;
    }

    QList<IFileWizardExtension *> extensionList
            = ExtensionSystem::PluginManager::getObjects<IFileWizardExtension>();
    foreach (IFileWizardExtension *ex, extensionList) {
        for (int i = 0; i < m_files.count(); i++) {
            ex->applyCodeStyle(&m_files[i]);
        }
    }

    // Write
    if (!m_factory->writeFiles(m_files, &errorMessage)) {
        QMessageBox::critical(parentWidget(), tr("File Generation Failure"), errorMessage);
        reject();
        return;
    }

    bool removeOpenProjectAttribute = false;
    // Run the extensions
    foreach (IFileWizardExtension *ex, extensionList) {
        bool remove;
        if (!ex->processFiles(m_files, &remove, &errorMessage)) {
            if (!errorMessage.isEmpty())
                QMessageBox::critical(parentWidget(), tr("File Generation Failure"), errorMessage);
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
            QMessageBox::critical(0, tr("File Generation Failure"), errorMessage);

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
        QMessageBox::critical(parentWidget(), tr("File Generation Failure"), errorMessage);
        reject();
    }
}

} // namespace Core
