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
#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECTBUILDOPTIONSPAGE_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECTBUILDOPTIONSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QDialog>
#include <QTableWidgetItem>

#include "vcprojectmanagerconstants.h"

class QLabel;
class QLineEdit;
class QProcess;
class QPushButton;
class QTableWidget;

namespace VcProjectManager {
namespace Internal {

class SchemaOptionsWidget;
class MsBuildInformation;
class ToolSchemaWidget;

struct VcProjectValidator {
    enum ValidationRequest {
        ValidationRequest_Add,
        ValidationRequest_Edit
    };

    Core::Id m_originalMsInfoID; // used only for ValidationRequest_Edit

    QString m_executable;
    QProcess *m_process;
    ValidationRequest m_requestType;
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

class MsBuildTableItem : public QTableWidgetItem
{
public:
    MsBuildTableItem();
    ~MsBuildTableItem();

    Core::Id msBuildID() const;
    void setMsBuildID(Core::Id id);

private:
    Core::Id m_id;
};

class VcProjectBuildOptionsWidget : public QWidget
{
    Q_OBJECT

public:
    VcProjectBuildOptionsWidget(QWidget *parent = 0);
    ~VcProjectBuildOptionsWidget();

    Core::Id currentSelectedBuildId() const;
    bool exists(const QString &exePath);
    bool hasAnyBuilds() const;
    void insertMSBuild(MsBuildInformation *msBuild);
    void removeMsBuild(Core::Id msBuildId);
    void replaceMsBuild(Core::Id msBuildId, MsBuildInformation *newMsBuild);
    void saveSettings() const;
    SchemaOptionsWidget *schemaOptionsWidget() const;

private slots:
    void onMsBuildAdded(Core::Id msBuildId);
    void onMsBuildReplaced(Core::Id oldMsBuildId, Core::Id newMsBuildId);
    void onMsBuildRemoved(Core::Id msBuildId);
    void onTableRowIndexChange(int index);

private:
    void insertMsBuildIntoTable(MsBuildInformation *msBuild);

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
    ToolSchemaWidget *m_toolSchemaWidget;

    QList<MsBuildInformation *> m_newMsBuilds;
    QList<Core::Id> m_removedMsBuilds;
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

    void saveSettings();
    void startVersionCheck();

private slots:
    void addNewMsBuild();
    void editMsBuild();
    void deleteMsBuild();
    void versionCheckFinished();

private:
    VcProjectBuildOptionsWidget *m_optionsWidget;
    VcProjectValidator m_validator;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECTBUILDOPTIONSPAGE_H
