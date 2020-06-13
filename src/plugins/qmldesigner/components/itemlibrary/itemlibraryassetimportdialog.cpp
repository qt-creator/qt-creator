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

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

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
#include <QtWidgets/qscrollarea.h>

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

static const int rowHeight = 26;

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

    QStringList importPaths;
    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (doc) {
        Model *model = doc->currentModel();
        if (model)
            importPaths = model->importPaths();
    }

    QString targetDir = defaulTargetDirectory;

    ProjectExplorer::Project *currentProject = ProjectExplorer::SessionManager::projectForFile(doc->fileName());
    if (currentProject)
        targetDir = currentProject->projectDirectory().toString();

    // Import is always done under known folder. The order of preference for folder is:
    // 1) An existing QUICK_3D_ASSETS_FOLDER under DEFAULT_ASSET_IMPORT_FOLDER project import path
    // 2) An existing QUICK_3D_ASSETS_FOLDER under any project import path
    // 3) New QUICK_3D_ASSETS_FOLDER under DEFAULT_ASSET_IMPORT_FOLDER project import path
    // 4) New QUICK_3D_ASSETS_FOLDER under any project import path
    // 5) New QUICK_3D_ASSETS_FOLDER under new DEFAULT_ASSET_IMPORT_FOLDER under project
    const QString defaultAssetFolder = QLatin1String(Constants::DEFAULT_ASSET_IMPORT_FOLDER);
    const QString quick3DFolder = QLatin1String(Constants::QUICK_3D_ASSETS_FOLDER);
    QString candidatePath = targetDir + defaultAssetFolder + quick3DFolder;
    int candidatePriority = 5;

    for (const auto &importPath : qAsConst(importPaths)) {
        if (importPath.startsWith(targetDir)) {
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

    if (!m_quick3DFiles.isEmpty()) {
        const QHash<QString, QVariantMap> allOptions = m_importer.allOptions();
        const QHash<QString, QStringList> supportedExtensions = m_importer.supportedExtensions();
        QVector<QJsonObject> groups;

        auto optIt = allOptions.constBegin();
        int optIndex = 0;
        while (optIt != allOptions.constEnd()) {
            QJsonObject options = QJsonObject::fromVariantMap(optIt.value());
            m_importOptions << options.value("options").toObject();
            groups << options.value("groups").toObject();
            const auto &exts = optIt.key().split(':');
            for (const auto &ext : exts)
                m_extToImportOptionsMap.insert(ext, optIndex);
            ++optIt;
            ++optIndex;
        }

        // Create tab for each supported extension group that also has files included in the import
        QMap<QString, int> tabMap; // QMap used for alphabetical order
        for (const auto &file : qAsConst(m_quick3DFiles)) {
            auto extIt = supportedExtensions.constBegin();
            QString ext = QFileInfo(file).suffix().toLower();
            while (extIt != supportedExtensions.constEnd()) {
                if (!tabMap.contains(extIt.key()) && extIt.value().contains(ext)) {
                    tabMap.insert(extIt.key(), m_extToImportOptionsMap.value(ext));
                    break;
                }
                ++extIt;
            }
        }

        ui->tabWidget->clear();
        auto tabIt = tabMap.constBegin();
        while (tabIt != tabMap.constEnd()) {
            createTab(tabIt.key(), tabIt.value(), groups[tabIt.value()]);
            ++tabIt;
        }

        // Pad all tabs to same height
        for (int i = 0; i < ui->tabWidget->count(); ++i) {
            auto optionsArea = qobject_cast<QScrollArea *>(ui->tabWidget->widget(i));
            if (optionsArea && optionsArea->widget()) {
                auto grid = qobject_cast<QGridLayout *>(optionsArea->widget()->layout());
                if (grid) {
                    int rows = grid->rowCount();
                    for (int j = rows; j < m_optionsRows; ++j) {
                        grid->addWidget(new QWidget(optionsArea->widget()), j, 0);
                        grid->setRowMinimumHeight(j, rowHeight);
                    }
                }
            }
        }

        ui->tabWidget->setCurrentIndex(0);
    }

    connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked,
            this, &ItemLibraryAssetImportDialog::onClose);
    connect(ui->tabWidget, &QTabWidget::currentChanged,
            this, &ItemLibraryAssetImportDialog::updateUi);

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

    QTimer::singleShot(0, [this]() {
        ui->tabWidget->setMaximumHeight(m_optionsHeight + ui->tabWidget->tabBar()->height() + 10);
        updateUi();
    });
}

ItemLibraryAssetImportDialog::~ItemLibraryAssetImportDialog()
{
    delete ui;
}

