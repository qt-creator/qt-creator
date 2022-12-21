// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/wizard.h>
#include <utils/pathchooser.h>

#include <QCoreApplication>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QGroupBox;
class QRadioButton;
class QLabel;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace QmakeProjectManager::Internal {

class LibraryDetailsController;
class LibraryTypePage;
class DetailsPage;
class SummaryPage;

class LibraryDetailsWidget
{
public:
    explicit LibraryDetailsWidget(QWidget *parent);

public:
    QGroupBox *platformGroupBox;
    QGroupBox *linkageGroupBox;
    QGroupBox *macGroupBox;
    QGroupBox *winGroupBox;

    Utils::PathChooser *includePathChooser;
    QLineEdit *packageLineEdit;
    Utils::PathChooser *libraryPathChooser;
    QComboBox *libraryComboBox;
    QComboBox *libraryTypeComboBox;
    QLabel *libraryLabel;
    QLabel *libraryFileLabel;
    QLabel *libraryTypeLabel;
    QLabel *packageLabel;
    QLabel *includeLabel;
    QCheckBox *linCheckBox;
    QCheckBox *macCheckBox;
    QCheckBox *winCheckBox;
    QRadioButton *dynamicRadio;
    QRadioButton *staticRadio;
    QRadioButton *libraryRadio;
    QRadioButton *frameworkRadio;
    QCheckBox *useSubfoldersCheckBox;
    QCheckBox *addSuffixCheckBox;
    QCheckBox *removeSuffixCheckBox;
};

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

    explicit AddLibraryWizard(const Utils::FilePath &proFile, QWidget *parent = nullptr);
    ~AddLibraryWizard() override;

    LibraryKind libraryKind() const;
    Utils::FilePath proFile() const;
    QString snippet() const;

private:
    LibraryTypePage *m_libraryTypePage = nullptr;
    DetailsPage *m_detailsPage = nullptr;
    SummaryPage *m_summaryPage = nullptr;
    Utils::FilePath m_proFile;
};

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
    void initializePage() override;
    bool isComplete() const override;
    QString snippet() const;

private:
    AddLibraryWizard *m_libraryWizard;
    LibraryDetailsWidget *m_libraryDetailsWidget = nullptr;
    LibraryDetailsController *m_libraryDetailsController = nullptr;
};

class SummaryPage : public QWizardPage
{
    Q_OBJECT
public:
    SummaryPage(AddLibraryWizard *parent);
    void initializePage() override;
    QString snippet() const;
private:
    AddLibraryWizard *m_libraryWizard = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QLabel *m_snippetLabel = nullptr;
    QString m_snippet;
};

} // QmakeProjectManager::Internal

Q_DECLARE_OPERATORS_FOR_FLAGS(QmakeProjectManager::Internal::AddLibraryWizard::Platforms)
