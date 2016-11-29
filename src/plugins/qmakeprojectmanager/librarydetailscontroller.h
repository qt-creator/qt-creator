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

#include "addlibrarywizard.h"

namespace QmakeProjectManager {
class QmakeProFileNode;
namespace Internal {

namespace Ui { class LibraryDetailsWidget; }

class LibraryDetailsController : public QObject
{
    Q_OBJECT
public:
    explicit LibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                      const QString &proFile,
                                      QObject *parent = 0);
    virtual bool isComplete() const = 0;
    virtual QString snippet() const = 0;

signals:
    void completeChanged();

protected:
    Ui::LibraryDetailsWidget *libraryDetailsWidget() const;

    AddLibraryWizard::Platforms platforms() const;
    AddLibraryWizard::LinkageType linkageType() const;
    AddLibraryWizard::MacLibraryType macLibraryType() const;
    QString proFile() const;
    bool isIncludePathChanged() const;
    bool guiSignalsIgnored() const;

    void updateGui();
    virtual AddLibraryWizard::LinkageType suggestedLinkageType() const = 0;
    virtual AddLibraryWizard::MacLibraryType suggestedMacLibraryType() const = 0;
    virtual QString suggestedIncludePath() const = 0;
    virtual void updateWindowsOptionsEnablement() = 0;

    void setIgnoreGuiSignals(bool ignore);

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

    QString m_proFile;

    bool m_ignoreGuiSignals = false;
    bool m_includePathChanged = false;

    bool m_linkageRadiosVisible = true;
    bool m_macLibraryRadiosVisible = true;
    bool m_includePathVisible = true;
    bool m_windowsGroupVisible = true;

    Ui::LibraryDetailsWidget *m_libraryDetailsWidget;
};

class NonInternalLibraryDetailsController : public LibraryDetailsController
{
    Q_OBJECT
public:
    explicit NonInternalLibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                                 const QString &proFile,
                                                 QObject *parent = 0);
    virtual bool isComplete() const;
    virtual QString snippet() const;
protected:
    virtual AddLibraryWizard::LinkageType suggestedLinkageType() const;
    virtual AddLibraryWizard::MacLibraryType suggestedMacLibraryType() const;
    virtual QString suggestedIncludePath() const;
    virtual void updateWindowsOptionsEnablement();
private:
    void slotLinkageTypeChanged();
    void slotRemoveSuffixChanged(bool ena);
    void slotLibraryPathChanged();
};

class PackageLibraryDetailsController : public NonInternalLibraryDetailsController
{
    Q_OBJECT
public:
    explicit PackageLibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                            const QString &proFile,
                                            QObject *parent = 0);
    virtual bool isComplete() const;
    virtual QString snippet() const;
private:
    bool isLinkPackageGenerated() const;
};

class SystemLibraryDetailsController : public NonInternalLibraryDetailsController
{
    Q_OBJECT
public:
    explicit SystemLibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                            const QString &proFile,
                                            QObject *parent = 0);
};

class ExternalLibraryDetailsController : public NonInternalLibraryDetailsController
{
    Q_OBJECT
public:
    explicit ExternalLibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                              const QString &proFile,
                                              QObject *parent = 0);
protected:
    virtual void updateWindowsOptionsEnablement();
};

class InternalLibraryDetailsController : public LibraryDetailsController
{
    Q_OBJECT
public:
    explicit InternalLibraryDetailsController(Ui::LibraryDetailsWidget *libraryDetails,
                                              const QString &proFile,
                                              QObject *parent = 0);
    virtual bool isComplete() const;
    virtual QString snippet() const;
protected:
    virtual AddLibraryWizard::LinkageType suggestedLinkageType() const;
    virtual AddLibraryWizard::MacLibraryType suggestedMacLibraryType() const;
    virtual QString suggestedIncludePath() const;
    virtual void updateWindowsOptionsEnablement();
private:
    void slotCurrentLibraryChanged();
    void updateProFile();

    QString m_rootProjectPath;
    QVector<QmakeProFileNode *> m_proFileNodes;
};

} // namespace Internal
} // namespace QmakeProjectManager
