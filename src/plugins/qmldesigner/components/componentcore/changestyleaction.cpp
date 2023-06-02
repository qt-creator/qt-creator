// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "changestyleaction.h"
#include "designermcumanager.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <QComboBox>
#include <QSettings>

namespace QmlDesigner {

static QString styleConfigFileName(const QString &qmlFileName)
{
    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectManager::projectForFile(Utils::FilePath::fromString(qmlFileName));

    if (currentProject) {
        const QList<Utils::FilePath> fileNames = currentProject->files(
            ProjectExplorer::Project::SourceFiles);
        for (const Utils::FilePath &fileName : fileNames)
            if (fileName.endsWith("qtquickcontrols2.conf"))
                return fileName.toString();
    }

    return QString();
}

ChangeStyleWidgetAction::ChangeStyleWidgetAction(QObject *parent) : QWidgetAction(parent)
{
    // The Default style was renamed to Basic in Qt 6. In Qt 6, "Default"
    // will result in a platform-specific style being chosen.

    items = getAllStyleItems();
}

void ChangeStyleWidgetAction::handleModelUpdate(const QString &style)
{
    emit modelUpdated(style);
}

const QList<StyleWidgetEntry> ChangeStyleWidgetAction::styleItems() const
{
    return items;
}

QList<StyleWidgetEntry> ChangeStyleWidgetAction::getAllStyleItems()
{
    QList<StyleWidgetEntry> items = {{"Basic", "Basic", {}},
                                     {"Default", "Default", {}},
                                     {"Fusion", "Fusion", {}},
                                     {"Imagine", "Imagine", {}},
                                     {"Material Light", "Material", "Light"},
                                     {"Material Dark", "Material", "Dark"},
                                     {"Universal Light", "Universal", "Light"},
                                     {"Universal Dark", "Universal", "Dark"},
                                     {"Universal System", "Universal", "System"}};

    if (Utils::HostOsInfo::isMacHost())
        items.append({"macOS", "macOS", {}});
    if (Utils::HostOsInfo::isWindowsHost())
        items.append({"Windows", "Windows", {}});

    return items;
}

void ChangeStyleWidgetAction::changeCurrentStyle(const QString &style, const QString &qmlFileName)
{
    if (style.isEmpty())
        return;

    auto items = getAllStyleItems();

    const Utils::FilePath configFileName = Utils::FilePath::fromString(
        styleConfigFileName(qmlFileName));

    if (configFileName.exists()) {
        QSettings infiFile(configFileName.toString(), QSettings::IniFormat);

        int contains = -1;

        for (const auto &item : std::as_const(items)) {
            if (item.displayName == style) {
                contains = items.indexOf(item);
                break;
            }
        }

        if (contains >= 0) {
            const QString styleName = items.at(contains).styleName;
            const QString styleTheme = items.at(contains).styleTheme;

            infiFile.setValue("Controls/Style", styleName);

            if (!styleTheme.isEmpty())
                infiFile.setValue((styleName + "/Theme"), styleTheme);
        } else {
            infiFile.setValue("Controls/Style", style);
        }
    }
}

int ChangeStyleWidgetAction::getCurrentStyle(const QString &fileName)
{
    const QString confFileName = styleConfigFileName(fileName);

    if (Utils::FilePath::fromString(confFileName).exists()) {
        QSettings infiFile(confFileName, QSettings::IniFormat);
        const QString styleName = infiFile.value("Controls/Style", "Basic").toString();
        const QString styleTheme = infiFile.value(styleName + "/Theme", "").toString();
        const auto items = getAllStyleItems();

        int i = 0;
        for (const auto &item : items) {
            if (item.styleName == styleName && item.styleTheme == styleTheme)
                return i;
            ++i;
        }
    }

    return 0;
}

void ChangeStyleWidgetAction::handleStyleChanged(const QString &style)
{
    changeCurrentStyle(style, qmlFileName);

    if (view)
        view->resetPuppet();
}

const char enabledTooltip[] = QT_TRANSLATE_NOOP("ChangeStyleWidgetAction",
                                                "Change style for Qt Quick Controls 2.");
const char disbledTooltip[] = QT_TRANSLATE_NOOP("ChangeStyleWidgetAction",
                                                "Change style for Qt Quick Controls 2. Configuration file qtquickcontrols2.conf not found.");

QWidget *ChangeStyleWidgetAction::createWidget(QWidget *parent)
{
    auto comboBox = new QComboBox(parent);
    comboBox->setToolTip(tr(enabledTooltip));

    for (const auto &item : std::as_const(items))
        comboBox->addItem(item.displayName);

    comboBox->setEditable(true);
    comboBox->setCurrentIndex(0);

    connect(this, &ChangeStyleWidgetAction::modelUpdated, comboBox, [comboBox](const QString &style) {

        if (!comboBox)
            return;

        QSignalBlocker blocker(comboBox);

        if (style.isEmpty()) { /* The .conf file is misssing. */
            comboBox->setDisabled(true);
            comboBox->setToolTip(tr(disbledTooltip));
            comboBox->setCurrentIndex(0);
        } else if (DesignerMcuManager::instance().isMCUProject()) {
            comboBox->setDisabled(true);
            //TODO: add tooltip regarding MCU limitations, however we are behind string freeze
            comboBox->setEditText(style);
        } else {
            comboBox->setDisabled(false);
            comboBox->setToolTip(tr(enabledTooltip));
            comboBox->setEditText(style);
        }
    });

    connect(comboBox, &QComboBox::textActivated, this, &ChangeStyleWidgetAction::handleStyleChanged);

    return comboBox;
}

void ChangeStyleAction::currentContextChanged(const SelectionContext &selectionContext)
{
    AbstractView *view = selectionContext.view();
    if (view && view->model()) {
        m_action->view = view;

        const QString fileName = view->model()->fileUrl().toLocalFile();

        if (m_action->qmlFileName == fileName)
            return;

        m_action->qmlFileName = fileName;

        const QString confFileName = styleConfigFileName(fileName);

        if (Utils::FilePath::fromString(confFileName).exists()) {
            QSettings infiFile(confFileName, QSettings::IniFormat);
            const QString styleName = infiFile.value("Controls/Style", "Basic").toString();
            const QString styleTheme = infiFile.value(styleName + "/Theme", "").toString();
            const auto items = m_action->styleItems();

            QString comboBoxEntry = styleName;

            for (const auto &item : items) {
                if (item.styleName == styleName) {
                    if (!styleTheme.isEmpty() && (item.styleTheme == styleTheme)) {
                        comboBoxEntry.append(" ");
                        comboBoxEntry.append(styleTheme);

                        break;
                    }
                }
            }

            m_action->handleModelUpdate(comboBoxEntry);
        } else {
            m_action->handleModelUpdate("");
        }
    }
}

} // namespace QmlDesigner
