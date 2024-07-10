// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidsdkmanager.h"
#include "androidsdkmanagerdialog.h"
#include "androidsdkmodel.h"
#include "androidtr.h"

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSortFilterProxyModel>
#include <QTreeView>

using namespace Utils;
using namespace std::placeholders;

namespace Android::Internal {

class OptionsDialog : public QDialog
{
public:
    OptionsDialog(AndroidSdkManager *sdkManager, QWidget *parent)
        : QDialog(parent)
    {
        QTC_CHECK(sdkManager);
        resize(800, 480);
        setWindowTitle(Tr::tr("SDK Manager Arguments"));

        m_argumentDetailsEdit = new QPlainTextEdit;
        m_argumentDetailsEdit->setReadOnly(true);

        m_process.setEnvironment(AndroidConfig::toolsEnvironment());
        m_process.setCommand({AndroidConfig::sdkManagerToolPath(),
                              {"--help", "--sdk_root=" + AndroidConfig::sdkLocation().toString()}});
        connect(&m_process, &Process::done, this, [this] {
            const QString output = m_process.allOutput();
            QString argumentDetails;
            const int tagIndex = output.indexOf("Common Arguments:");
            if (tagIndex >= 0) {
                const int detailsIndex = output.indexOf('\n', tagIndex);
                if (detailsIndex >= 0)
                    argumentDetails = output.mid(detailsIndex + 1);
            }
            if (argumentDetails.isEmpty())
                argumentDetails = Tr::tr("Cannot load available arguments for \"sdkmanager\" command.");
            m_argumentDetailsEdit->setPlainText(argumentDetails);
        });
        m_process.start();

        auto dialogButtons = new QDialogButtonBox;
        dialogButtons->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        connect(dialogButtons, &QDialogButtonBox::accepted, this, &OptionsDialog::accept);
        connect(dialogButtons, &QDialogButtonBox::rejected, this, &OptionsDialog::reject);

        m_argumentsEdit = new QLineEdit;
        m_argumentsEdit->setText(AndroidConfig::sdkManagerToolArgs().join(" "));

        using namespace Layouting;

        Column {
            Form { Tr::tr("SDK manager arguments:"), m_argumentsEdit, br },
            Tr::tr("Available arguments:"),
            m_argumentDetailsEdit,
            dialogButtons,
        }.attachTo(this);
    }

    QStringList sdkManagerArguments() const
    {
        const QString userInput = m_argumentsEdit->text().simplified();
        return userInput.isEmpty() ? QStringList() : userInput.split(' ');
    }

private:
    QPlainTextEdit *m_argumentDetailsEdit = nullptr;
    QLineEdit *m_argumentsEdit = nullptr;
    Process m_process;
};

class PackageFilterModel : public QSortFilterProxyModel
{
public:
    PackageFilterModel(AndroidSdkModel *sdkModel);

