// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "addlibrarywizard.h"

#include <utils/guard.h>

namespace QmakeProjectManager {
class QmakeProFile;

namespace Internal {

class LibraryDetailsWidget;

class LibraryDetailsController : public QObject
{
    Q_OBJECT
public:
    explicit LibraryDetailsController(LibraryDetailsWidget *libraryDetails,
                                      const Utils::FilePath &proFile,
                                      QObject *parent = nullptr);
    virtual bool isComplete() const = 0;
    virtual QString snippet() const = 0;

signals:
    void completeChanged();

protected:
    LibraryDetailsWidget *libraryDetailsWidget() const;

    AddLibraryWizard::Platforms platforms() const;
    AddLibraryWizard::LinkageType linkageType() const;
    AddLibraryWizard::MacLibraryType macLibraryType() const;
    Utils::OsType libraryPlatformType() const;
    QString libraryPlatformFilter() const;
    Utils::FilePath proFile() const;
    bool isIncludePathChanged() const;

    void updateGui();
    virtual AddLibraryWizard::LinkageType suggestedLinkageType() const = 0;
    virtual AddLibraryWizard::MacLibraryType suggestedMacLibraryType() const = 0;
    virtual QString suggestedIncludePath() const = 0;
    virtual void updateWindowsOptionsEnablement() = 0;

    void setPlatformsVisible(bool ena);
    void setLinkageRadiosVisible(bool ena);
    void setLinkageGroupVisible(bool ena);
    void setMacLibraryRadiosVisible(bool ena);
    void setMacLibraryGroupVisible(bool ena);
    void setLibraryPathChooserVisible(bool ena);
    void setLibraryComboBoxVisible(bool ena);
    void setPackageLineEditVisible(bool ena);
    void setIncludePathVisible(bool ena);
    void setWindowsGroupVisible(bool ena);
    void setRemoveSuffixVisible(bool ena);

    bool isMacLibraryRadiosVisible() const;
    bool isIncludePathVisible() const;
    bool isWindowsGroupVisible() const;

    Utils::Guard m_ignoreChanges;

private:
    void slotIncludePathChanged();
    void slotPlatformChanged();
    void slotMacLibraryTypeChanged();
    void slotUseSubfoldersChanged(bool ena);
    void slotAddSuffixChanged(bool ena);

    void showLinkageType(AddLibraryWizard::LinkageType linkageType);
    void showMacLibraryType(AddLibraryWizard::MacLibraryType libType);

    AddLibraryWizard::Platforms m_platforms = AddLibraryWizard::LinuxPlatform
                                            | AddLibraryWizard::MacPlatform
                                            | AddLibraryWizard::WindowsMinGWPlatform
                                            | AddLibraryWizard::WindowsMSVCPlatform;
    AddLibraryWizard::LinkageType m_linkageType = AddLibraryWizard::NoLinkage;
    AddLibraryWizard::MacLibraryType m_macLibraryType = AddLibraryWizard::NoLibraryType;

    Utils::FilePath m_proFile;

    bool m_includePathChanged = false;

    bool m_linkageRadiosVisible = true;
    bool m_macLibraryRadiosVisible = true;
    bool m_includePathVisible = true;
    bool m_windowsGroupVisible = true;

    LibraryDetailsWidget *m_libraryDetailsWidget;
    QWizard *m_wizard = nullptr;
};

class NonInternalLibraryDetailsController : public LibraryDetailsController
{
    Q_OBJECT
public:
    explicit NonInternalLibraryDetailsController(LibraryDetailsWidget *libraryDetails,
                                                 const Utils::FilePath &proFile,
                                                 QObject *parent = nullptr);
    bool isComplete() const override;
    QString snippet() const override;
protected:
    AddLibraryWizard::LinkageType suggestedLinkageType() const override final;
    AddLibraryWizard::MacLibraryType suggestedMacLibraryType() const override final;
    QString suggestedIncludePath() const override final;
    void updateWindowsOptionsEnablement() override;
private:
    void handleLinkageTypeChange();
    void handleLibraryTypeChange();
    void handleLibraryPathChange();

    void slotLinkageTypeChanged();
    void slotRemoveSuffixChanged(bool ena);
    void slotLibraryTypeChanged();
    void slotLibraryPathChanged();
};

class PackageLibraryDetailsController : public NonInternalLibraryDetailsController
{
    Q_OBJECT
public:
    explicit PackageLibraryDetailsController(LibraryDetailsWidget *libraryDetails,
                                             const Utils::FilePath &proFile,
                                             QObject *parent = nullptr);
    bool isComplete() const override;
    QString snippet() const override;
protected:
    void updateWindowsOptionsEnablement() override final {
        NonInternalLibraryDetailsController::updateWindowsOptionsEnablement();
    }
private:
    bool isLinkPackageGenerated() const;
};

class SystemLibraryDetailsController : public NonInternalLibraryDetailsController
{
    Q_OBJECT
public:
    explicit SystemLibraryDetailsController(LibraryDetailsWidget *libraryDetails,
                                            const Utils::FilePath &proFile,
                                            QObject *parent = nullptr);
protected:
    void updateWindowsOptionsEnablement() override final {
        NonInternalLibraryDetailsController::updateWindowsOptionsEnablement();
    }
};

class ExternalLibraryDetailsController : public NonInternalLibraryDetailsController
{
    Q_OBJECT
public:
    explicit ExternalLibraryDetailsController(LibraryDetailsWidget *libraryDetails,
                                              const Utils::FilePath &proFile,
                                              QObject *parent = nullptr);
protected:
    void updateWindowsOptionsEnablement() override final;
};

class InternalLibraryDetailsController : public LibraryDetailsController
{
    Q_OBJECT
public:
    explicit InternalLibraryDetailsController(LibraryDetailsWidget *libraryDetails,
                                              const Utils::FilePath &proFile,
                                              QObject *parent = nullptr);
    bool isComplete() const override;
    QString snippet() const override;
protected:
    AddLibraryWizard::LinkageType suggestedLinkageType() const override final;
    AddLibraryWizard::MacLibraryType suggestedMacLibraryType() const override final;
    QString suggestedIncludePath() const override final;
    void updateWindowsOptionsEnablement() override final;
private:
    void slotCurrentLibraryChanged();
    void updateProFile();

    QString m_rootProjectPath;
    QVector<QmakeProFile *> m_proFiles;
};

} // namespace Internal
} // namespace QmakeProjectManager
