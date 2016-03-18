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

#include "qtwizard.h"

namespace QmakeProjectManager {
namespace Internal {

struct QtProjectParameters;
class FilesPage;
struct LibraryParameters;

// Library wizard dialog.
class LibraryWizardDialog : public BaseQmakeProjectWizardDialog
{
    Q_OBJECT

public:
    LibraryWizardDialog(const Core::BaseFileWizardFactory *factory, const QString &templateName,
                        const QIcon &icon,
                        QWidget *parent,
                        const Core::WizardDialogParameters &parameters);

    void setSuffixes(const QString &header, const QString &source,  const QString &form= QString());
    void setLowerCaseFiles(bool);

    QtProjectParameters parameters() const;
    LibraryParameters libraryParameters() const;

    static QString pluginInterface(const QString &baseClass);

    int nextId() const override;

protected:
    void initializePage(int id) override;
    void cleanupPage(int id) override;

private:
    void slotCurrentIdChanged(int);

    QtProjectParameters::Type type() const;
    void setupFilesPage();
    bool isModulesPageSkipped() const;
    int skipModulesPageIfNeeded() const;

    FilesPage *m_filesPage;
    bool m_pluginBaseClassesInitialized;
    int m_filesPageId;
    int m_modulesPageId;
    int m_targetPageId;
};

} // namespace Internal
} // namespace QmakeProjectManager