    void setAcceptedPackageState(AndroidSdkPackage::PackageState state);
    void setAcceptedSearchPackage(const QString &text);
    bool filterAcceptsRow(int source_row, const QModelIndex &sourceParent) const override;

private:
    AndroidSdkPackage::PackageState m_packageState = AndroidSdkPackage::AnyValidState;
    QString m_searchText;
};

class AndroidSdkManagerDialog : public QDialog
{
public:
    AndroidSdkManagerDialog(AndroidSdkManager *sdkManager, QWidget *parent);

private:
    AndroidSdkManager *m_sdkManager = nullptr;
    AndroidSdkModel *m_sdkModel = nullptr;
};

AndroidSdkManagerDialog::AndroidSdkManagerDialog(AndroidSdkManager *sdkManager, QWidget *parent)
    : QDialog(parent)
    , m_sdkManager(sdkManager)
    , m_sdkModel(new AndroidSdkModel(m_sdkManager, this))
{
    QTC_CHECK(sdkManager);

    setWindowTitle(Tr::tr("Android SDK Manager"));
    resize(664, 396);
    setModal(true);

    auto packagesView = new QTreeView;
    packagesView->setIndentation(20);
    packagesView->header()->setCascadingSectionResizes(false);

    auto updateInstalledButton = new QPushButton(Tr::tr("Update Installed"));

    auto channelCheckbox = new QComboBox;
    channelCheckbox->addItem(Tr::tr("Default"));
    channelCheckbox->addItem(Tr::tr("Stable"));
    channelCheckbox->addItem(Tr::tr("Beta"));
    channelCheckbox->addItem(Tr::tr("Dev"));
    channelCheckbox->addItem(Tr::tr("Canary"));

    auto obsoleteCheckBox = new QCheckBox(Tr::tr("Include obsolete"));

    auto showAvailableRadio = new QRadioButton(Tr::tr("Available"));
    auto showInstalledRadio = new QRadioButton(Tr::tr("Installed"));
    auto showAllRadio = new QRadioButton(Tr::tr("All"));
    showAllRadio->setChecked(true);

    auto optionsButton = new QPushButton(Tr::tr("Advanced Options..."));

    auto searchField = new FancyLineEdit;
    searchField->setPlaceholderText("Filter");

    auto expandCheck = new QCheckBox(Tr::tr("Expand All"));

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

    auto proxyModel = new PackageFilterModel(m_sdkModel);
    packagesView->setModel(proxyModel);
    packagesView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    packagesView->header()->setSectionResizeMode(AndroidSdkModel::packageNameColumn,
                                                       QHeaderView::Stretch);
    packagesView->header()->setStretchLastSection(false);

    using namespace Layouting;
    Column {
        Grid {
            Row {searchField, expandCheck}, br,
            packagesView,
            Column {
                updateInstalledButton,
                st,
                Group {
                    title(Tr::tr("Show Packages")),
                    Column {
                        Row { Tr::tr("Channel:"), channelCheckbox },
                        obsoleteCheckBox,
                        hr,
                        showAvailableRadio,
                        showInstalledRadio,
                        showAllRadio,
                    }
                },
                optionsButton,
            }, br,
        },
        buttonBox,
    }.attachTo(this);

    connect(m_sdkModel, &AndroidSdkModel::dataChanged, this, [this, buttonBox] {
        buttonBox->button(QDialogButtonBox::Apply)
            ->setEnabled(m_sdkModel->installationChange().count());
    });

    connect(expandCheck, &QCheckBox::stateChanged, this, [packagesView](int state) {
        if (state == Qt::Checked)
            packagesView->expandAll();
        else
            packagesView->collapseAll();
    });
    connect(updateInstalledButton, &QPushButton::clicked,
            m_sdkManager, &AndroidSdkManager::runUpdate);
    connect(showAllRadio, &QRadioButton::toggled, this, [this, proxyModel](bool checked) {
        if (checked) {
            proxyModel->setAcceptedPackageState(AndroidSdkPackage::AnyValidState);
            m_sdkModel->resetSelection();
        }
    });
    connect(showInstalledRadio, &QRadioButton::toggled, this, [this, proxyModel](bool checked) {
        if (checked) {
            proxyModel->setAcceptedPackageState(AndroidSdkPackage::Installed);
            m_sdkModel->resetSelection();
        }
    });
    connect(showAvailableRadio, &QRadioButton::toggled, this, [this, proxyModel](bool checked) {
        if (checked) {
            proxyModel->setAcceptedPackageState(AndroidSdkPackage::Available);
            m_sdkModel->resetSelection();
        }
    });

    connect(searchField, &QLineEdit::textChanged,
            this, [this, proxyModel, expandCheck](const QString &text) {
        proxyModel->setAcceptedSearchPackage(text);
        m_sdkModel->resetSelection();
        // It is more convenient to expand the view with the results
        expandCheck->setChecked(!text.isEmpty());
    });

    connect(buttonBox->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, [this] {
        m_sdkManager->runInstallationChange(m_sdkModel->installationChange());
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AndroidSdkManagerDialog::reject);

    connect(optionsButton, &QPushButton::clicked, this, [this] {
        OptionsDialog dlg(m_sdkManager, this);
        if (dlg.exec() == QDialog::Accepted) {
            QStringList arguments = dlg.sdkManagerArguments();
            if (arguments != AndroidConfig::sdkManagerToolArgs()) {
                AndroidConfig::setSdkManagerToolArgs(arguments);
                m_sdkManager->reloadPackages();
            }
        }
    });

    connect(obsoleteCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
        const QString obsoleteArg = "--include_obsolete";
        QStringList args = AndroidConfig::sdkManagerToolArgs();
        if (state == Qt::Checked && !args.contains(obsoleteArg)) {
            args.append(obsoleteArg);
            AndroidConfig::setSdkManagerToolArgs(args);
       } else if (state == Qt::Unchecked && args.contains(obsoleteArg)) {
            args.removeAll(obsoleteArg);
            AndroidConfig::setSdkManagerToolArgs(args);
       }
        m_sdkManager->reloadPackages();
    });

    connect(channelCheckbox, &QComboBox::currentIndexChanged, this, [this](int index) {
        QStringList args = AndroidConfig::sdkManagerToolArgs();
        QString existingArg;
        for (int i = 0; i < 4; ++i) {
            const QString arg = "--channel=" + QString::number(i);
            if (args.contains(arg)) {
                existingArg = arg;
                break;
            }
        }

        if (index == 0 && !existingArg.isEmpty()) {
            args.removeAll(existingArg);
            AndroidConfig::setSdkManagerToolArgs(args);
        } else if (index > 0) {
            // Add 1 to account for Stable (second item) being channel 0
            const QString channelArg = "--channel=" + QString::number(index - 1);
            if (existingArg != channelArg) {
                if (!existingArg.isEmpty()) {
                    args.removeAll(existingArg);
                    AndroidConfig::setSdkManagerToolArgs(args);
                }
                args.append(channelArg);
                AndroidConfig::setSdkManagerToolArgs(args);
            }
       }
        m_sdkManager->reloadPackages();
    });
}

PackageFilterModel::PackageFilterModel(AndroidSdkModel *sdkModel) :
    QSortFilterProxyModel(sdkModel)
{
    setSourceModel(sdkModel);
}

void PackageFilterModel::setAcceptedPackageState(AndroidSdkPackage::PackageState state)
{
    m_packageState = state;
    invalidateFilter();
}

void PackageFilterModel::setAcceptedSearchPackage(const QString &name)
{
    m_searchText = name;
    invalidateFilter();
}

bool PackageFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex srcIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!srcIndex.isValid())
        return false;

    auto packageState = [](const QModelIndex& i) {
      return (AndroidSdkPackage::PackageState)i.data(AndroidSdkModel::PackageStateRole).toInt();
    };

    auto packageFound = [this](const QModelIndex& i) {
        return i.data(AndroidSdkModel::packageNameColumn).toString()
                .contains(m_searchText, Qt::CaseInsensitive);
    };

    bool showTopLevel = false;
    if (!sourceParent.isValid()) {
        // Top Level items
        for (int row = 0; row < sourceModel()->rowCount(srcIndex); ++row) {
            QModelIndex childIndex = sourceModel()->index(row, 0, srcIndex);
            if ((m_packageState & packageState(childIndex) && packageFound(childIndex))) {
                showTopLevel = true;
                break;
            }
        }
    }

    return showTopLevel || ((packageState(srcIndex) & m_packageState) && packageFound(srcIndex));
}

void executeAndroidSdkManagerDialog(AndroidSdkManager *sdkManager, QWidget *parent)
{
    AndroidSdkManagerDialog dialog(sdkManager, parent);
    dialog.exec();
}

} // Android::Internal
