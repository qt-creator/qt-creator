/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef VCPROJECTBUILDOPTIONSPAGE_H
#define VCPROJECTBUILDOPTIONSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QDialog>

#include "vcprojectmanagerconstants.h"

class QLabel;
class QLineEdit;
class QProcess;
class QPushButton;
class QTableWidget;

namespace VcProjectManager {
namespace Internal {

class SchemaOptionsWidget;

struct VcProjectValidator {
    enum ValidationRequest {
        ValidationRequest_Add,
        ValidationRequest_Edit
    };

    QString m_originalExecutable;  // used only for ValidationRequest_Edit
    QString m_executable;
    QProcess *m_process;
    ValidationRequest m_requestType;
};

struct MsBuildInformation {
    QString m_executable;
    QString m_version;
};

struct SchemaInformation {
    Constants::SchemaVersion m_schemaVersion;
    QString m_schemaFilePath;
};

class VcProjectEditMsBuildDialog : public QDialog
{
    Q_OBJECT

public:
    VcProjectEditMsBuildDialog(QWidget *parent = 0);
    ~VcProjectEditMsBuildDialog();

    QString path() const;
    void setPath(const QString &path);

private slots:
    void showBrowseFileDialog();

private:
    QLineEdit *m_pathChooser;
    QPushButton *m_browseButton;
};

class VcProjectBuildOptionsWidget : public QWidget
{
    Q_OBJECT

public:
    VcProjectBuildOptionsWidget(QWidget *parent = 0);
    ~VcProjectBuildOptionsWidget();

    MsBuildInformation build(int index);
    int buildCount();
    MsBuildInformation currentSelectedBuild() const;
    int currentSelectedRow() const;
    bool exists(const QString &exePath);
    void insertMSBuild(const MsBuildInformation &info);
    void removeBuild(int index);
    void updateMsBuild(const QString &exePath, const MsBuildInformation &newMsBuildInfo);
    SchemaOptionsWidget *schemaOptionsWidget() const;

private slots:
    void onTableRowIndexChange(int index);

signals:
    void addNewButtonClicked();
    void editButtonClicked();
    void deleteButtonClicked();
    void currentBuildSelectionChanged(int);

private:
    QPushButton *m_addMsBuildButton;
    QPushButton *m_editBuildButton;
    QPushButton *m_deleteBuildButton;
    QTableWidget *m_buildTableWidget;
    SchemaOptionsWidget *m_schemaOptionsWidget;
};

class VcProjectBuildOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    VcProjectBuildOptionsPage();
    ~VcProjectBuildOptionsPage();

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

    QVector<MsBuildInformation *> msBuilds() const;
    QList<SchemaInformation> schemaInfos() const;
    void loadSettings();
    void saveSettings();
    void startVersionCheck();

private slots:
    void addNewMsBuild();
    void editMsBuild();
    void deleteMsBuild();
    void versionCheckFinished();

signals:
    void vcOptionsUpdated();

private:
    QVector<MsBuildInformation *> m_msBuildInformations;
    QList<SchemaInformation> m_schemaInformations;
    QStringList m_schemaPaths;
    VcProjectBuildOptionsWidget *m_optionsWidget;
    VcProjectValidator m_validator;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTBUILDOPTIONSPAGE_H
