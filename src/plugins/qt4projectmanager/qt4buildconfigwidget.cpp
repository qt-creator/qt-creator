/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "qt4buildconfigwidget.h"

#include "makestep.h"
#include "qmakestep.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qt4projectmanager.h"
#include "ui_qt4buildconfigwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/mainwindow.h>

#include <QtGui/QFileDialog>

namespace {
bool debug = false;
}

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

Qt4BuildConfigWidget::Qt4BuildConfigWidget(Qt4Project *project)
    : BuildStepConfigWidget(),
      m_pro(project)
{
    m_ui = new Ui::Qt4BuildConfigWidget();
    m_ui->setupUi(this);
    m_ui->shadowBuildDirEdit->setPromptDialogTitle(tr("Shadow Build Directory"));
    m_ui->shadowBuildDirEdit->setExpectedKind(Core::Utils::PathChooser::Directory);
    m_ui->invalidQtWarningLabel->setVisible(false);

    connect(m_ui->nameLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(changeConfigName(QString)));

    connect(m_ui->shadowBuildCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(shadowBuildCheckBoxClicked(bool)));

    connect(m_ui->shadowBuildDirEdit, SIGNAL(beforeBrowsing()),
            this, SLOT(onBeforeBeforeShadowBuildDirBrowsed()));

    connect(m_ui->shadowBuildDirEdit, SIGNAL(changed()),
            this, SLOT(shadowBuildLineEditTextChanged()));

    connect(m_ui->qtVersionComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(qtVersionComboBoxCurrentIndexChanged(QString)));

    connect(m_ui->importLabel, SIGNAL(linkActivated(QString)),
            this, SLOT(importLabelClicked()));

    connect(m_ui->manageQtVersionPushButtons, SIGNAL(clicked()),
            this, SLOT(manageQtVersions()));

    connect(m_pro->qt4ProjectManager()->versionManager(), SIGNAL(qtVersionsChanged()),
            this, SLOT(setupQtVersionsComboBox()));
}

Qt4BuildConfigWidget::~Qt4BuildConfigWidget()
{
    delete m_ui;
}

void Qt4BuildConfigWidget::manageQtVersions()
{
    Core::ICore *core = Core::ICore::instance();
    core->showOptionsDialog(Constants::QT_CATEGORY, Constants::QTVERSION_PAGE);
}


QString Qt4BuildConfigWidget::displayName() const
{
    return tr("General");
}

void Qt4BuildConfigWidget::init(const QString &buildConfiguration)
{
    if (debug)
        qDebug() << "Qt4BuildConfigWidget::init()";

    m_buildConfiguration = buildConfiguration;

    m_ui->nameLineEdit->setText(m_pro->displayNameFor(m_buildConfiguration));

    setupQtVersionsComboBox();

    bool shadowBuild = m_pro->value(buildConfiguration, "useShadowBuild").toBool();
    m_ui->shadowBuildCheckBox->setChecked(shadowBuild);
    m_ui->shadowBuildDirEdit->setEnabled(shadowBuild);
    m_ui->shadowBuildDirEdit->setPath(m_pro->buildDirectory(buildConfiguration));
    shadowBuildLineEditTextChanged(); // Update the import label
}

void Qt4BuildConfigWidget::changeConfigName(const QString &newName)
{
    m_pro->setDisplayNameFor(m_buildConfiguration, newName);
}

void Qt4BuildConfigWidget::setupQtVersionsComboBox()
{
    if (m_buildConfiguration.isEmpty()) // not yet initialized
        return;

    disconnect(m_ui->qtVersionComboBox, SIGNAL(currentIndexChanged(QString)),
        this, SLOT(qtVersionComboBoxCurrentIndexChanged(QString)));

    m_ui->qtVersionComboBox->clear();
    m_ui->qtVersionComboBox->addItem(tr("Default Qt Version"), 0);

    if (m_pro->qtVersionId(m_buildConfiguration) == 0) {
        m_ui->qtVersionComboBox->setCurrentIndex(0);
        m_ui->invalidQtWarningLabel->setVisible(false);
    }
    // Add Qt Versions to the combo box
    QtVersionManager *vm = m_pro->qt4ProjectManager()->versionManager();
    const QList<QtVersion *> &versions = vm->versions();
    for (int i = 0; i < versions.size(); ++i) {
        m_ui->qtVersionComboBox->addItem(versions.at(i)->name(), versions.at(i)->uniqueId());

        if (versions.at(i)->uniqueId() == m_pro->qtVersionId(m_buildConfiguration)) {
            m_ui->qtVersionComboBox->setCurrentIndex(i + 1);
            m_ui->invalidQtWarningLabel->setVisible(!versions.at(i)->isValid());
        }
    }

    // And connect again
    connect(m_ui->qtVersionComboBox, SIGNAL(currentIndexChanged(QString)),
        this, SLOT(qtVersionComboBoxCurrentIndexChanged(QString)));
}

