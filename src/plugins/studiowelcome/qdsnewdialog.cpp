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

#include "qdsnewdialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>
#include <qmldesigner/components/componentcore/theme.h>

#include "createproject.h"
#include "wizardfactories.h"
#include "newprojectdialogimageprovider.h"

#include <QMessageBox>
#include <QQmlContext>
#include <QScreen>

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
    , m_categoryModel{new PresetCategoryModel(&m_presetData, this)}
    , m_presetModel{new PresetModel(&m_presetData, this)}
    , m_screenSizeModel{new ScreenSizeModel(this)}
    , m_styleModel{new StyleModel(this)}
    , m_recentsStore{Core::ICore::settings()}
{
    setParent(m_dialog);

    m_dialog->setResizeMode(QQuickWidget::SizeRootObjectToView); // SizeViewToRootObject
    m_dialog->engine()->addImageProvider(QStringLiteral("newprojectdialog_library"),
                                         new Internal::NewProjectDialogImageProvider());
    QmlDesigner::Theme::setupTheme(m_dialog->engine());
    qmlRegisterSingletonInstance<QdsNewDialog>("BackendApi", 1, 0, "BackendApi", this);
    m_dialog->engine()->addImportPath(Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources/imports").toString());
    m_dialog->engine()->addImportPath(Core::ICore::resourcePath("qmldesigner/newprojectdialog/imports").toString());
    m_dialog->setSource(QUrl::fromLocalFile(qmlPath()));

    m_dialog->setWindowModality(Qt::ApplicationModal);
    m_dialog->setWindowFlags(Qt::Dialog);
    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
    m_dialog->setMinimumSize(1149, 554);

    QSize screenSize = m_dialog->screen()->geometry().size();
    if (screenSize.height() < 1080)
        m_dialog->resize(parent->size());

    QObject::connect(&m_wizard, &WizardHandler::deletingWizard, this, &QdsNewDialog::onDeletingWizard);
    QObject::connect(&m_wizard, &WizardHandler::wizardCreated, this, &QdsNewDialog::onWizardCreated);
    QObject::connect(&m_wizard, &WizardHandler::statusMessageChanged, this, &QdsNewDialog::onStatusMessageChanged);
    QObject::connect(&m_wizard, &WizardHandler::projectCanBeCreated, this, &QdsNewDialog::onProjectCanBeCreatedChanged);

    m_dialog->installEventFilter(this);

    QObject::connect(&m_wizard, &WizardHandler::wizardCreationFailed, this, [this]() {
        QMessageBox::critical(m_dialog, tr("New project"), tr("Failed to initialize data"));
        reject();
        delete this;
    });

    QObject::connect(m_styleModel.data(), &StyleModel::modelAboutToBeReset, this, [this]() {
        m_qmlStyleIndex = -1;
    });
}

bool QdsNewDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_dialog && event->type() == QEvent::KeyPress
        && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
        reject();
        return true;
    }

    return false;
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

void QdsNewDialog::updateScreenSizes()
{
    int index = m_wizard.screenSizeIndex(m_currentPreset->screenSizeName);
    if (index > -1) {
        setScreenSizeIndex(index);
    } else {
        index = m_screenSizeModel->appendItem(m_currentPreset->screenSizeName);
        setScreenSizeIndex(index);
    }

    m_screenSizeModel->reset();
}

void QdsNewDialog::onWizardCreated(QStandardItemModel *screenSizeModel, QStandardItemModel *styleModel)
{
    m_screenSizeModel->setBackendModel(screenSizeModel);
    m_styleModel->setBackendModel(styleModel);

    auto userPreset = m_currentPreset->asUserPreset();

    if (m_qmlDetailsLoaded) {
        if (m_currentPreset->isUserPreset()) {
            if (m_wizard.haveVirtualKeyboard())
                setUseVirtualKeyboard(userPreset->useQtVirtualKeyboard);

            if (m_wizard.haveTargetQtVersion()) {
                int index = m_wizard.targetQtVersionIndex(userPreset->qtVersion);
                if (index != -1)
                    setTargetQtVersionIndex(index);
            }
        } else {
            if (m_wizard.haveTargetQtVersion()) {
                int index = m_wizard.targetQtVersionIndex();
                if (index != -1)
                    setTargetQtVersionIndex(index);
            }
        }

        emit haveVirtualKeyboardChanged();
        emit haveTargetQtVersionChanged();

        updateScreenSizes();

        setProjectName(m_qmlProjectName);
        setProjectLocation(m_qmlProjectLocation.toString());
    }

    if (m_qmlStylesLoaded && m_wizard.haveStyleModel()) {
        if (m_currentPreset->isUserPreset()) {
            int index = m_wizard.styleIndex(userPreset->styleName);
            if (index != -1)
                setStyleIndex(index);
        } else {
            /* NOTE: For a builtin preset, we don't need to set style index. That's because defaults
             *       will be loaded from the backend Wizard.
             */
        }
        m_styleModel->reset();
    }
}

