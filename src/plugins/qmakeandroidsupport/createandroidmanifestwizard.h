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

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/wizard.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QFormLayout;
QT_END_NAMESPACE

namespace ProjectExplorer { class Target; }
namespace QmakeProjectManager { class QmakeProFileNode; }

namespace QmakeAndroidSupport {
namespace Internal {

class CreateAndroidManifestWizard;

class NoApplicationProFilePage : public QWizardPage
{
    Q_OBJECT
public:
    NoApplicationProFilePage(CreateAndroidManifestWizard *wizard);
private:
    CreateAndroidManifestWizard *m_wizard;
};

class ChooseProFilePage : public QWizardPage
{
    Q_OBJECT
public:
    ChooseProFilePage(CreateAndroidManifestWizard *wizard, const QList<QmakeProjectManager::QmakeProFileNode *> &nodes, const QmakeProjectManager::QmakeProFileNode *select);
private:
    void nodeSelected(int index);
private:
    CreateAndroidManifestWizard *m_wizard;
    QComboBox *m_comboBox;
};

class ChooseDirectoryPage : public QWizardPage
{
    Q_OBJECT
public:
    ChooseDirectoryPage(CreateAndroidManifestWizard *wizard);
    void initializePage();
protected:
    bool isComplete() const;
private:
    void checkPackageSourceDir();
private:
    CreateAndroidManifestWizard *m_wizard;
    Utils::PathChooser *m_androidPackageSourceDir;
    QLabel *m_sourceDirectoryWarning;
    QLabel *m_warningIcon;
    QLabel *m_label;
    QFormLayout *m_layout;
    bool m_complete;
};

class CreateAndroidManifestWizard : public Utils::Wizard
{
    Q_OBJECT
public:
    CreateAndroidManifestWizard(ProjectExplorer::Target *target);

    QmakeProjectManager::QmakeProFileNode *node() const;
    void setNode(QmakeProjectManager::QmakeProFileNode *node);

    void accept();
    bool copyGradle();

    void setDirectory(const QString &directory);
    void setCopyGradle(bool copy);

private:
    enum CopyState {
        Ask,
        OverwriteAll,
        SkipAll
    };
    bool copy(const QFileInfo &src, const QFileInfo &dst, QStringList *addedFiles);

    void createAndroidManifestFile();
    void createAndroidTemplateFiles();
    ProjectExplorer::Target *m_target;
    QmakeProjectManager::QmakeProFileNode *m_node;
    QString m_directory;
    CopyState m_copyState;
    bool m_copyGradle;
};

} //namespace QmakeAndroidSupport
} //namespace Internal