void Qt4BuildConfigWidget::onBeforeBeforeShadowBuildDirBrowsed()
{
    QString initialDirectory = QFileInfo(m_pro->file()->fileName()).absolutePath();
    if (!initialDirectory.isEmpty())
        m_ui->shadowBuildDirEdit->setInitialBrowsePathBackup(initialDirectory);
}

void Qt4BuildConfigWidget::shadowBuildCheckBoxClicked(bool checked)
{
    m_ui->shadowBuildDirEdit->setEnabled(checked);
    bool b = m_ui->shadowBuildCheckBox->isChecked();
    m_pro->setValue(m_buildConfiguration, "useShadowBuild", b);
    if (b)
        m_pro->setValue(m_buildConfiguration, "buildDirectory", m_ui->shadowBuildDirEdit->path());
    else
        m_pro->setValue(m_buildConfiguration, "buildDirectory", QVariant(QString::null));
}

void Qt4BuildConfigWidget::shadowBuildLineEditTextChanged()
{
    m_pro->setValue(m_buildConfiguration, "buildDirectory", m_ui->shadowBuildDirEdit->path());
    // if the directory already exists
    // check if we have a build in there and
    // offer to import it
    m_ui->importLabel->setVisible(false);
    if (m_ui->shadowBuildCheckBox->isChecked()) {
        QString qtPath = m_pro->qt4ProjectManager()->versionManager()->findQtVersionFromMakefile(m_ui->shadowBuildDirEdit->path());
        if (!qtPath.isEmpty()) {
            m_ui->importLabel->setVisible(true);
        }
    }

//    QFileInfo fi(m_ui->shadowBuildDirEdit->path());
//    if (fi.exists()) {
//        m_ui->shadowBuildLineEdit->setStyleSheet("");
//        m_ui->shadowBuildLineEdit->setToolTip("");
//    } else {
//        m_ui->shadowBuildLineEdit->setStyleSheet("background: red;");
//        m_ui->shadowBuildLineEdit->setToolTip(tr("Directory does not exist."));
//    }
}

void Qt4BuildConfigWidget::importLabelClicked()
{
    if (m_ui->shadowBuildCheckBox->isChecked()) {
        QString directory = m_ui->shadowBuildDirEdit->path();
        if (!directory.isEmpty()) {
            QtVersionManager *vm = m_pro->qt4ProjectManager()->versionManager();
            QString qtPath = vm->findQtVersionFromMakefile(directory);
            if (!qtPath.isEmpty()) {
                QtVersion *version = vm->qtVersionForDirectory(qtPath);
                if (!version) {
                    version = new QtVersion(QFileInfo(qtPath).baseName(), qtPath);
                    vm->addVersion(version);
                }
                QtVersion::QmakeBuildConfig qmakeBuildConfig = version->defaultBuildConfig();
                qmakeBuildConfig = vm->scanMakefileForQmakeConfig(directory, qmakeBuildConfig);

                // So we got all the information now apply it...
                m_pro->setQtVersion(m_buildConfiguration, version->uniqueId());
                // Combo box will be updated at the end

                // Find qmakestep...
                QMakeStep *qmakeStep = m_pro->qmakeStep();
                MakeStep *makeStep = m_pro->makeStep();

                qmakeStep->setValue(m_buildConfiguration, "buildConfiguration", int(qmakeBuildConfig));
                // Adjust command line arguments, this is ugly as hell
                // If we are switching to BuildAll we want "release" in there and no "debug"
                // or "debug" in there and no "release"
                // If we are switching to not BuildAl we want neither "release" nor "debug" in there
                QStringList makeCmdArguments = makeStep->value(m_buildConfiguration, "makeargs").toStringList();
                bool debug = qmakeBuildConfig & QtVersion::DebugBuild;
                if (qmakeBuildConfig & QtVersion::BuildAll) {
                    makeCmdArguments.removeAll(debug ? "release" : "debug");
                    if (!makeCmdArguments.contains(debug ? "debug" : "release"))
                        makeCmdArguments.append(debug ? "debug" : "release");
                } else {
                    makeCmdArguments.removeAll("debug");
                    makeCmdArguments.removeAll("remove");
                }
                makeStep->setValue(m_buildConfiguration, "makeargs", makeCmdArguments);
            }
        }
    }
    setupQtVersionsComboBox();
}

void Qt4BuildConfigWidget::qtVersionComboBoxCurrentIndexChanged(const QString &)
{
    //Qt Version
    int newQtVersion;
    if (m_ui->qtVersionComboBox->currentIndex() == 0) {
        newQtVersion = 0;
    } else {
        newQtVersion = m_ui->qtVersionComboBox->itemData(m_ui->qtVersionComboBox->currentIndex()).toInt();
    }
    bool isValid = m_pro->qt4ProjectManager()->versionManager()->version(newQtVersion)->isValid();
    m_ui->invalidQtWarningLabel->setVisible(!isValid);
    if (newQtVersion != m_pro->qtVersionId(m_buildConfiguration)) {
        m_pro->setQtVersion(m_buildConfiguration, newQtVersion);
        m_pro->update();
    }
}
