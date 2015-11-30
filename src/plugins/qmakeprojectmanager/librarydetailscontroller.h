/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef LIBRARYDETAILSCONTROLLER_H
#define LIBRARYDETAILSCONTROLLER_H

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

    enum CreatorPlatform {
        CreatorLinux,
        CreatorMac,
        CreatorWindows
    };

    CreatorPlatform creatorPlatform() const;

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

private slots:
    void slotIncludePathChanged();
    void slotPlatformChanged();
    void slotMacLibraryTypeChanged();
    void slotUseSubfoldersChanged(bool ena);
    void slotAddSuffixChanged(bool ena);
private:

    void showLinkageType(AddLibraryWizard::LinkageType linkageType);
    void showMacLibraryType(AddLibraryWizard::MacLibraryType libType);

    AddLibraryWizard::Platforms m_platforms = AddLibraryWizard::LinuxPlatform
                                            | AddLibraryWizard::MacPlatform
                                            | AddLibraryWizard::WindowsMinGWPlatform
                                            | AddLibraryWizard::WindowsMSVCPlatform;
    AddLibraryWizard::LinkageType m_linkageType = AddLibraryWizard::NoLinkage;
    AddLibraryWizard::MacLibraryType m_macLibraryType = AddLibraryWizard::NoLibraryType;

    QString m_proFile;

    CreatorPlatform m_creatorPlatform;

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
private slots:
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
private slots:
    void slotCurrentLibraryChanged();
    void updateProFile();
private:
    QString m_rootProjectPath;
    QVector<QmakeProFileNode *> m_proFileNodes;
};

} // namespace Internal
} // namespace QmakeProjectManager

#endif // LIBRARYDETAILSCONTROLLER_H
