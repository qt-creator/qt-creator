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

#include <utils/wizard.h>
#include <utils/pathchooser.h>

QT_BEGIN_NAMESPACE
class QRadioButton;
class QLabel;
QT_END_NAMESPACE

namespace QmakeProjectManager {
namespace Internal {

class LibraryDetailsWidget;
class LibraryDetailsController;
class LibraryTypePage;
class DetailsPage;
class SummaryPage;

namespace Ui { class LibraryDetailsWidget; }

class AddLibraryWizard : public Utils::Wizard
{
    Q_OBJECT
public:
    enum LibraryKind {
        InternalLibrary,
        ExternalLibrary,
        SystemLibrary,
        PackageLibrary
        };

    enum LinkageType {
        DynamicLinkage,
        StaticLinkage,
        NoLinkage
        };

    enum MacLibraryType {
        FrameworkType,
        LibraryType,
        NoLibraryType
        };

    enum Platform {
        LinuxPlatform   = 0x01,
        MacPlatform     = 0x02,
        WindowsMinGWPlatform = 0x04,
        WindowsMSVCPlatform = 0x08
        };

    Q_DECLARE_FLAGS(Platforms, Platform)

    explicit AddLibraryWizard(const QString &fileName, QWidget *parent = 0);
    ~AddLibraryWizard();

    LibraryKind libraryKind() const;
    QString proFile() const;
    QString snippet() const;

signals:

private:
    LibraryTypePage *m_libraryTypePage = nullptr;
    DetailsPage *m_detailsPage = nullptr;
    SummaryPage *m_summaryPage = nullptr;
    QString m_proFile;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AddLibraryWizard::Platforms)

class LibraryTypePage : public QWizardPage
{
    Q_OBJECT
public:
    LibraryTypePage(AddLibraryWizard *parent);
    AddLibraryWizard::LibraryKind libraryKind() const;

private:
    QRadioButton *m_internalRadio = nullptr;
    QRadioButton *m_externalRadio = nullptr;
    QRadioButton *m_systemRadio = nullptr;
    QRadioButton *m_packageRadio = nullptr;
};

class DetailsPage : public QWizardPage
{
    Q_OBJECT
public:
    DetailsPage(AddLibraryWizard *parent);
    virtual void initializePage();
    virtual bool isComplete() const;
    QString snippet() const;

private:
    AddLibraryWizard *m_libraryWizard;
    Ui::LibraryDetailsWidget *m_libraryDetailsWidget = nullptr;
    LibraryDetailsController *m_libraryDetailsController = nullptr;
};

class SummaryPage : public QWizardPage
{
    Q_OBJECT
public:
    SummaryPage(AddLibraryWizard *parent);
    virtual void initializePage();
    QString snippet() const;
private:
    AddLibraryWizard *m_libraryWizard = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QLabel *m_snippetLabel = nullptr;
    QString m_snippet;
};

} // namespace Internal
} // namespace QmakeProjectManager
