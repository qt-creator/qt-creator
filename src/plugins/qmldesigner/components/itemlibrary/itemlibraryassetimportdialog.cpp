/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#include "itemlibraryassetimportdialog.h"
#include "ui_itemlibraryassetimportdialog.h"

#include "qmldesignerplugin.h"
#include "qmldesignerconstants.h"
#include "model.h"

#include "utils/outputformatter.h"
#include "theme.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qtimer.h>
#include <QtCore/qjsonarray.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qspinbox.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qtabbar.h>

namespace QmlDesigner {

namespace {

static void addFormattedMessage(Utils::OutputFormatter *formatter, const QString &str,
                                const QString &srcPath, Utils::OutputFormat format) {
    if (!formatter)
        return;
    QString msg = str;
    if (!srcPath.isEmpty())
        msg += QStringLiteral(": \"%1\"").arg(srcPath);
    msg += QLatin1Char('\n');
    formatter->appendMessage(msg, format);
    formatter->plainTextEdit()->verticalScrollBar()->setValue(
                formatter->plainTextEdit()->verticalScrollBar()->maximum());
}

}

ItemLibraryAssetImportDialog::ItemLibraryAssetImportDialog(const QStringList &importFiles,
                                     const QString &defaulTargetDirectory, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ItemLibraryAssetImportDialog),
    m_importer(this)
{
    setModal(true);
    ui->setupUi(this);

    m_outputFormatter = new Utils::OutputFormatter;
    m_outputFormatter->setPlainTextEdit(ui->plainTextEdit);

    // Skip unsupported assets
    bool skipSome = false;
    for (const auto &file : importFiles) {
        if (m_importer.isQuick3DAsset(file))
            m_quick3DFiles << file;
        else
            skipSome = true;
    }

    if (skipSome)
        addWarning("Cannot import 3D and other assets simultaneously. Skipping non-3D assets.");

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Import"));
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
            this, &ItemLibraryAssetImportDialog::onImport);

    ui->buttonBox->button(QDialogButtonBox::Close)->setDefault(true);

    // Import is always done under known folder. The order of preference for folder is:
    // 1) An existing QUICK_3D_ASSETS_FOLDER under DEFAULT_ASSET_IMPORT_FOLDER project import path
    // 2) An existing QUICK_3D_ASSETS_FOLDER under any project import path
    // 3) New QUICK_3D_ASSETS_FOLDER under DEFAULT_ASSET_IMPORT_FOLDER project import path
    // 4) New QUICK_3D_ASSETS_FOLDER under any project import path
    // 5) New QUICK_3D_ASSETS_FOLDER under new DEFAULT_ASSET_IMPORT_FOLDER under project
    const QString defaultAssetFolder = QLatin1String(Constants::DEFAULT_ASSET_IMPORT_FOLDER);
    const QString quick3DFolder = QLatin1String(Constants::QUICK_3D_ASSETS_FOLDER);
    QString candidatePath = defaulTargetDirectory + defaultAssetFolder + quick3DFolder;
    int candidatePriority = 5;
    QStringList importPaths;

    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (doc) {
        Model *model = doc->currentModel();
        if (model)
            importPaths = model->importPaths();
    }

    for (auto importPath : qAsConst(importPaths)) {
        if (importPath.startsWith(defaulTargetDirectory)) {
            const bool isDefaultFolder = importPath.endsWith(defaultAssetFolder);
            const QString assetFolder = importPath + quick3DFolder;
            const bool exists = QFileInfo(assetFolder).exists();
            if (exists) {
                if (isDefaultFolder) {
                    // Priority one location, stop looking
                    candidatePath = assetFolder;
                    break;
                } else if (candidatePriority > 2) {
                    candidatePriority = 2;
                    candidatePath = assetFolder;
                }
            } else {
                if (candidatePriority > 3 && isDefaultFolder) {
                    candidatePriority = 3;
                    candidatePath = assetFolder;
                } else if (candidatePriority > 4) {
                    candidatePriority = 4;
                    candidatePath = assetFolder;
                }
            }
        }
    }
    m_quick3DImportPath = candidatePath;

