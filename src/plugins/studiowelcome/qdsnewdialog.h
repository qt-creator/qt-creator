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

#pragma once

#include <QQuickWidget>

#include <coreplugin/dialogs/newdialog.h>
#include <utils/infolabel.h>
#include <utils/optional.h>

#include "wizardhandler.h"
#include "presetmodel.h"
#include "screensizemodel.h"
#include "stylemodel.h"
#include "recentpresets.h"

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace StudioWelcome {
class QdsNewDialog : public QObject, public Core::NewDialog
{
    Q_OBJECT

public:
    Q_PROPERTY(int selectedPreset MEMBER m_qmlSelectedPreset WRITE setSelectedPreset)
    Q_PROPERTY(QString projectName MEMBER m_qmlProjectName WRITE setProjectName NOTIFY projectNameChanged)
    Q_PROPERTY(QString projectLocation MEMBER m_qmlProjectLocation READ projectLocation WRITE setProjectLocation NOTIFY projectLocationChanged)
    Q_PROPERTY(QString projectDescription MEMBER m_qmlProjectDescription READ projectDescription WRITE setProjectDescription NOTIFY projectDescriptionChanged)
    Q_PROPERTY(QString customWidth MEMBER m_qmlCustomWidth)
    Q_PROPERTY(QString customHeight MEMBER m_qmlCustomHeight)
    Q_PROPERTY(int styleIndex MEMBER m_qmlStyleIndex READ getStyleIndex WRITE setStyleIndex)
    Q_PROPERTY(bool useVirtualKeyboard MEMBER m_qmlUseVirtualKeyboard READ getUseVirtualKeyboard WRITE setUseVirtualKeyboard NOTIFY useVirtualKeyboardChanged)
    Q_PROPERTY(bool haveVirtualKeyboard MEMBER m_qmlHaveVirtualKeyboard READ getHaveVirtualKeyboard NOTIFY haveVirtualKeyboardChanged)
    Q_PROPERTY(bool haveTargetQtVersion MEMBER m_qmlHaveTargetQtVersion READ getHaveTargetQtVersion NOTIFY haveTargetQtVersionChanged)
    Q_PROPERTY(bool saveAsDefaultLocation MEMBER m_qmlSaveAsDefaultLocation WRITE setSaveAsDefaultLocation)
    Q_PROPERTY(QString statusMessage MEMBER m_qmlStatusMessage READ getStatusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString statusType MEMBER m_qmlStatusType READ getStatusType NOTIFY statusTypeChanged)
    Q_PROPERTY(bool fieldsValid MEMBER m_qmlFieldsValid READ getFieldsValid NOTIFY fieldsValidChanged)

    Q_PROPERTY(bool detailsLoaded MEMBER m_qmlDetailsLoaded)
    Q_PROPERTY(bool stylesLoaded MEMBER m_qmlStylesLoaded)

    Q_INVOKABLE QString currentPresetQmlPath() const;
    // TODO: screen size index should better be a property
    Q_INVOKABLE void setScreenSizeIndex(int index); // called when ComboBox item is "activated"
    Q_INVOKABLE int screenSizeIndex() const;
    Q_INVOKABLE void setTargetQtVersion(int index);

    Q_INVOKABLE QString chooseProjectLocation();

    explicit QdsNewDialog(QWidget *parent);

    QWidget *widget() override { return m_dialog; }

    void setWizardFactories(QList<Core::IWizardFactory *> factories, const Utils::FilePath &defaultLocation,
                            const QVariantMap &extraVariables) override;
    void setWindowTitle(const QString &title) override { m_dialog->setWindowTitle(title); }
    void showDialog() override;
    void setSelectedPreset(int selection);

    void setStyleIndex(int index);
    int getStyleIndex() const;
    void setUseVirtualKeyboard(bool value) { m_qmlUseVirtualKeyboard = value; }
    bool getUseVirtualKeyboard() const { return m_qmlUseVirtualKeyboard; }

    bool getFieldsValid() const { return m_qmlFieldsValid; }
    bool getHaveVirtualKeyboard() const;
    bool getHaveTargetQtVersion() const;

    void setSaveAsDefaultLocation(bool value) { m_qmlSaveAsDefaultLocation = value; }

    QString getStatusMessage() const { return m_qmlStatusMessage; }
    QString getStatusType() const { return m_qmlStatusType; }

public slots:
    void accept();
    void reject();

signals:
    void projectNameChanged();
    void projectLocationChanged();
    void projectDescriptionChanged();
    void useVirtualKeyboardChanged();
    void haveVirtualKeyboardChanged();
    void haveTargetQtVersionChanged();
    void statusMessageChanged();
    void statusTypeChanged();
    void fieldsValidChanged();

private slots:
    void onStatusMessageChanged(Utils::InfoLabel::InfoType type, const QString &message);
    void onProjectCanBeCreatedChanged(bool value);

private:
    QString qmlPath() const;

    void setProjectName(const QString &name);
    void setProjectLocation(const QString &location);
    QString projectLocation() const { return m_qmlProjectLocation.toString(); }

    void setProjectDescription(const QString &description)
    {
        m_qmlProjectDescription = description;
        emit projectDescriptionChanged();
    }

    QString projectDescription() const { return m_qmlProjectDescription; }

private slots:
    void onDeletingWizard();
    void onWizardCreated(QStandardItemModel *screenSizeModel, QStandardItemModel *styleModel);

private:
    QQuickWidget *m_dialog = nullptr;

    PresetData m_presetData;
    QPointer<PresetCategoryModel> m_categoryModel;
    QPointer<PresetModel> m_presetModel;
    QPointer<ScreenSizeModel> m_screenSizeModel;
    QPointer<StyleModel> m_styleModel;
    QString m_qmlProjectName;
    Utils::FilePath m_qmlProjectLocation;
    QString m_qmlProjectDescription;
    int m_qmlSelectedPreset = -1;
    int m_qmlScreenSizeIndex = -1;
    int m_qmlTargetQtVersionIndex = -1;
    // m_qmlStyleIndex is like a cache, so it needs to be updated on get()
    mutable int m_qmlStyleIndex = -1;
    bool m_qmlUseVirtualKeyboard = false;
    bool m_qmlHaveVirtualKeyboard = false;
    bool m_qmlHaveTargetQtVersion = false;
    bool m_qmlSaveAsDefaultLocation = false;
    bool m_qmlFieldsValid = false;
    QString m_qmlStatusMessage;
    QString m_qmlStatusType;

    int m_presetPage = -1; // i.e. the page in the Presets View

    QString m_qmlCustomWidth;
    QString m_qmlCustomHeight;

    bool m_qmlDetailsLoaded = false;
    bool m_qmlStylesLoaded = false;

    Utils::optional<PresetItem> m_currentPreset;

    WizardHandler m_wizard;
    RecentPresetsStore m_recentsStore;
};

} //namespace StudioWelcome
