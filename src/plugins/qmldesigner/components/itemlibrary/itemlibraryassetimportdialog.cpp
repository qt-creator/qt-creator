// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryassetimportdialog.h"
#include "ui_itemlibraryassetimportdialog.h"

#include "import3dcanvas.h"
#include "import3dconnectionmanager.h"

#include <model.h>
#include <model/modelutils.h>
#include <nodeinstanceview.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <rewriterview.h>
#include <variantproperty.h>

#include <theme.h>
#include <utils/outputformatter.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <coreplugin/icore.h>

#include <QFileInfo>
#include <QDir>
#include <QLoggingCategory>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QScrollBar>
#include <QTabBar>
#include <QScrollArea>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>

namespace QmlDesigner {

namespace {

void addFormattedMessage(Utils::OutputFormatter *formatter,
                         const QString &str,
                         const QString &srcPath,
                         Utils::OutputFormat format)
{
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

const int rowHeight = 32;
const int checkBoxColWidth = 18;
const int labelMinWidth = 130;
const int controlMinWidth = 65;
const int columnSpacing = 16;

constexpr QStringView qdsWorkaroundsKey{u"designStudioWorkarounds"};
constexpr QStringView expandValuesKey{u"expandValueComponents"};

} // namespace

ItemLibraryAssetImportDialog::ItemLibraryAssetImportDialog(
        const QStringList &importFiles, const QString &defaulTargetDirectory,
        const QVariantMap &supportedExts, const QVariantMap &supportedOpts,
        const QJsonObject &defaultOpts, const QSet<QString> &preselectedFilesForOverwrite,
        AbstractView *view, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ItemLibraryAssetImportDialog)
    , m_view(view)
    , m_importer(this)
    , m_preselectedFilesForOverwrite(preselectedFilesForOverwrite)
{
    setModal(true);
    ui->setupUi(this);

    m_outputFormatter = new Utils::OutputFormatter;
    m_outputFormatter->setPlainTextEdit(ui->plainTextEdit);

    // Skip unsupported assets
    QHash<QString, bool> supportMap;
    for (const auto &file : importFiles) {
        QString suffix = QFileInfo(file).suffix().toLower();
        if (!supportMap.contains(suffix)) {
            bool supported = false;
            for (const auto &exts : supportedExts) {
                if (exts.toStringList().contains(suffix)) {
                    supported = true;
                    break;
                }
            }
            supportMap.insert(suffix, supported);
        }
        if (supportMap[suffix])
            m_quick3DFiles << file;
    }

    if (m_quick3DFiles.size() != importFiles.size())
        addWarning("Cannot import 3D and other assets simultaneously. Skipping non-3D assets.");

    connect(ui->importButton, &QPushButton::clicked, this, &ItemLibraryAssetImportDialog::onImport);

    ui->importButton->setDefault(true);

    ui->advancedSettingsButton->setStyleSheet(
                "QPushButton#advancedSettingsButton {background-color: transparent}");
    ui->advancedSettingsButton->setStyleSheet(
                QString("QPushButton { border: none; color :%1 }").arg(
                    Utils::creatorColor(Utils::Theme::QmlDesigner_HighlightColor).name()));

    QStringList importPaths;
    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (doc) {
        Model *model = doc->currentModel();
        if (model)
            importPaths = model->importPaths();
    }

    QString targetDir = QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath().toString();
    if (targetDir.isEmpty())
        targetDir = defaulTargetDirectory;

    m_quick3DImportPath = QmlDesignerPlugin::instance()->documentManager()
                              .generatedComponentUtils().import3dBasePath().toString();

    if (!m_quick3DFiles.isEmpty()) {
        QVector<QJsonObject> groups;

        auto optIt = supportedOpts.constBegin();
        int optIndex = 0;
        while (optIt != supportedOpts.constEnd()) {
            QJsonObject options = QJsonObject::fromVariantMap(qvariant_cast<QVariantMap>(optIt.value()));
            m_importOptions << options.value("options").toObject();
            if (m_importOptions.last().contains(qdsWorkaroundsKey)) {
                QJsonObject optObj = m_importOptions.last()[qdsWorkaroundsKey].toObject();
                optObj.insert("value", QJsonValue{true});
                m_importOptions.last().insert(qdsWorkaroundsKey, optObj);
            }
            auto it = defaultOpts.constBegin();
            while (it != defaultOpts.constEnd()) {
                if (m_importOptions.last().contains(it.key())) {
                    QJsonObject optObj = m_importOptions.last()[it.key()].toObject();
                    QJsonValue value(it.value().toObject()["value"]);
                    optObj.insert("value", value);
                    m_importOptions.last().insert(it.key(), optObj);
                }
                ++it;
            }
            groups << options.value("groups").toObject();
            const auto &exts = optIt.key().split(':');
            for (const auto &ext : exts)
                m_extToImportOptionsMap.insert(ext, optIndex);
            ++optIt;
            ++optIndex;
        }

        // Resize lists in loop for Qt5 compatibility
        for (int i = 0; i < optIndex; ++i) {
            m_simpleData.contentWidgets.append({});
            m_advancedData.contentWidgets.append({});
            m_labelToControlWidgetMaps.append(QHash<QString, QWidget *>());
        }

        // Create tab for each supported extension group that also has files included in the import
        QMap<QString, int> tabMap; // QMap used for alphabetical order
        for (const auto &file : std::as_const(m_quick3DFiles)) {
            auto extIt = supportedExts.constBegin();
            QString ext = QFileInfo(file).suffix().toLower();
            while (extIt != supportedExts.constEnd()) {
                if (!tabMap.contains(extIt.key()) && extIt.value().toStringList().contains(ext)) {
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

            auto padGrid = [](QWidget *widget, int optionRows) {
                auto grid = qobject_cast<QGridLayout *>(widget->layout());
                if (grid) {
                    int rows = grid->rowCount();
                    for (int i = rows; i <optionRows; ++i) {
                        grid->addWidget(new QWidget(widget), i, 0);
                        grid->setRowMinimumHeight(i, rowHeight);
                    }
                }
            };
            padGrid(m_simpleData.contentWidgets[tabIt.value()], m_simpleData.optionsRows);
            padGrid(m_advancedData.contentWidgets[tabIt.value()], m_advancedData.optionsRows);

            ++tabIt;
        }

        ui->tabWidget->setCurrentIndex(0);
    }

    connect(ui->closeButton, &QPushButton::clicked,
            this, &ItemLibraryAssetImportDialog::onClose);
    connect(ui->tabWidget, &QTabWidget::currentChanged,
            this, &ItemLibraryAssetImportDialog::updateUi);
    connect(canvas(), &Import3dCanvas::requestImageUpdate,
            this, &ItemLibraryAssetImportDialog::onRequestImageUpdate);
    connect(canvas(), &Import3dCanvas::requestRotation,
            this, &ItemLibraryAssetImportDialog::onRequestRotation);

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
    connect(&m_importer, &ItemLibraryAssetImporter::importReadyForPreview,
            this, &ItemLibraryAssetImportDialog::onImportReadyForPreview);

    connect(ui->advancedSettingsButton, &QPushButton::clicked,
            this, &ItemLibraryAssetImportDialog::toggleAdvanced);

    QTimer::singleShot(0, this, &ItemLibraryAssetImportDialog::updateUi);

    if (m_quick3DFiles.size() != 1) {
        addInfo(tr("Select import options and press \"Import\" to import the following files:"));
    } else {
        addInfo(tr("Importing:"));
        QTimer::singleShot(0, this, &ItemLibraryAssetImportDialog::onImport);
    }

    for (const auto &file : std::as_const(m_quick3DFiles))
        addInfo(file);

    updateImportButtonState();
}

ItemLibraryAssetImportDialog::~ItemLibraryAssetImportDialog()
{
    cleanupPreviewPuppet();
    delete ui;
}

void ItemLibraryAssetImportDialog::updateImport(AbstractView *view,
                                                const ModelNode &updateNode,
                                                const QVariantMap &supportedExts,
                                                const QVariantMap &supportedOpts)
{
    QString errorMsg;
    const ModelNode &node = updateNode;
    if (node.hasMetaInfo()) {
        QString compFileName = ModelUtils::componentFilePath(node); // absolute path
        bool preselectNodeSource = false;
        if (compFileName.isEmpty()) {
            // Node is not a file component, so we have to check if the current doc itself is
            compFileName = node.model()->fileUrl().toLocalFile();
            preselectNodeSource = true;
        }
        QFileInfo compFileInfo{compFileName};

        // Find to top asset folder
        const QString oldAssetFolder = QLatin1String(Constants::OLD_QUICK_3D_ASSETS_FOLDER);
        QString assetFolder = QLatin1String(Constants::QUICK_3D_COMPONENTS_FOLDER);
        const QStringList parts = compFileName.split('/');
        int i = parts.size() - 1;
        int previousSize = 0;
        for (; i >= 0; --i) {
            if (parts[i] == oldAssetFolder)
                assetFolder = oldAssetFolder;
            if (parts[i] == assetFolder)
                break;
            previousSize = parts[i].size();
        }
        if (i >= 0) {
            const QString assetPath = compFileName.left(compFileName.lastIndexOf(assetFolder)
                                                        + assetFolder.size() + previousSize + 1);
            const QDir assetDir(assetPath);

            // Find import options and the original source scene
            const QString jsonFileName = assetDir.absoluteFilePath(
                        Constants::QUICK_3D_ASSET_IMPORT_DATA_NAME);
            QFile jsonFile{jsonFileName};
            if (jsonFile.open(QIODevice::ReadOnly)) {
                QJsonParseError jsonError;
                const QByteArray fileData = jsonFile.readAll();
                auto jsonDocument = QJsonDocument::fromJson(fileData, &jsonError);
                jsonFile.close();
                if (jsonError.error == QJsonParseError::NoError) {
                    QJsonObject jsonObj = jsonDocument.object();
                    const QJsonObject options = jsonObj.value(
                                Constants::QUICK_3D_ASSET_IMPORT_DATA_OPTIONS_KEY).toObject();
                    QString sourcePath = jsonObj.value(
                                Constants::QUICK_3D_ASSET_IMPORT_DATA_SOURCE_KEY).toString();
                    if (options.isEmpty() || sourcePath.isEmpty()) {
                        errorMsg = QCoreApplication::translate(
                                    "ModelNodeOperations",
                                    "Asset import data file \"%1\" is invalid.").arg(jsonFileName);
                    } else {
                        QFileInfo sourceInfo{sourcePath};
                        if (!sourceInfo.exists()) {
                            // Unable to find original scene source, launch file dialog to locate it
                            QString initialPath;
                            ProjectExplorer::Project *currentProject
                                    = ProjectExplorer::ProjectManager::projectForFile(
                                        Utils::FilePath::fromString(compFileName));
                            if (currentProject)
                                initialPath = currentProject->projectDirectory().toString();
                            else
                                initialPath = compFileInfo.absolutePath();
                            QStringList selectedFiles = QFileDialog::getOpenFileNames(
                                        Core::ICore::dialogParent(),
                                        tr("Locate 3D Asset \"%1\"").arg(sourceInfo.fileName()),
                                        initialPath, sourceInfo.fileName());
                            if (!selectedFiles.isEmpty()
                                    && QFileInfo{selectedFiles[0]}.fileName() == sourceInfo.fileName()) {
                                sourcePath = selectedFiles[0];
                                sourceInfo.setFile(sourcePath);
                            }
                        }
                        if (sourceInfo.exists()) {
                            // In case of a selected node inside an imported component, preselect
                            // any file pointed to by a "source" property of the node.
                            QSet<QString> preselectedFiles;
                            if (preselectNodeSource && updateNode.hasProperty("source")) {
                                QString source = updateNode.variantProperty("source").value().toString();
                                if (QFileInfo{source}.isRelative())
                                    source = QDir{compFileInfo.absolutePath()}.absoluteFilePath(source);
                                preselectedFiles.insert(source);
                            }
                            auto importDlg = new ItemLibraryAssetImportDialog(
                                        {sourceInfo.absoluteFilePath()},
                                        node.model()->fileUrl().toLocalFile(),
                                        supportedExts, supportedOpts, options,
                                        preselectedFiles, view,
                                        Core::ICore::dialogParent());
                            importDlg->show();

                        } else {
                            errorMsg = QCoreApplication::translate(
                                        "ModelNodeOperations", "Unable to locate source scene \"%1\".")
                                    .arg(sourceInfo.fileName());
                        }
                    }
                } else {
                    errorMsg = jsonError.errorString();
                }
            } else {
                errorMsg = QCoreApplication::translate("ModelNodeOperations",
                                                       "Opening asset import data file \"%1\" failed.")
                        .arg(jsonFileName);
            }
        } else {
            errorMsg = QCoreApplication::translate("ModelNodeOperations",
                                                   "Unable to resolve asset import path.");
        }
    }

    if (!errorMsg.isEmpty()) {
        QMessageBox::warning(
                    qobject_cast<QWidget *>(Core::ICore::dialogParent()),
                    QCoreApplication::translate("ModelNodeOperations", "Import Update Failed"),
                    QCoreApplication::translate("ModelNodeOperations",
                                                "Failed to update import.\nError:\n%1").arg(errorMsg),
                    QMessageBox::Close);
    }
}

void ItemLibraryAssetImportDialog::createTab(const QString &tabLabel, int optionsIndex,
                                             const QJsonObject &groups)
{
    auto optionsArea = new QScrollArea(ui->tabWidget);
    optionsArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto optionsAreaContents = new QWidget(optionsArea);
    m_simpleData.contentWidgets[optionsIndex] = new QWidget(optionsAreaContents);
    m_advancedData.contentWidgets[optionsIndex] = new QWidget(optionsAreaContents);

    // Advanced widgets need to be set up first, as simple widgets will connect to those
    QGridLayout *advancedLayout = createOptionsGrid(m_advancedData.contentWidgets[optionsIndex], true,
                                                    optionsIndex, groups);
    QGridLayout *simpleLayout = createOptionsGrid(m_simpleData.contentWidgets[optionsIndex], false,
                                                  optionsIndex, groups);

    m_advancedData.contentWidgets[optionsIndex]->setLayout(advancedLayout);
    m_simpleData.contentWidgets[optionsIndex]->setLayout(simpleLayout);

    m_advancedData.contentWidgets[optionsIndex]->setVisible(false);

    auto layout = new QVBoxLayout(optionsAreaContents);
    layout->addWidget(m_simpleData.contentWidgets[optionsIndex]);
    layout->addWidget(m_advancedData.contentWidgets[optionsIndex]);

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

QGridLayout *ItemLibraryAssetImportDialog::createOptionsGrid(
        QWidget *contentWidget, bool advanced, int optionsIndex, const QJsonObject &groups)
{
    auto layout = new QGridLayout();
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

    // First index has ungrouped widgets, rest are groups
    // First item in each real group is group label
    QVector<QVector<QPair<QWidget *, QWidget *>>> widgets;
    QHash<QString, int> groupIndexMap;
    QHash<QString, QPair<QWidget *, QWidget *>> optionToWidgetsMap;
    QHash<QString, QJsonArray> conditionMap;
    QHash<QWidget *, QWidget *> conditionalWidgetMap;
    QHash<QString, QString> optionToGroupMap;

    int rowIndex[2] = {0, 0};

    widgets.append(QVector<QPair<QWidget *, QWidget *>>());

    const QStringList &groupIds = groups.keys();
    for (const QString &groupId : groupIds) {
        if (!advanced && !isSimpleGroup(groupId))
            continue;
        const auto &group = groups.value(groupId);
        const QString name = group.toObject().value("name").toString();
        const QJsonArray items = group.toObject().value("items").toArray();
        for (const auto &item : items)
            optionToGroupMap.insert(item.toString(), name);
        auto groupLabel = new QLabel(name, contentWidget);
        QFont labelFont = groupLabel->font();
        labelFont.setBold(true);
        groupLabel->setFont(labelFont);
        widgets.append({{groupLabel, nullptr}});
        groupIndexMap.insert(name, widgets.size() - 1);
    }

    QJsonObject &options = m_importOptions[optionsIndex];
    const auto optKeys = options.keys();
    for (const auto &optKey : optKeys) {
        if ((!advanced && !isSimpleOption(optKey)) || isHiddenOption(optKey))
            continue;
        QJsonObject optObj = options.value(optKey).toObject();
        const QString optName = optObj.value("name").toString();
        const QString optDesc = optObj.value("description").toString();
        const QString optType = optObj.value("type").toString();
        QJsonObject optRange = optObj.value("range").toObject();
        QJsonValue optValue = optObj.value("value");
        QJsonArray conditions = optObj.value("conditions").toArray();

        auto *optLabel = new QLabel(contentWidget);
        optLabel->setText(optName);
        optLabel->setToolTip(optDesc);

        QWidget *optControl = nullptr;
        if (optType == "Boolean") {
            auto *optCheck = new QCheckBox(contentWidget);
            optCheck->setChecked(optValue.toBool());
            optControl = optCheck;
            if (advanced) {
                QObject::connect(optCheck, &QCheckBox::toggled, this,
                                 [this, optCheck, optKey, optionsIndex]() {
                    QJsonObject optObj = m_importOptions[optionsIndex].value(optKey).toObject();
                    QJsonValue value(optCheck->isChecked());
                    optObj.insert("value", value);
                    m_importOptions[optionsIndex].insert(optKey, optObj);
                    updateImportButtonState();
                });
            } else {
                // Simple options also exist in advanced, so don't connect simple controls directly
                // to import options. Connect them instead to corresponding advanced controls.
                auto *advCheck = qobject_cast<QCheckBox *>(
                            m_labelToControlWidgetMaps[optionsIndex].value(optKey));
                if (advCheck) {
                    QObject::connect(optCheck, &QCheckBox::toggled, this, [this, optCheck, advCheck]() {
                        if (advCheck->isChecked() != optCheck->isChecked()) {
                            advCheck->setChecked(optCheck->isChecked());
                            updateImportButtonState();
                        }
                    });
                    QObject::connect(advCheck, &QCheckBox::toggled, this, [this, optCheck, advCheck]() {
                        if (advCheck->isChecked() != optCheck->isChecked()) {
                            optCheck->setChecked(advCheck->isChecked());
                            updateImportButtonState();
                        }
                    });
                }
            }
        } else if (optType == "Real") {
            auto *optSpin = new QDoubleSpinBox(contentWidget);
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
            if (advanced) {
                QObject::connect(optSpin, &QDoubleSpinBox::valueChanged, this,
                                 [this, optSpin, optKey, optionsIndex] {
                    QJsonObject optObj = m_importOptions[optionsIndex].value(optKey).toObject();
                    QJsonValue value(optSpin->value());
                    optObj.insert("value", value);
                    m_importOptions[optionsIndex].insert(optKey, optObj);
                    updateImportButtonState();
                });
            } else {
                auto *advSpin = qobject_cast<QDoubleSpinBox *>(
                            m_labelToControlWidgetMaps[optionsIndex].value(optKey));
                if (advSpin) {
                    // Connect corresponding advanced control
                    QObject::connect(optSpin, &QDoubleSpinBox::valueChanged,
                                     this, [this, optSpin, advSpin] {
                        if (advSpin->value() != optSpin->value()) {
                            advSpin->setValue(optSpin->value());
                            updateImportButtonState();
                        }
                    });
                    QObject::connect(advSpin, &QDoubleSpinBox::valueChanged,
                                     this, [this, optSpin, advSpin] {
                        if (advSpin->value() != optSpin->value()) {
                            optSpin->setValue(advSpin->value());
                            updateImportButtonState();
                        }
                    });
                }
            }
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
        if (advanced)
            m_labelToControlWidgetMaps[optionsIndex].insert(optKey, optControl);
    }

    // Find condition chains (up to two levels supported)
    // key: Option that has condition and is also specified in another condition as property
    // value: List of extra widgets that are affected by key property via condition
    QHash<QString, QList<QWidget *>> conditionChains;
    auto it = conditionMap.constBegin();
    while (it != conditionMap.constEnd()) {
        const QString &option = it.key();
        const QJsonArray &conditions = it.value();
        if (!conditions.isEmpty()) {
            const QString optItem = conditions[0].toObject().value("property").toString();
            if (conditionMap.contains(optItem)) {
                if (!conditionChains.contains(optItem))
                    conditionChains.insert(optItem, {});
                QPair<QWidget *, QWidget *> widgetPair = optionToWidgetsMap.value(option);
                if (widgetPair.first)
                    conditionChains[optItem].append(widgetPair.first);
                if (widgetPair.second)
                    conditionChains[optItem].append(widgetPair.second);
            }
        }
        ++it;
    }

    // Handle conditions
    it = conditionMap.constBegin();
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
                            QWidget *w2, const QList<QWidget *> &extraWidgets, Mode mode) {
                        bool equals = (mode == Mode::equals) == optValue.toBool();
                        bool enable = cb->isChecked() == equals;
                        w1->setEnabled(enable);
                        w2->setEnabled(enable);
                        if (extraWidgets.isEmpty())
                            return;

                        if (auto conditionCb = qobject_cast<QCheckBox *>(w2)) {
                            for (const auto w : extraWidgets)
                                w->setEnabled(conditionCb->isChecked() && enable);
                        }
                    };
                    // Only initialize conditional state if conditional control is enabled.
                    // If it is disabled, it is assumed that previous chained condition handling
                    // already handled this case.
                    if (optCb->isEnabled())
                        enableConditionally(optCb, conLabel, conControl, conditionChains[option], mode);
                    if (conditionalWidgetMap.contains(optCb))
                        conditionalWidgetMap.insert(optCb, nullptr);
                    else
                        conditionalWidgetMap.insert(optCb, conControl);
                    QObject::connect(
                                optCb, &QCheckBox::toggled, optCb,
                                [optCb, conLabel, conControl, extraWidgets = conditionChains[option],
                                mode, enableConditionally]() {
                        enableConditionally(optCb, conLabel, conControl, extraWidgets, mode);
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
                    QObject::connect(optSpin, &QDoubleSpinBox::valueChanged, optSpin,
                                     [optSpin, conLabel, conControl, mode, enableConditionally] {
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
                    if (groupWidgets.size() == 2)
                        widgets[0].prepend(groupWidgets[1]);
                    if (groupWidgets.size() >= 1)
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
        if (advanced) {
            layout->addWidget(new QLabel(tr("No options available for this type."),
                                         contentWidget), 0, 0, 2, 7, Qt::AlignCenter);
        } else {
            layout->addWidget(new QLabel(tr("No simple options available for this type."),
                                         contentWidget), 0, 0, 2, 7, Qt::AlignCenter);
        }
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
            auto *separator = new QFrame(contentWidget);
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
    for (const auto &rowWidgets : std::as_const(widgets[0])) {
        int col = rowIndex[0] < totalRowCount ? 0 : 1;
        insertOptionToLayout(col, rowWidgets);
    }

    int optionRows = qMax(rowIndex[0], rowIndex[1]);
    int &globalOptionRows = advanced ? m_advancedData.optionsRows : m_simpleData.optionsRows;
    int &globalOptionsHeight = advanced ? m_advancedData.optionsHeight : m_simpleData.optionsHeight;
    globalOptionRows = qMax(globalOptionRows, optionRows);
    globalOptionsHeight = qMax(rowHeight * optionRows + 20, globalOptionsHeight);
    layout->setContentsMargins(8, 6, 8, 0);

    return layout;
}

void ItemLibraryAssetImportDialog::updateUi()
{
    auto optionsArea = qobject_cast<QScrollArea *>(ui->tabWidget->currentWidget());
    if (optionsArea) {
        int optionsHeight = m_advancedMode ? m_advancedData.optionsHeight
                                           : m_simpleData.optionsHeight;

        ui->tabWidget->setMaximumHeight(optionsHeight + ui->tabWidget->tabBar()->height() + 10);
        auto optionsAreaContents = optionsArea->widget();
        int scrollBarWidth = optionsArea->verticalScrollBar()->isVisible()
                ? optionsArea->verticalScrollBar()->width() : 0;

        optionsAreaContents->resize(optionsArea->contentsRect().width() - scrollBarWidth - 8,
                                    optionsHeight);

        resize(width(), m_dialogHeight);
    }
}

bool ItemLibraryAssetImportDialog::isSimpleGroup(const QString &id)
{
    static QStringList simpleGroups {
        "globalScale"
    };

    return simpleGroups.contains(id);
}

bool ItemLibraryAssetImportDialog::isSimpleOption(const QString &id)
{
    static QStringList simpleOptions {
        "globalScale",
        "globalScaleValue"
    };

    return simpleOptions.contains(id);
}

bool ItemLibraryAssetImportDialog::isHiddenOption(const QString &id)
{
    static QList<QStringView> hiddenOptions {
        qdsWorkaroundsKey,
        expandValuesKey // Hidden because qdsWorkaroundsKey we force true implies this
    };

    return hiddenOptions.contains(id);
}

void ItemLibraryAssetImportDialog::startPreview()
{
    cleanupPreviewPuppet();

    // Preview is done via custom QML file added into the temporary folder of the preview
    QString previewQml =
R"(
import QtQuick
import QtQuick3D

Rectangle {
    id: root
    width: %1
    height: %2

    property alias sceneNode: sceneNode
    property alias view3d: view3d
    property string extents
    property string sceneModelName: "%3"

    gradient: Gradient {
        GradientStop { position: 1.0; color: "#222222" }
        GradientStop { position: 0.0; color: "#999999" }
    }

    View3D {
        id: view3d
        anchors.fill: parent
        camera: viewCamera

        environment: SceneEnvironment {
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.VeryHigh
        }

        PerspectiveCamera {
            id: viewCamera
            x: 600
            y: 600
            z: 600
            eulerRotation.x: -45
            eulerRotation.y: -45
            clipFar: 100000
            clipNear: 10
        }

        DirectionalLight {
            rotation: viewCamera.rotation
        }

        Node {
            id: sceneNode
        }
    }

    Text {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        color: "white"
        text: root.extents
        font.pixelSize: 14
    }
}
)";

    QSize size = canvas()->size();
    previewQml = previewQml.arg(size.width()).arg(size.height()).arg(m_previewCompName);

    m_previewFile.writeFileContents(previewQml.toUtf8());

    if (!m_previewFile.exists()) {
        addWarning("Failed to write preview file.");
        return;
    }

    m_connectionManager = new Import3dConnectionManager;
    m_rewriterView = new RewriterView{m_view->externalDependencies(), RewriterView::Amend};
    m_nodeInstanceView = new NodeInstanceView{*m_connectionManager, m_view->externalDependencies()};

#ifdef QDS_USE_PROJECTSTORAGE
    m_model = m_view->model()->createModel("Item");
#else
    m_model = QmlDesigner::Model::create("QtQuick/Item", 2, 1);
    m_model->setFileUrl(m_previewFile.toUrl());
#endif

    auto textDocument = std::make_unique<QTextDocument>(previewQml);
    auto modifier = std::make_unique<NotIndentingTextEditModifier>(textDocument.get(),
                                                                   QTextCursor{textDocument.get()});
    m_rewriterView->setTextModifier(modifier.get());
    m_model->setRewriterView(m_rewriterView);

    if (!m_rewriterView->errors().isEmpty()) {
        addWarning("Preview scene creation failed.");
        cleanupPreviewPuppet();
        return;
    }

    m_nodeInstanceView->setTarget(m_view->nodeInstanceView()->target());

    auto previewImageCallback = [this](const QImage &image) {
        canvas()->updateRenderImage(image);
    };

    auto crashCallback = [&] {
        addWarning("Preview process crashed.");
        cleanupPreviewPuppet();
    };

    m_connectionManager->setPreviewImageCallback(std::move(previewImageCallback));
    m_nodeInstanceView->setCrashCallback(std::move(crashCallback));

    m_model->setNodeInstanceView(m_nodeInstanceView);
}

void ItemLibraryAssetImportDialog::cleanupPreviewPuppet()
{
    if (m_model) {
        m_model->setNodeInstanceView({});
        m_model->setRewriterView({});
        m_model.reset();
    }

    if (m_nodeInstanceView)
        m_nodeInstanceView->setCrashCallback({});

    if (m_connectionManager)
        m_connectionManager->setPreviewImageCallback({});

    delete m_rewriterView;
    delete m_nodeInstanceView;
    delete m_connectionManager;
}

Import3dCanvas *ItemLibraryAssetImportDialog::canvas()
{
    return ui->import3dcanvas;
}

void ItemLibraryAssetImportDialog::resizeEvent(QResizeEvent *event)
{
    m_dialogHeight = event->size().height();
    updateUi();
}

void ItemLibraryAssetImportDialog::setCloseButtonState(bool importing)
{
    ui->closeButton->setEnabled(true);
    ui->closeButton->setText(importing ? tr("Cancel") : tr("Close"));
}

void ItemLibraryAssetImportDialog::updateImportButtonState()
{
    ui->importButton->setText(m_previewOptions == m_importOptions ? tr("Accept") : tr("Import"));
}

void ItemLibraryAssetImportDialog::addError(const QString &error, const QString &srcPath)
{
    m_closeOnFinish = false;
    addFormattedMessage(m_outputFormatter, error, srcPath, Utils::StdErrFormat);
}

void ItemLibraryAssetImportDialog::addWarning(const QString &warning, const QString &srcPath)
{
    m_closeOnFinish = false;
    addFormattedMessage(m_outputFormatter, warning, srcPath, Utils::StdOutFormat);
}

void ItemLibraryAssetImportDialog::addInfo(const QString &info, const QString &srcPath)
{
    addFormattedMessage(m_outputFormatter, info, srcPath, Utils::NormalMessageFormat);
}

void ItemLibraryAssetImportDialog::onImport()
{
    ui->importButton->setEnabled(false);

    if (!m_previewCompName.isEmpty() && m_previewOptions == m_importOptions) {
        cleanupPreviewPuppet();
        m_importer.finalizeQuick3DImport();
        return;
    }

    setCloseButtonState(true);
    ui->progressBar->setValue(0);

    if (!m_quick3DFiles.isEmpty()) {
        if (!m_previewCompName.isEmpty()) {
            m_importer.reImportQuick3D(m_previewCompName, m_importOptions);
        } else {
            m_importer.importQuick3D(m_quick3DFiles, m_quick3DImportPath,
                                     m_importOptions, m_extToImportOptionsMap,
                                     m_preselectedFilesForOverwrite);
        }
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

void ItemLibraryAssetImportDialog::onImportReadyForPreview(const QString &path, const QString &compName)
{
    addInfo(tr("Import is ready for preview."));
    if (m_previewCompName.isEmpty())
        addInfo(tr("Click \"Accept\" to finish the import or adjust options and click \"Import\" to import again."));

    m_previewFile = Utils::FilePath::fromString(path).pathAppended(m_importer.previewFileName());
    m_previewCompName = compName;
    m_previewOptions = m_importOptions;
    QTimer::singleShot(0, this, &ItemLibraryAssetImportDialog::startPreview);

    ui->importButton->setEnabled(true);
    updateImportButtonState();
}

void ItemLibraryAssetImportDialog::onRequestImageUpdate()
{
    if (m_nodeInstanceView)
        m_nodeInstanceView->view3DAction(View3DActionType::Import3dUpdatePreviewImage, canvas()->size());
}

void ItemLibraryAssetImportDialog::onRequestRotation(const QPointF &delta)
{
    if (m_nodeInstanceView)
        m_nodeInstanceView->view3DAction(View3DActionType::Import3dRotatePreviewModel, delta);
}

void ItemLibraryAssetImportDialog::onImportNearlyFinished()
{
    // Canceling import is no longer doable
    ui->closeButton->setEnabled(false);
}

void ItemLibraryAssetImportDialog::onImportFinished()
{
    setCloseButtonState(false);
    if (m_importer.isCancelled()) {
        QString interruptStr = tr("Import interrupted.");
        addError(interruptStr);
        setImportProgress(0, interruptStr);
        if (m_explicitClose)
            QTimer::singleShot(1000, this, &ItemLibraryAssetImportDialog::doClose);
    } else {
        QString doneStr = tr("Import done.");
        addInfo(doneStr);
        setImportProgress(100, doneStr);
        if (m_closeOnFinish) {
            // Add small delay to allow user to visually confirm import finishing
            QTimer::singleShot(1000, this, &ItemLibraryAssetImportDialog::doClose);
        }
    }
}

void ItemLibraryAssetImportDialog::onClose()
{
    ui->importButton->setEnabled(false);
    m_explicitClose = true;
    doClose();
}

void ItemLibraryAssetImportDialog::doClose()
{
    if (m_importer.isImporting()) {
        addInfo(tr("Canceling import."));
        m_importer.cancelImport();
    } else if (isVisible()) {
        if (ui->progressBar->value() == 100) // import done successfully
            accept();
        else
            reject();
        close();
        deleteLater();
    }
}

void ItemLibraryAssetImportDialog::toggleAdvanced()
{
    m_advancedMode = !m_advancedMode;
    for (const auto &widget : std::as_const(m_simpleData.contentWidgets)) {
        if (widget)
            widget->setVisible(!m_advancedMode);
    }
    for (const auto &widget : std::as_const(m_advancedData.contentWidgets)) {
        if (widget)
            widget->setVisible(m_advancedMode);
    }

    if (m_advancedMode)
        ui->advancedSettingsButton->setText(tr("Hide Advanced Options"));
    else
        ui->advancedSettingsButton->setText(tr("Show All Options"));

    int diff = qMin(300, m_advancedData.optionsHeight - m_simpleData.optionsHeight);
    m_dialogHeight = qMax(350, m_dialogHeight + (m_advancedMode ? diff : -diff));

    updateUi();
}

}