    // Create UI controls for options
    if (!importFiles.isEmpty()) {
        QJsonObject supportedOptions = QJsonObject::fromVariantMap(
                    m_importer.supportedOptions(importFiles[0]));
        m_importOptions = supportedOptions.value("options").toObject();
        const QJsonObject groups = supportedOptions.value("groups").toObject();

        const int labelWidth = 210;
        const int valueWidth = 75;
        const int groupIndent = 10;
        const int columnSpacing = 10;
        const int rowHeight = 26;
        int rowIndex[2] = {0, 0};

        // First index has ungrouped widgets, rest are groups
        // First item in each real group is group label
        QVector<QVector<QPair<QWidget *, QWidget *>>> widgets;
        QHash<QString, int> groupIndexMap;
        QHash<QString, QPair<QWidget *, QWidget *>> optionToWidgetsMap;
        QHash<QString, QJsonArray> conditionMap;
        QHash<QString, QString> optionToGroupMap;

        auto layout = new QGridLayout(ui->optionsAreaContents);
        layout->setColumnMinimumWidth(0, groupIndent);
        layout->setColumnMinimumWidth(1, labelWidth);
        layout->setColumnMinimumWidth(2, valueWidth);
        layout->setColumnMinimumWidth(3, columnSpacing);
        layout->setColumnMinimumWidth(4, groupIndent);
        layout->setColumnMinimumWidth(5, labelWidth);
        layout->setColumnMinimumWidth(6, valueWidth);

        widgets.append(QVector<QPair<QWidget *, QWidget *>>());

        for (const auto group : groups) {
            const QString name = group.toObject().value("name").toString();
            const QJsonArray items = group.toObject().value("items").toArray();
            for (const auto item : items)
                optionToGroupMap.insert(item.toString(), name);
            auto groupLabel = new QLabel(name, ui->optionsAreaContents);
            QFont labelFont = groupLabel->font();
            labelFont.setBold(true);
            groupLabel->setFont(labelFont);
            widgets.append({{groupLabel, nullptr}});
            groupIndexMap.insert(name, widgets.size() - 1);
        }

        const auto optKeys = m_importOptions.keys();
        for (const auto &optKey : optKeys) {
            QJsonObject optObj = m_importOptions.value(optKey).toObject();
            const QString optName = optObj.value("name").toString();
            const QString optDesc = optObj.value("description").toString();
            const QString optType = optObj.value("type").toString();
            QJsonObject optRange = optObj.value("range").toObject();
            QJsonValue optValue = optObj.value("value");
            QJsonArray conditions = optObj.value("conditions").toArray();

            QWidget *optControl = nullptr;
            if (optType == "Boolean") {
                auto *optCheck = new QCheckBox(ui->optionsAreaContents);
                optCheck->setChecked(optValue.toBool());
                optControl = optCheck;
                QObject::connect(optCheck, &QCheckBox::toggled, [this, optCheck, optKey]() {
                    QJsonObject optObj = m_importOptions.value(optKey).toObject();
                    QJsonValue value(optCheck->isChecked());
                    optObj.insert("value", value);
                    m_importOptions.insert(optKey, optObj);
                });
            } else if (optType == "Real") {
                auto *optSpin = new QDoubleSpinBox(ui->optionsAreaContents);
                double min = -999999999.;
                double max = 999999999.;
                double step = 1.;
                int decimals = 3;
                if (!optRange.isEmpty()) {
                    min = optRange.value("minimum").toDouble();
                    max = optRange.value("maximum").toDouble();
                    // Ensure step is reasonable for small ranges
                    double range = max - min;
                    while (range <= 10.) {
                        step /= 10.;
                        range *= 10.;
                        if (step < 0.02)
                            ++decimals;
                    }

                }
                optSpin->setRange(min, max);
                optSpin->setDecimals(decimals);
                optSpin->setValue(optValue.toDouble());
                optSpin->setSingleStep(step);
                optControl = optSpin;
                QObject::connect(optSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                                 [this, optSpin, optKey]() {
                    QJsonObject optObj = m_importOptions.value(optKey).toObject();
                    QJsonValue value(optSpin->value());
                    optObj.insert("value", value);
                    m_importOptions.insert(optKey, optObj);
                });
            } else {
                qWarning() << __FUNCTION__ << "Unsupported option type:" << optType;
                continue;
            }

            if (!conditions.isEmpty())
                conditionMap.insert(optKey, conditions);

            auto *optLabel = new QLabel(ui->optionsAreaContents);
            optLabel->setText(optName);
            optLabel->setToolTip(optDesc);
            optControl->setToolTip(optDesc);

            const QString &groupName = optionToGroupMap.value(optKey);
            if (!groupName.isEmpty() && groupIndexMap.contains(groupName))
                widgets[groupIndexMap[groupName]].append({optLabel, optControl});
            else
                widgets[0].append({optLabel, optControl});
            optionToWidgetsMap.insert(optKey, {optLabel, optControl});
        }

        // Add option widgets to layout. Grouped options are added to the tops of the columns
        for (int i = 1; i < widgets.size(); ++i) {
            int col = rowIndex[1] < rowIndex[0] ? 1 : 0;
            const auto &groupWidgets = widgets[i];
            if (!groupWidgets.isEmpty()) {
                // First widget in each group is the group label
                layout->addWidget(groupWidgets[0].first, rowIndex[col], col * 4, 1, 3);
                layout->setRowMinimumHeight(rowIndex[col],rowHeight);
                ++rowIndex[col];
                for (int j = 1; j < groupWidgets.size(); ++j) {
                    layout->addWidget(groupWidgets[j].first, rowIndex[col], col * 4 + 1);
                    layout->addWidget(groupWidgets[j].second, rowIndex[col], col * 4 + 2);
                    layout->setRowMinimumHeight(rowIndex[col],rowHeight);
                    ++rowIndex[col];
                }
            }
        }

        // Ungrouped options are spread evenly under the groups
        int totalRowCount = (rowIndex[0] + rowIndex[1] + widgets[0].size() + 1) / 2;
        for (const auto &widgets : qAsConst(widgets[0])) {
            int col = rowIndex[0] < totalRowCount ? 0 : 1;
            layout->addWidget(widgets.first, rowIndex[col], col * 4, 1, 2);
            layout->addWidget(widgets.second, rowIndex[col], col * 4 + 2);
            layout->setRowMinimumHeight(rowIndex[col],rowHeight);
            ++rowIndex[col];
        }

        // Handle conditions
        QHash<QString, QJsonArray>::const_iterator it = conditionMap.begin();
        while (it != conditionMap.end()) {
            const QString &option = it.key();
            const QJsonArray &conditions = it.value();
            const auto &conWidgets = optionToWidgetsMap.value(option);
            QWidget *conLabel = conWidgets.first;
            QWidget *conControl = conWidgets.second;
            // Currently we only support single condition per option, though the schema allows for
            // multiple, as no real life option currently has multiple conditions and connections
            // get complicated if we need to comply to multiple conditions.
            if (!conditions.isEmpty() && conLabel && conControl) {
                const auto &conObj = conditions[0].toObject();
                const QString optItem = conObj.value("property").toString();
                const auto &optWidgets = optionToWidgetsMap.value(optItem);
                const QString optMode = conObj.value("mode").toString();
                const QVariant optValue = conObj.value("value").toVariant();
                enum class Mode { equals, notEquals, greaterThan, lessThan };
                Mode mode;
                if (optMode == "NotEquals")
                    mode = Mode::notEquals;
                else if (optMode == "GreaterThan")
                    mode = Mode::greaterThan;
                else if (optMode == "LessThan")
                    mode = Mode::lessThan;
                else
                    mode = Mode::equals; // Default to equals

                if (optWidgets.first && optWidgets.second) {
                    auto optCb = qobject_cast<QCheckBox *>(optWidgets.second);
                    auto optSpin = qobject_cast<QDoubleSpinBox *>(optWidgets.second);
                    if (optCb) {
                        auto enableConditionally = [optValue](QCheckBox *cb, QWidget *w1,
                                QWidget *w2, Mode mode) {
                            bool equals = (mode == Mode::equals) == optValue.toBool();
                            bool enable = cb->isChecked() == equals;
                            w1->setEnabled(enable);
                            w2->setEnabled(enable);
                        };
                        enableConditionally(optCb, conLabel, conControl, mode);
                        QObject::connect(
                                    optCb, &QCheckBox::toggled,
                                    [optCb, conLabel, conControl, mode, enableConditionally]() {
                            enableConditionally(optCb, conLabel, conControl, mode);
                        });
                    }
                    if (optSpin) {
                        auto enableConditionally = [optValue](QDoubleSpinBox *sb, QWidget *w1,
                                QWidget *w2, Mode mode) {
                            bool enable = false;
                            double value = optValue.toDouble();
                            if (mode == Mode::equals)
                                enable = qFuzzyCompare(value, sb->value());
                            else if (mode == Mode::notEquals)
                                enable = !qFuzzyCompare(value, sb->value());
                            else if (mode == Mode::greaterThan)
                                enable = sb->value() > value;
                            else if (mode == Mode::lessThan)
                                enable = sb->value() < value;
                            w1->setEnabled(enable);
                            w2->setEnabled(enable);
                        };
                        enableConditionally(optSpin, conLabel, conControl, mode);
                        QObject::connect(
                                    optSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                                    [optSpin, conLabel, conControl, mode, enableConditionally]() {
                            enableConditionally(optSpin, conLabel, conControl, mode);
                        });
                    }
                }
            }
            ++it;
        }

        ui->optionsAreaContents->setLayout(layout);
        ui->optionsAreaContents->resize(
                    groupIndent * 2 + labelWidth * 2 + valueWidth * 2 + columnSpacing,
                    rowHeight * qMax(rowIndex[0], rowIndex[1]));
    }