void ItemLibraryAssetImportDialog::createTab(const QString &tabLabel, int optionsIndex,
                                             const QJsonObject &groups)
{
    const int checkBoxColWidth = 18;
    const int labelMinWidth = 130;
    const int controlMinWidth = 65;
    const int columnSpacing = 16;
    int rowIndex[2] = {0, 0};

    QJsonObject &options = m_importOptions[optionsIndex];

    // First index has ungrouped widgets, rest are groups
    // First item in each real group is group label
    QVector<QVector<QPair<QWidget *, QWidget *>>> widgets;
    QHash<QString, int> groupIndexMap;
    QHash<QString, QPair<QWidget *, QWidget *>> optionToWidgetsMap;
    QHash<QString, QJsonArray> conditionMap;
    QHash<QWidget *, QWidget *> conditionalWidgetMap;
    QHash<QString, QString> optionToGroupMap;

    auto optionsArea = new QScrollArea(ui->tabWidget);
    optionsArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto optionsAreaContents = new QWidget(optionsArea);

    auto layout = new QGridLayout(optionsAreaContents);
    layout->setColumnMinimumWidth(0, checkBoxColWidth);
    layout->setColumnMinimumWidth(1, labelMinWidth);
    layout->setColumnMinimumWidth(2, controlMinWidth);
    layout->setColumnMinimumWidth(3, columnSpacing);
    layout->setColumnMinimumWidth(4, checkBoxColWidth);
    layout->setColumnMinimumWidth(5, labelMinWidth);
    layout->setColumnMinimumWidth(6, controlMinWidth);
    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 4);
    layout->setColumnStretch(2, 2);
    layout->setColumnStretch(3, 0);
    layout->setColumnStretch(4, 0);
    layout->setColumnStretch(5, 4);
    layout->setColumnStretch(6, 2);

    widgets.append(QVector<QPair<QWidget *, QWidget *>>());

    for (const auto group : groups) {
        const QString name = group.toObject().value("name").toString();
        const QJsonArray items = group.toObject().value("items").toArray();
        for (const auto item : items)
            optionToGroupMap.insert(item.toString(), name);
        auto groupLabel = new QLabel(name, optionsAreaContents);
        QFont labelFont = groupLabel->font();
        labelFont.setBold(true);
        groupLabel->setFont(labelFont);
        widgets.append({{groupLabel, nullptr}});
        groupIndexMap.insert(name, widgets.size() - 1);
    }

    const auto optKeys = options.keys();
    for (const auto &optKey : optKeys) {
        QJsonObject optObj = options.value(optKey).toObject();
        const QString optName = optObj.value("name").toString();
        const QString optDesc = optObj.value("description").toString();
        const QString optType = optObj.value("type").toString();
        QJsonObject optRange = optObj.value("range").toObject();
        QJsonValue optValue = optObj.value("value");
        QJsonArray conditions = optObj.value("conditions").toArray();

        auto *optLabel = new QLabel(optionsAreaContents);
        optLabel->setText(optName);
        optLabel->setToolTip(optDesc);

        QWidget *optControl = nullptr;
        if (optType == "Boolean") {
            auto *optCheck = new QCheckBox(optionsAreaContents);
            optCheck->setChecked(optValue.toBool());
            optControl = optCheck;
            QObject::connect(optCheck, &QCheckBox::toggled,
                             [this, optCheck, optKey, optionsIndex]() {
                QJsonObject optObj = m_importOptions[optionsIndex].value(optKey).toObject();
                QJsonValue value(optCheck->isChecked());
                optObj.insert("value", value);
                m_importOptions[optionsIndex].insert(optKey, optObj);
            });
        } else if (optType == "Real") {
            auto *optSpin = new QDoubleSpinBox(optionsAreaContents);
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
            optSpin->setMinimumWidth(controlMinWidth);
            optControl = optSpin;
            QObject::connect(optSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                             [this, optSpin, optKey, optionsIndex]() {
                QJsonObject optObj = m_importOptions[optionsIndex].value(optKey).toObject();
                QJsonValue value(optSpin->value());
                optObj.insert("value", value);
                m_importOptions[optionsIndex].insert(optKey, optObj);
            });
        } else {
            qWarning() << __FUNCTION__ << "Unsupported option type:" << optType;
            continue;
        }
        optControl->setToolTip(optDesc);

        if (!conditions.isEmpty())
            conditionMap.insert(optKey, conditions);

        const QString &groupName = optionToGroupMap.value(optKey);
        if (!groupName.isEmpty() && groupIndexMap.contains(groupName))
            widgets[groupIndexMap[groupName]].append({optLabel, optControl});
        else
            widgets[0].append({optLabel, optControl});
        optionToWidgetsMap.insert(optKey, {optLabel, optControl});
    }

    // Handle conditions
    auto it = conditionMap.constBegin();
    while (it != conditionMap.constEnd()) {
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
                    if (conditionalWidgetMap.contains(optCb))
                        conditionalWidgetMap.insert(optCb, nullptr);
                    else
                        conditionalWidgetMap.insert(optCb, conControl);
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

    // Combine options where a non-boolean option depends on a boolean option that no other
    // option depends on
    auto condIt = conditionalWidgetMap.constBegin();
    while (condIt != conditionalWidgetMap.constEnd()) {
        if (condIt.value()) {
            // Find and fix widget pairs
            for (int i = 0; i < widgets.size(); ++i) {
                auto &groupWidgets = widgets[i];
                auto widgetIt = groupWidgets.begin();
                while (widgetIt != groupWidgets.end()) {
                    if (widgetIt->second == condIt.value()
                            && !qobject_cast<QCheckBox *>(condIt.value())) {
                        if (widgetIt->first)
                            widgetIt->first->hide();
                        groupWidgets.erase(widgetIt);
                    } else {
                        ++widgetIt;
                    }
                }
                // If group was left with less than two actual members, disband the group
                // and move the remaining member to ungrouped options
                // Note: <= 2 instead of < 2 because each group has group label member
                if (i != 0 && groupWidgets.size() <= 2) {
                    widgets[0].prepend(groupWidgets[1]);
                    groupWidgets[0].first->hide(); // hide group label
                    groupWidgets.clear();
                }
            }
        }
        ++condIt;
    }

    auto incrementColIndex = [&](int col) {
        layout->setRowMinimumHeight(rowIndex[col], rowHeight);
        ++rowIndex[col];
    };

    auto insertOptionToLayout = [&](int col, const QPair<QWidget *, QWidget *> &optionWidgets) {
        layout->addWidget(optionWidgets.first, rowIndex[col], col * 4 + 1, 1, 2);
        int adj = qobject_cast<QCheckBox *>(optionWidgets.second) ? 0 : 2;
        layout->addWidget(optionWidgets.second, rowIndex[col], col * 4 + adj);
        if (!adj) {
            // Check box option may have additional conditional value field
            QWidget *condWidget = conditionalWidgetMap.value(optionWidgets.second);
            if (condWidget)
                layout->addWidget(condWidget, rowIndex[col], col * 4 + 2);
        }
        incrementColIndex(col);
    };

    if (widgets.size() == 1 && widgets[0].isEmpty()) {
        layout->addWidget(new QLabel(tr("No options available for this type."),
                                     optionsAreaContents), 0, 0, 2, 7, Qt::AlignCenter);
        incrementColIndex(0);
        incrementColIndex(0);
    }

    // Add option widgets to layout. Grouped options are added to the tops of the columns
    for (int i = 1; i < widgets.size(); ++i) {
        int col = rowIndex[1] < rowIndex[0] ? 1 : 0;
        const auto &groupWidgets = widgets[i];
        if (!groupWidgets.isEmpty()) {
            // First widget in each group is the group label
            layout->addWidget(groupWidgets[0].first, rowIndex[col], col * 4, 1, 3);
            incrementColIndex(col);
            for (int j = 1; j < groupWidgets.size(); ++j)
                insertOptionToLayout(col, groupWidgets[j]);
            // Add a separator line after each group
            auto *separator = new QFrame(optionsAreaContents);
            separator->setMaximumHeight(1);
            separator->setFrameShape(QFrame::HLine);
            separator->setFrameShadow(QFrame::Sunken);
            separator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            layout->addWidget(separator, rowIndex[col], col * 4, 1, 3);
            incrementColIndex(col);
        }
    }

    // Ungrouped options are spread evenly under the groups
    int totalRowCount = (rowIndex[0] + rowIndex[1] + widgets[0].size() + 1) / 2;
    for (const auto &rowWidgets : qAsConst(widgets[0])) {
        int col = rowIndex[0] < totalRowCount ? 0 : 1;
        insertOptionToLayout(col, rowWidgets);
    }

    int optionRows = qMax(rowIndex[0], rowIndex[1]);
    m_optionsRows = qMax(m_optionsRows, optionRows);
    m_optionsHeight = qMax(rowHeight * optionRows + 16, m_optionsHeight);
    layout->setContentsMargins(8, 8, 8, 8);
    optionsAreaContents->setContentsMargins(0, 0, 0, 0);
    optionsAreaContents->setLayout(layout);
    optionsAreaContents->setMinimumWidth(
                (checkBoxColWidth + labelMinWidth + controlMinWidth) * 2  + columnSpacing);
    optionsAreaContents->setObjectName("optionsAreaContents"); // For stylesheet

    optionsArea->setWidget(optionsAreaContents);
    optionsArea->setStyleSheet("QScrollArea {background-color: transparent}");
    optionsAreaContents->setStyleSheet(
                "QWidget#optionsAreaContents {background-color: transparent}");

    ui->tabWidget->addTab(optionsArea, tr("%1 options").arg(tabLabel));
}

void ItemLibraryAssetImportDialog::updateUi()
{
    auto optionsArea = qobject_cast<QScrollArea *>(ui->tabWidget->currentWidget());
    if (optionsArea) {
        auto optionsAreaContents = optionsArea->widget();
        int scrollBarWidth = optionsArea->verticalScrollBar()->isVisible()
                ? optionsArea->verticalScrollBar()->width() : 0;
        optionsAreaContents->resize(optionsArea->contentsRect().width()
                                    - scrollBarWidth - 8, m_optionsHeight);
    }
}

void ItemLibraryAssetImportDialog::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    updateUi();
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
                                 m_importOptions, m_extToImportOptionsMap);
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