QString QdsNewDialog::currentPresetQmlPath() const
{
    if (!m_currentPreset || m_currentPreset->qmlPath.isEmpty())
        return {};

    return m_currentPreset->qmlPath.toString();
}

void QdsNewDialog::setScreenSizeIndex(int index)
{
    m_wizard.setScreenSizeIndex(index);
    m_qmlScreenSizeIndex = index;
}

int QdsNewDialog::screenSizeIndex() const
{
    return m_wizard.screenSizeIndex();
}

void QdsNewDialog::setTargetQtVersionIndex(int index)
{
    if (m_qmlTargetQtVersionIndex != index) {
        m_wizard.setTargetQtVersionIndex(index);
        m_qmlTargetQtVersionIndex = index;

        emit targetQtVersionIndexChanged();
    }
}

int QdsNewDialog::getTargetQtVersionIndex() const
{
    return m_qmlTargetQtVersionIndex;
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

void QdsNewDialog::setUseVirtualKeyboard(bool value)
{
    if (m_qmlUseVirtualKeyboard != value) {
        m_qmlUseVirtualKeyboard = value;
        emit useVirtualKeyboardChanged();
    }
}

void QdsNewDialog::setWizardFactories(QList<Core::IWizardFactory *> factories_,
                                      const Utils::FilePath &defaultLocation,
                                      const QVariantMap &)
{
    Utils::Id platform = Utils::Id::fromSetting("Desktop");

    WizardFactories factories{factories_, m_dialog, platform};

    std::vector<RecentPresetData> recents = m_recentsStore.fetchAll();
    std::vector<UserPresetData> userPresets =  m_userPresetsStore.fetchAll();
    m_presetData.setData(factories.presetsGroupedByCategory(), userPresets, recents);

    m_categoryModel->reset();
    m_presetModel->reset();

    if (m_qmlSelectedPreset > -1)
        setSelectedPreset(m_qmlSelectedPreset);

    if (factories.empty())
        return; // TODO: some message box?

    const Core::IWizardFactory *first = factories.front();
    Utils::FilePath projectLocation = first->runPath(defaultLocation);

    m_qmlProjectName = uniqueProjectName(projectLocation.toString());
    emit projectNameChanged(); // So that QML knows to update the field

    m_qmlProjectLocation = Utils::FilePath::fromString(QDir::toNativeSeparators(projectLocation.toString()));
    emit projectLocationChanged(); // So that QML knows to update the field

    /* NOTE:
     * Here we expect that details are loaded && that styles are loaded. We use the
     * functionality below to update the state of the first item that is selected right when
     * the dialog pops up. Otherwise, relying solely on onWizardCreated is not useful, since
     * for the dialog popup, that wizard is created before details & styles are loaded.
     *
     * It might be a better alternative to receive notifications from QML in the cpp file, that
     * style is loaded, and that details is loaded, and do updates from there. But, if we handle
     * those events, they may be called before the wizard is created - so we would need to make
     * sure that all events have occurred before we go ahead and configure the wizard.
    */

    auto userPreset = m_currentPreset->asUserPreset();

    if (m_qmlDetailsLoaded) {
        updateScreenSizes();

        if (m_wizard.haveTargetQtVersion()) {
            int index = (userPreset ? m_wizard.targetQtVersionIndex(userPreset->qtVersion)
                                    : m_wizard.targetQtVersionIndex());
            if (index != -1)
                setTargetQtVersionIndex(index);
        }

        if (m_wizard.haveVirtualKeyboard() && userPreset)
            setUseVirtualKeyboard(userPreset->useQtVirtualKeyboard);

        emit haveVirtualKeyboardChanged();
        emit haveTargetQtVersionChanged();
    }

    if (m_qmlStylesLoaded && m_wizard.haveStyleModel()) {
        if (userPreset) {
            int index = m_wizard.styleIndex(userPreset->styleName);
            if (index != -1)
                setStyleIndex(index);
        }
        m_styleModel->reset();
    }
}

QString QdsNewDialog::recentsTabName() const
{
    return PresetData::recentsTabName();
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

    m_dialog->hide();
    create.withName(m_qmlProjectName)
        .atLocation(m_qmlProjectLocation)
        .withScreenSizes(m_qmlScreenSizeIndex, m_qmlCustomWidth, m_qmlCustomHeight)
        .withStyle(m_qmlStyleIndex)
        .useQtVirtualKeyboard(m_qmlUseVirtualKeyboard)
        .saveAsDefaultLocation(m_qmlSaveAsDefaultLocation)
        .withTargetQtVersion(m_qmlTargetQtVersionIndex)
        .execute();

    std::shared_ptr<PresetItem> item = m_wizard.preset();
    QString customSizeName = m_qmlCustomWidth + " x " + m_qmlCustomHeight;

    m_recentsStore.add(item->categoryId, item->displayName(), customSizeName, item->isUserPreset());

    m_dialog->close();
    m_dialog->deleteLater();
    m_dialog = nullptr;
}

void QdsNewDialog::reject()
{
    m_screenSizeModel->setBackendModel(nullptr);
    m_styleModel->setBackendModel(nullptr);
    m_wizard.destroyWizard();

    m_dialog->close();
    m_dialog = nullptr;
}

QString QdsNewDialog::chooseProjectLocation()
{
    Utils::FilePath newPath = Utils::FileUtils::getExistingDirectory(m_dialog, tr("Choose Directory"),
                                                                     m_qmlProjectLocation);

    return QDir::toNativeSeparators(newPath.toString());
}

void QdsNewDialog::setSelectedPreset(int selection)
{
    if (m_qmlSelectedPreset != selection || m_presetPage != m_presetModel->page()) {
        m_qmlSelectedPreset = selection;

        m_currentPreset = m_presetModel->preset(m_qmlSelectedPreset);
        if (m_currentPreset) {
            setProjectDescription(m_currentPreset->description);

            m_presetPage = m_presetModel->page();
            m_wizard.reset(m_currentPreset, m_qmlSelectedPreset);
        }
    }
}

void QdsNewDialog::savePresetDialogAccept()
{
    QString screenSize = m_qmlCustomWidth + " x " + m_qmlCustomHeight;
    QString targetQtVersion = "";
    QString styleName = "";
    bool useVirtualKeyboard = false;

    if (m_wizard.haveTargetQtVersion())
        targetQtVersion = m_wizard.targetQtVersionName(m_qmlTargetQtVersionIndex);

    if (m_wizard.haveStyleModel())
        styleName = m_wizard.styleName(m_qmlStyleIndex);

    if (m_wizard.haveVirtualKeyboard())
        useVirtualKeyboard = m_qmlUseVirtualKeyboard;

    UserPresetData preset = {m_currentPreset->categoryId,
                             m_currentPreset->wizardName,
                             m_qmlPresetName,
                             screenSize,
                             useVirtualKeyboard,
                             targetQtVersion,
                             styleName};

    if (!m_userPresetsStore.save(preset)) {
        QMessageBox::warning(m_dialog,
                             tr("Save Preset"),
                             tr("A preset with this name already exists."));
        return;
    }

    // reload model
    std::vector<RecentPresetData> recents = m_recentsStore.fetchAll();
    std::vector<UserPresetData> userPresets = m_userPresetsStore.fetchAll();
    m_presetData.reload(userPresets, recents);

    m_categoryModel->reset();

    emit userPresetSaved();
}

void QdsNewDialog::removeCurrentPreset()
{
    if (!m_currentPreset->isUserPreset()) {
        qWarning() << "Will not attempt to remove non-user preset";
        return;
    }

    // remove preset & reload model
    std::vector<RecentPresetData> recents = m_recentsStore.remove(m_currentPreset->categoryId,
                                                                  m_currentPreset->displayName());

    auto userPreset = m_currentPreset->asUserPreset();
    m_userPresetsStore.remove(userPreset->categoryId, userPreset->displayName());
    std::vector<UserPresetData> userPresets = m_userPresetsStore.fetchAll();
    m_presetData.reload(userPresets, recents);

    m_qmlSelectedPreset = -1;
    m_presetPage = -1;

    if (userPresets.size() == 0) {
        m_presetModel->setPage(0);
        emit lastUserPresetRemoved();
    }

    m_categoryModel->reset();
    m_presetModel->reset();
}