    ui->optionsArea->setStyleSheet("QScrollArea {background-color: transparent}");
    ui->optionsAreaContents->setStyleSheet(
                "QWidget#optionsAreaContents {background-color: transparent}");

    connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked,
            this, &ItemLibraryAssetImportDialog::onClose);

    connect(&m_importer, &ItemLibraryAssetImporter::errorReported,
            this, &ItemLibraryAssetImportDialog::addError);
    connect(&m_importer, &ItemLibraryAssetImporter::warningReported,
            this, &ItemLibraryAssetImportDialog::addWarning);
    connect(&m_importer, &ItemLibraryAssetImporter::infoReported,
            this, &ItemLibraryAssetImportDialog::addInfo);
    connect(&m_importer, &ItemLibraryAssetImporter::importNearlyFinished,
            this, &ItemLibraryAssetImportDialog::onImportNearlyFinished);
    connect(&m_importer, &ItemLibraryAssetImporter::importFinished,
            this, &ItemLibraryAssetImportDialog::onImportFinished);
    connect(&m_importer, &ItemLibraryAssetImporter::progressChanged,
            this, &ItemLibraryAssetImportDialog::setImportProgress);

    addInfo(tr("Select import options and press \"Import\" to import the following files:"));
    for (const auto &file : m_quick3DFiles)
        addInfo(file);
}

