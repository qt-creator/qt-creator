/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "createandroidmanifestwizard.h"

#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <proparser/prowriter.h>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <qtsupport/qtkitinformation.h>
#include <coreplugin/editormanager/editormanager.h>

using namespace Android;
using namespace Android::Internal;
using QmakeProjectManager::QmakeProject;
using QmakeProjectManager::QmakeProFileNode;

//
// NoApplicationProFilePage
//
NoApplicationProFilePage::NoApplicationProFilePage(CreateAndroidManifestWizard *wizard)
    : m_wizard(wizard)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("No application .pro file found in this project."));
    layout->addWidget(label);
    setTitle(tr("No Application .pro File"));
}

//
// ChooseProFilePage
//
ChooseProFilePage::ChooseProFilePage(CreateAndroidManifestWizard *wizard, const QList<QmakeProFileNode *> &nodes)
    : m_wizard(wizard)
{
    QFormLayout *fl = new QFormLayout(this);
    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    label->setText(tr("Select the .pro file for which you want to create an AndroidManifest.xml file."));
    fl->addRow(label);

    m_comboBox = new QComboBox(this);
    foreach (QmakeProFileNode *node, nodes)
        m_comboBox->addItem(node->displayName(), QVariant::fromValue(static_cast<void *>(node))); // TODO something more?

    nodeSelected(0);
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(nodeSelected(int)));

    fl->addRow(tr(".pro file:"), m_comboBox);
    setTitle(tr("Select a .pro File"));
}

void ChooseProFilePage::nodeSelected(int index)
{
    Q_UNUSED(index)
    m_wizard->setNode(static_cast<QmakeProFileNode *>(m_comboBox->itemData(m_comboBox->currentIndex()).value<void *>()));
}


//
// ChooseDirectoryPage
//
ChooseDirectoryPage::ChooseDirectoryPage(CreateAndroidManifestWizard *wizard)
    : m_wizard(wizard), m_androidPackageSourceDir(0), m_complete(true)
{
    QString androidPackageDir = m_wizard->node()->singleVariableValue(QmakeProjectManager::AndroidPackageSourceDir);

    QFormLayout *fl = new QFormLayout(this);
    QLabel *label = new QLabel(this);
    label->setWordWrap(true);
    fl->addRow(label);

    m_sourceDirectoryWarning = new QLabel(this);
    m_sourceDirectoryWarning->setVisible(false);
    m_sourceDirectoryWarning->setText(tr("The Android package source directory cannot be the same as the project directory."));
    m_sourceDirectoryWarning->setWordWrap(true);
    m_warningIcon = new QLabel(this);
    m_warningIcon->setVisible(false);
    m_warningIcon->setPixmap(QPixmap(QLatin1String(":/projectexplorer/images/compile_error.png")));
    m_warningIcon->setWordWrap(true);
    m_warningIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(m_warningIcon);
    hbox->addWidget(m_sourceDirectoryWarning);
    hbox->setAlignment(m_warningIcon, Qt::AlignTop);

    fl->addRow(hbox);

    m_androidPackageSourceDir = new Utils::PathChooser(this);
    m_androidPackageSourceDir->setExpectedKind(Utils::PathChooser::Directory);
    fl->addRow(tr("Android package source directory:"), m_androidPackageSourceDir);

    if (androidPackageDir.isEmpty()) {
        label->setText(tr("Select the Android package source directory.\n\n"
                          "The files in the Android package source directory are copied to the build directory's "
                          "Android directory and the default files are overwritten."));

        m_androidPackageSourceDir->setPath(QFileInfo(m_wizard->node()->path()).absolutePath().append(QLatin1String("/android")));
        connect(m_androidPackageSourceDir, SIGNAL(changed(QString)),
                this, SLOT(checkPackageSourceDir()));
    } else {
        label->setText(tr("The Android manifest file will be created in the ANDROID_PACKAGE_SOURCE_DIR set in the .pro file."));
        m_androidPackageSourceDir->setPath(androidPackageDir);
        m_androidPackageSourceDir->setReadOnly(true);
    }


    m_wizard->setDirectory(m_androidPackageSourceDir->path());

    connect(m_androidPackageSourceDir, SIGNAL(pathChanged(QString)),
            m_wizard, SLOT(setDirectory(QString)));
}

