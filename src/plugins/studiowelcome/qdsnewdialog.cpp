/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QQmlContext>
#include <QMessageBox>

#include "qdsnewdialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>
#include <utils/qtcassert.h>
#include <qmldesigner/components/componentcore/theme.h>

#include "createproject.h"
#include "wizardfactories.h"
#include "newprojectdialogimageprovider.h"

using namespace StudioWelcome;

namespace {

/*
 * NOTE: copied from projectexplorer/jsonwizard/jsonprojectpage.h
*/
QString uniqueProjectName(const QString &path)
{
    const QDir pathDir(path);
    //: File path suggestion for a new project. If you choose
    //: to translate it, make sure it is a valid path name without blanks
    //: and using only ascii chars.
    const QString prefix = QObject::tr("UntitledProject");

    QString name = prefix;
    int i = 0;
    while (pathDir.exists(name))
        name = prefix + QString::number(++i);

    return name;
}

}

/***********************/

QdsNewDialog::QdsNewDialog(QWidget *parent)
    : m_dialog{new QQuickWidget(parent)}
    , m_categoryModel{new NewProjectCategoryModel(this)}
    , m_projectModel{new NewProjectModel(this)}
    , m_screenSizeModel{new ScreenSizeModel(this)}
    , m_styleModel{new StyleModel(this)}
{
    setParent(m_dialog);

    m_dialog->rootContext()->setContextProperties(QVector<QQmlContext::PropertyPair>{
        {{"categoryModel"}, QVariant::fromValue(m_categoryModel.data())},
        {{"projectModel"}, QVariant::fromValue(m_projectModel.data())},
        {{"screenSizeModel"}, QVariant::fromValue(m_screenSizeModel.data())},
        {{"styleModel"}, QVariant::fromValue(m_styleModel.data())},
        {{"dialogBox"}, QVariant::fromValue(this)},
    });

    m_dialog->setResizeMode(QQuickWidget::SizeRootObjectToView); // SizeViewToRootObject
    m_dialog->engine()->addImageProvider(QStringLiteral("newprojectdialog_library"),
                                         new Internal::NewProjectDialogImageProvider());
    QmlDesigner::Theme::setupTheme(m_dialog->engine());
    m_dialog->engine()->addImportPath(Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources/imports").toString());
    m_dialog->engine()->addImportPath(Core::ICore::resourcePath("qmldesigner/newprojectdialog/imports").toString());
    QString sourcesPath = qmlPath();
    m_dialog->setSource(QUrl::fromLocalFile(sourcesPath));

    m_dialog->setWindowModality(Qt::ApplicationModal);
    m_dialog->setWindowFlags(Qt::Dialog);
    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
    m_dialog->setMinimumSize(1155, 804);

    QObject::connect(&m_wizard, &WizardHandler::deletingWizard, this, &QdsNewDialog::onDeletingWizard);
    QObject::connect(&m_wizard, &WizardHandler::wizardCreated, this, &QdsNewDialog::onWizardCreated);
    QObject::connect(&m_wizard, &WizardHandler::statusMessageChanged, this, &QdsNewDialog::onStatusMessageChanged);
    QObject::connect(&m_wizard, &WizardHandler::projectCanBeCreated, this, &QdsNewDialog::onProjectCanBeCreatedChanged);

    QObject::connect(&m_wizard, &WizardHandler::wizardCreationFailed, this, [this]() {
        QMessageBox::critical(m_dialog, "New project", "Failed to initialize data");
        reject();
        delete this;
    });

    QObject::connect(m_styleModel.data(), &StyleModel::modelAboutToBeReset, this, [this]() {
        this->m_qmlStyleIndex = -1;
    });
}

void QdsNewDialog::onDeletingWizard()
{
    m_screenSizeModel->setBackendModel(nullptr);
    m_qmlScreenSizeIndex = -1;
    m_screenSizeModel->reset();

    m_styleModel->setBackendModel(nullptr);
    m_qmlStyleIndex = -1;
}

void QdsNewDialog::setProjectName(const QString &name)
{
    m_qmlProjectName = name;
    m_wizard.setProjectName(name);
}

void QdsNewDialog::setProjectLocation(const QString &location)
{
    m_qmlProjectLocation = Utils::FilePath::fromString(QDir::toNativeSeparators(location));
    m_wizard.setProjectLocation(m_qmlProjectLocation);
}

void QdsNewDialog::onStatusMessageChanged(Utils::InfoLabel::InfoType type, const QString &message)
{
    switch (type) {
    case Utils::InfoLabel::Warning:
        m_qmlStatusType = "warning";
        break;
    case Utils::InfoLabel::Error:
        m_qmlStatusType = "error";
        break;
    default:
        m_qmlStatusType = "normal";
        break;
    }

    emit statusTypeChanged();

    m_qmlStatusMessage = message;
    emit statusMessageChanged();
}

void QdsNewDialog::onProjectCanBeCreatedChanged(bool value)
{
    if (m_qmlFieldsValid == value)
        return;

    m_qmlFieldsValid = value;

    emit fieldsValidChanged();
}

void QdsNewDialog::onWizardCreated(QStandardItemModel *screenSizeModel, QStandardItemModel *styleModel)
{
    m_screenSizeModel->setBackendModel(screenSizeModel);
    m_styleModel->setBackendModel(styleModel);

    if (m_qmlDetailsLoaded) {
        m_screenSizeModel->reset();
        emit haveVirtualKeyboardChanged();
        emit haveTargetQtVersionChanged();

        setProjectName(m_qmlProjectName);
        setProjectLocation(m_qmlProjectLocation.toString());
    }

    if (m_qmlStylesLoaded)
        m_styleModel->reset();
}

QString QdsNewDialog::currentProjectQmlPath() const
{
    if (!m_currentProject || m_currentProject->qmlPath.isEmpty())
        return "";

    return m_currentProject->qmlPath.toString();
}

void QdsNewDialog::setScreenSizeIndex(int index)
{
    m_wizard.setScreenSizeIndex(index);
    m_qmlScreenSizeIndex = index;
}

void QdsNewDialog::setTargetQtVersion(int index)
{
    m_wizard.setTargetQtVersionIndex(index);
    m_qmlTargetQtVersionIndex = index;
}

void QdsNewDialog::setStyleIndex(int index)
{
    if (!m_qmlStylesLoaded)
        return;

    if (index == -1) {
        m_qmlStyleIndex = index;
        return;
    }

    m_qmlStyleIndex = index;
    int actualIndex = m_styleModel->actualIndex(m_qmlStyleIndex);
    QTC_ASSERT(actualIndex >= 0, return);

    m_wizard.setStyleIndex(actualIndex);
}

int QdsNewDialog::getStyleIndex() const
{
    /**
     * m_wizard.styleIndex property is the wizard's (backend's) value of the style index.
     * The initial value (saved in the wizard.json) is read from there. Any subsequent reads of
     * the style index should use m_styleIndex, which is the QML's style index property. Setting
     * the style index should update both the m_styleIndex and the backend. In this regard, the
     * QdsNewDialog's m_styleIndex acts as some kind of cache.
    */

    if (!m_qmlStylesLoaded)
        return -1;

    if (m_qmlStyleIndex == -1) {
        int actualIndex = m_wizard.styleIndex();
        // Not nice, get sets the property... m_qmlStyleIndex acts like a cache.
        m_qmlStyleIndex = m_styleModel->filteredIndex(actualIndex);
        return m_qmlStyleIndex;
    }

    return m_styleModel->actualIndex(m_qmlStyleIndex);
}

void QdsNewDialog::setWizardFactories(QList<Core::IWizardFactory *> factories_,
                                      const Utils::FilePath &defaultLocation,
                                      const QVariantMap &)
{
    Utils::Id platform = Utils::Id::fromSetting("Desktop");

    WizardFactories factories{factories_, m_dialog, platform};

    m_categoryModel->setProjects(factories.projectsGroupedByCategory()); // calls model reset
    m_projectModel->setProjects(factories.projectsGroupedByCategory()); // calls model reset

    if (m_qmlSelectedProject > -1)
        setSelectedProject(m_qmlSelectedProject);

    if (factories.empty())
        return; // TODO: some message box?

    const Core::IWizardFactory *first = factories.front();
    Utils::FilePath projectLocation = first->runPath(defaultLocation);

    m_qmlProjectName = uniqueProjectName(projectLocation.toString());
    emit projectNameChanged(); // So that QML knows to update the field

    m_qmlProjectLocation = Utils::FilePath::fromString(QDir::toNativeSeparators(projectLocation.toString()));
    emit projectLocationChanged(); // So that QML knows to update the field

    if (m_qmlDetailsLoaded)
        m_screenSizeModel->reset();

    if (m_qmlStylesLoaded)
        m_styleModel->reset();
}

QString QdsNewDialog::qmlPath() const
{
    return Core::ICore::resourcePath("qmldesigner/newprojectdialog/NewProjectDialog.qml").toString();
}

void QdsNewDialog::showDialog()
{
    m_dialog->show();
}

bool QdsNewDialog::getHaveVirtualKeyboard() const
{
    return m_wizard.haveVirtualKeyboard();
}

bool QdsNewDialog::getHaveTargetQtVersion() const
{
    return m_wizard.haveTargetQtVersion();
}

void QdsNewDialog::accept()
{
    CreateProject create{m_wizard};

    create.withName(m_qmlProjectName)
        .atLocation(m_qmlProjectLocation)
        .withScreenSizes(m_qmlScreenSizeIndex, m_qmlCustomWidth, m_qmlCustomHeight)
        .withStyle(m_qmlStyleIndex)
        .useQtVirtualKeyboard(m_qmlUseVirtualKeyboard)
        .saveAsDefaultLocation(m_qmlSaveAsDefaultLocation)
        .withTargetQtVersion(m_qmlTargetQtVersionIndex)
        .execute();

    m_dialog->close();
}

void QdsNewDialog::reject()
{
    m_screenSizeModel->setBackendModel(nullptr);
    m_styleModel->setBackendModel(nullptr);
    m_wizard.destroyWizard();

    m_dialog->close();
}

QString QdsNewDialog::chooseProjectLocation()
{
    Utils::FilePath newPath = Utils::FileUtils::getExistingDirectory(m_dialog, tr("Choose Directory"),
                                                                     m_qmlProjectLocation);

    return QDir::toNativeSeparators(newPath.toString());
}

void QdsNewDialog::setSelectedProject(int selection)
{
    if (m_qmlSelectedProject != selection || m_projectPage != m_projectModel->page()) {
        m_qmlSelectedProject = selection;

        m_currentProject = m_projectModel->project(m_qmlSelectedProject);
        if (m_currentProject) {
            setProjectDescription(m_currentProject->description);

            m_projectPage = m_projectModel->page();
            m_wizard.reset(m_currentProject.value(), m_qmlSelectedProject, m_qmlProjectLocation);
        }
    }
}