ItemLibraryAssetImportDialog::~ItemLibraryAssetImportDialog()
{
    delete ui;
}

void ItemLibraryAssetImportDialog::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    int scrollBarHeight = ui->optionsArea->horizontalScrollBar()->isVisible()
            ? ui->optionsArea->horizontalScrollBar()->height() : 0;
    ui->tabWidget->setMaximumHeight(ui->optionsAreaContents->height() + scrollBarHeight
                                    + ui->tabWidget->tabBar()->height() + 8);
    ui->optionsArea->resize(ui->tabWidget->currentWidget()->size());
}

void ItemLibraryAssetImportDialog::setCloseButtonState(bool importing)
{
    ui->buttonBox->button(QDialogButtonBox::Close)->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::Close)->setText(importing ? tr("Cancel") : tr("Close"));
}

void ItemLibraryAssetImportDialog::addError(const QString &error, const QString &srcPath)
{
    addFormattedMessage(m_outputFormatter, error, srcPath, Utils::StdErrFormat);
}

void ItemLibraryAssetImportDialog::addWarning(const QString &warning, const QString &srcPath)
{
    addFormattedMessage(m_outputFormatter, warning, srcPath, Utils::StdOutFormat);
}

void ItemLibraryAssetImportDialog::addInfo(const QString &info, const QString &srcPath)
{
    addFormattedMessage(m_outputFormatter, info, srcPath, Utils::NormalMessageFormat);
}

void ItemLibraryAssetImportDialog::onImport()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    setCloseButtonState(true);
    ui->progressBar->setValue(0);

    if (!m_quick3DFiles.isEmpty()) {
        m_importer.importQuick3D(m_quick3DFiles, m_quick3DImportPath,
                                 m_importOptions.toVariantMap());
    }
}

void ItemLibraryAssetImportDialog::setImportProgress(int value, const QString &text)
{
    ui->progressLabel->setText(text);
    if (value < 0)
        ui->progressBar->setRange(0, 0);
    else
        ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(value);
}

void ItemLibraryAssetImportDialog::onImportNearlyFinished()
{
    // Canceling import is no longer doable
    ui->buttonBox->button(QDialogButtonBox::Close)->setEnabled(false);
}

void ItemLibraryAssetImportDialog::onImportFinished()
{
    setCloseButtonState(false);
    if (m_importer.isCancelled()) {
        QString interruptStr = tr("Import interrupted.");
        addError(interruptStr);
        setImportProgress(0, interruptStr);
    } else {
        QString doneStr = tr("Import done.");
        addInfo(doneStr);
        setImportProgress(100, doneStr);
    }
}

void ItemLibraryAssetImportDialog::onClose()
{
    if (m_importer.isImporting()) {
        addInfo(tr("Canceling import."));
        m_importer.cancelImport();
    } else {
        reject();
        close();
        deleteLater();
    }
}
}