void ChooseDirectoryPage::checkPackageSourceDir()
{
    QString projectDir = QFileInfo(m_wizard->node()->path()).absolutePath();
    QString newDir = m_androidPackageSourceDir->path();
    bool isComplete = QFileInfo(projectDir) != QFileInfo(newDir);

    m_sourceDirectoryWarning->setVisible(!isComplete);
    m_warningIcon->setVisible(!isComplete);

    if (isComplete != m_complete) {
        m_complete = isComplete;
        emit completeChanged();
    }
}

bool ChooseDirectoryPage::isComplete() const
{
    return m_complete;
}

//
// CreateAndroidManifestWizard
//
CreateAndroidManifestWizard::CreateAndroidManifestWizard(ProjectExplorer::Target *target)
    : m_target(target), m_node(0)
{
    setWindowTitle(tr("Create Android Manifest Wizard"));

    QmakeProject *project = static_cast<QmakeProject *>(target->project());
    QList<QmakeProFileNode *> nodes = project->applicationProFiles();
    if (nodes.isEmpty()) {
        // oh uhm can't create anything
        addPage(new NoApplicationProFilePage(this));
    } else if (nodes.size() == 1) {
        setNode(nodes.first());
        addPage(new ChooseDirectoryPage(this));
    } else {
        addPage(new ChooseProFilePage(this, nodes));
        addPage(new ChooseDirectoryPage(this));
    }
}

QmakeProjectManager::QmakeProFileNode *CreateAndroidManifestWizard::node() const
{
    return m_node;
}

void CreateAndroidManifestWizard::setNode(QmakeProjectManager::QmakeProFileNode *node)
{
    m_node = node;
}

void CreateAndroidManifestWizard::setDirectory(const QString &directory)
{
    m_directory = directory;
}

QString CreateAndroidManifestWizard::sourceFileName() const
{
    QString result;
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(m_target->kit());
    if (!version)
        return result;
    Utils::FileName srcPath
            = Utils::FileName::fromString(version->qmakeProperty("QT_INSTALL_PREFIX"))
            .appendPath(QLatin1String("src/android/java"));
    srcPath.appendPath(QLatin1String("AndroidManifest.xml"));
    return srcPath.toString();
}

void CreateAndroidManifestWizard::createAndroidManifestFile()
{
    if (m_directory.isEmpty())
        return;

    QDir dir;
    if (!QFileInfo(m_directory).exists())
        dir.mkpath(m_directory);
    QString fileName = m_directory + QLatin1String("/AndroidManifest.xml");
    if (QFileInfo(fileName).exists()) {
        if (QMessageBox::question(this, tr("Overwrite AndroidManifest.xml"),
                                 tr("Overwrite existing AndroidManifest.xml?"),
                                 QMessageBox::Yes, QMessageBox::No)
                == QMessageBox::Yes) {
            if (!QFile(m_directory + QLatin1String("/AndroidManifest.xml")).remove()) {
                QMessageBox::warning(this, tr("File Removal Error"),
                                     tr("Could not remove file %1.").arg(fileName));
                return;
            }
        } else {
            return;
        }
    }

    if (!QFile::copy(sourceFileName(), fileName)) {
        QMessageBox::warning(this, tr("File Creation Error"),
                             tr("Could not create file %1.").arg(fileName));
        return;
    }

    if (m_node->singleVariableValue(QmakeProjectManager::AndroidPackageSourceDir).isEmpty()) {
        // and now time for some magic
        QString dir = QFileInfo(fileName).absolutePath();
        QString value = QLatin1String("$$PWD/")
                + QDir(m_target->project()->projectDirectory()).relativeFilePath(dir);
        bool result =
                m_node->setProVariable(QLatin1String("ANDROID_PACKAGE_SOURCE_DIR"), QStringList(value));
        QStringList unChanged;
        m_node->addFiles(QStringList(fileName), &unChanged);

        if (result == false
                || !unChanged.isEmpty()) {
            QMessageBox::warning(this, tr("Project File not Updated"),
                                 tr("Could not update the .pro file %1.").arg(m_node->path()));
        }
    }
    Core::EditorManager::openEditor(fileName);
}

void CreateAndroidManifestWizard::accept()
{
    createAndroidManifestFile();
    Utils::Wizard::accept();
}
