/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "autotestconstants.h"
#include "testcodeparser.h"
#include "testframeworkmanager.h"
#include "testsettingspage.h"
#include "testsettings.h"
#include "testtreemodel.h"

#include <coreplugin/icore.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QRegExp>

namespace Autotest {
namespace Internal {

class TestFilterDialog : public QDialog
{
public:
    explicit TestFilterDialog(QWidget *parent = nullptr, Qt::WindowFlags f = 0);
    QString filterPath() const;
    void setDetailsText(const QString &details) { m_details->setText(details); }
    void setDefaultFilterPath(const QString &defaultPath);
private:
    static bool validate(Utils::FancyLineEdit *edit, QString * /*errormessage*/);
    QLabel *m_details;
    Utils::FancyLineEdit *m_lineEdit;
    QString m_defaultPath;
};

TestFilterDialog::TestFilterDialog(QWidget *parent, Qt::WindowFlags f)
    : QDialog(parent, f),
      m_details(new QLabel),
      m_lineEdit(new Utils::FancyLineEdit)
{
    setModal(true);
    auto layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->addWidget(m_details);
    m_lineEdit->setValidationFunction(&validate);
    layout->addWidget(m_lineEdit);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto okButton = buttonBox->button(QDialogButtonBox::Ok);
    auto cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    okButton->setEnabled(false);
    layout->addWidget(buttonBox);
    setLayout(layout);
    connect(m_lineEdit, &Utils::FancyLineEdit::validChanged, okButton, &QPushButton::setEnabled);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString TestFilterDialog::filterPath() const
{
    static const QRegExp repetition("//+");
    QString path = m_lineEdit->isValid() ? m_lineEdit->text() : m_defaultPath;
    path.replace('\\', '/');         // turn windows separators into forward slashes
    path.replace(repetition, "/");   // avoid duplicated separators
    if (!path.startsWith('/'))
        path.prepend('/');
    if (!path.endsWith('/'))
        path.append('/');
    if (path.length() <= 2)          // after surrounding with '/' this should be > 2 if valid
        return QString();            // empty string marks invalid filter
    return path;
}

void TestFilterDialog::setDefaultFilterPath(const QString &defaultPath)
{
    m_lineEdit->setText(defaultPath);
    if (m_lineEdit->isValid())
        m_defaultPath = defaultPath;
    else
        m_lineEdit->setText(m_defaultPath);
}

bool TestFilterDialog::validate(Utils::FancyLineEdit *edit, QString *)
{
    if (!edit)
        return false;
    const QString &value = edit->text();
    if (value.isEmpty())
        return false;
    // should we distinguish between Windows and UNIX?
    static const QRegExp valid("(\\|/)?([^?*:;\"\'|<>\t\b]+(\\|/)?)+");
    return valid.exactMatch(value);
}
/**************************************************************************************************/

TestSettingsWidget::TestSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);

    m_ui.frameworksWarnIcon->setVisible(false);
    m_ui.frameworksWarnIcon->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui.frameworksWarn->setVisible(false);
    m_ui.frameworksWarn->setText(tr("No active test frameworks."));
    m_ui.frameworksWarn->setToolTip(tr("You will not be able to use the AutoTest plugin without "
                                       "having at least one active test framework."));
    connect(m_ui.frameworkTreeWidget, &QTreeWidget::itemChanged,
            this, &TestSettingsWidget::onFrameworkItemChanged);
    connect(m_ui.addFilter, &QPushButton::clicked, this, &TestSettingsWidget::onAddFilterClicked);
    connect(m_ui.editFilter, &QPushButton::clicked, this, &TestSettingsWidget::onEditFilterClicked);
    connect(m_ui.filterTreeWidget, &QTreeWidget::activated,
            this, &TestSettingsWidget::onEditFilterClicked);
    connect(m_ui.removeFilter, &QPushButton::clicked,
            this, &TestSettingsWidget::onRemoveFilterClicked);
    connect(m_ui.filterTreeWidget, &QTreeWidget::itemSelectionChanged, [this] () {
        const bool enable = m_ui.filterTreeWidget->selectionModel()->hasSelection();
        m_ui.editFilter->setEnabled(enable);
        m_ui.removeFilter->setEnabled(enable);
    });
}

void TestSettingsWidget::setSettings(const TestSettings &settings)
{
    m_ui.timeoutSpin->setValue(settings.timeout / 1000); // we store milliseconds
    m_ui.omitInternalMsgCB->setChecked(settings.omitInternalMssg);
    m_ui.omitRunConfigWarnCB->setChecked(settings.omitRunConfigWarn);
    m_ui.limitResultOutputCB->setChecked(settings.limitResultOutput);
    m_ui.autoScrollCB->setChecked(settings.autoScroll);
    m_ui.processArgsCB->setChecked(settings.processArgs);
    m_ui.filterGroupBox->setChecked(settings.filterScan);
    populateFrameworksListWidget(settings.frameworks);
    populateFiltersWidget(settings.whiteListFilters);
}

TestSettings TestSettingsWidget::settings() const
{
    TestSettings result;
    result.timeout = m_ui.timeoutSpin->value() * 1000; // we display seconds
    result.omitInternalMssg = m_ui.omitInternalMsgCB->isChecked();
    result.omitRunConfigWarn = m_ui.omitRunConfigWarnCB->isChecked();
    result.limitResultOutput = m_ui.limitResultOutputCB->isChecked();
    result.autoScroll = m_ui.autoScrollCB->isChecked();
    result.processArgs = m_ui.processArgsCB->isChecked();
    result.filterScan = m_ui.filterGroupBox->isChecked();
    frameworkSettings(result);
    result.whiteListFilters = filters();
    return result;
}

void TestSettingsWidget::populateFrameworksListWidget(const QHash<Core::Id, bool> &frameworks)
{
    TestFrameworkManager *frameworkManager = TestFrameworkManager::instance();
    const QList<Core::Id> &registered = frameworkManager->sortedRegisteredFrameworkIds();
    m_ui.frameworkTreeWidget->clear();
    for (const Core::Id &id : registered) {
        auto *item = new QTreeWidgetItem(m_ui.frameworkTreeWidget,
                                         QStringList(frameworkManager->frameworkNameForId(id)));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(0, frameworks.value(id) ? Qt::Checked : Qt::Unchecked);
        item->setData(0, Qt::UserRole, id.toSetting());
        item->setData(1, Qt::CheckStateRole, frameworkManager->groupingEnabled(id) ? Qt::Checked
                                                                                   : Qt::Unchecked);
        item->setToolTip(0, tr("Enable or disable test frameworks to be handled by the AutoTest "
                               "plugin."));
        QString toolTip = frameworkManager->groupingToolTip(id);
        if (toolTip.isEmpty())
            toolTip = tr("Enable or disable grouping of test cases by folder.");
        item->setToolTip(1, toolTip);
    }
}

void TestSettingsWidget::populateFiltersWidget(const QStringList &filters)
{
    for (const QString &filter : filters)
        new QTreeWidgetItem(m_ui.filterTreeWidget, {filter} );
}

void TestSettingsWidget::frameworkSettings(TestSettings &settings) const
{
    const QAbstractItemModel *model = m_ui.frameworkTreeWidget->model();
    QTC_ASSERT(model, return);
    const int itemCount = model->rowCount();
    for (int row = 0; row < itemCount; ++row) {
        QModelIndex idx = model->index(row, 0);
        const Core::Id id = Core::Id::fromSetting(idx.data(Qt::UserRole));
        settings.frameworks.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
        idx = model->index(row, 1);
        settings.frameworksGrouping.insert(id, idx.data(Qt::CheckStateRole) == Qt::Checked);
    }
}

QStringList TestSettingsWidget::filters() const
{
    QStringList result;
    if (QAbstractItemModel *model = m_ui.filterTreeWidget->model()) {
        for (int row = 0, count = model->rowCount(); row < count; ++row)
            result.append(model->index(row, 0).data().toString());
    }
    return result;
}

void TestSettingsWidget::onFrameworkItemChanged()
{
    if (QAbstractItemModel *model = m_ui.frameworkTreeWidget->model()) {
        for (int row = 0, count = model->rowCount(); row < count; ++row) {
            if (model->index(row, 0).data(Qt::CheckStateRole) == Qt::Checked) {
                m_ui.frameworksWarn->setVisible(false);
                m_ui.frameworksWarnIcon->setVisible(false);
                return;
            }
        }
    }
    m_ui.frameworksWarn->setVisible(true);
    m_ui.frameworksWarnIcon->setVisible(true);
}

void TestSettingsWidget::onAddFilterClicked()
{
    TestFilterDialog dialog;
    dialog.setWindowTitle(tr("Add Filter"));
    dialog.setDetailsText("<p>" + tr("Specify a filter expression to be added to the list of filters."
                                     "<br/>Wildcards are not supported.") + "</p>");
    if (dialog.exec() == QDialog::Accepted) {
        const QString &filter = dialog.filterPath();
        if (!filter.isEmpty())
            new QTreeWidgetItem(m_ui.filterTreeWidget, {filter} );
    }
}

void TestSettingsWidget::onEditFilterClicked()
{
    const QList<QTreeWidgetItem *> &selected = m_ui.filterTreeWidget->selectedItems();
    QTC_ASSERT(selected.size() == 1, return);
    const QString &oldFilter = selected.first()->data(0, Qt::DisplayRole).toString();

    TestFilterDialog dialog;
    dialog.setWindowTitle(tr("Edit Filter"));
    dialog.setDetailsText("<p>" + tr("Specify a filter expression that will replace \"%1\"."
                                     "<br/>Wildcards are not supported.").arg(oldFilter) + "</p>");
    dialog.setDefaultFilterPath(oldFilter);
    if (dialog.exec() == QDialog::Accepted) {
        const QString &edited = dialog.filterPath();
        if (!edited.isEmpty() && edited != oldFilter)
            selected.first()->setData(0, Qt::DisplayRole, edited);
    }
}

void TestSettingsWidget::onRemoveFilterClicked()
{
    const QList<QTreeWidgetItem *> &selected = m_ui.filterTreeWidget->selectedItems();
    QTC_ASSERT(selected.size() == 1, return);
    m_ui.filterTreeWidget->removeItemWidget(selected.first(), 0);
    delete selected.first();
}

TestSettingsPage::TestSettingsPage(const QSharedPointer<TestSettings> &settings)
    : m_settings(settings)
{
    setId("A.AutoTest.0.General");
    setDisplayName(tr("General"));
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("AutoTest", Constants::AUTOTEST_SETTINGS_TR));
    setCategoryIcon(Utils::Icon({{":/autotest/images/settingscategory_autotest.png",
                    Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint));
}

TestSettingsPage::~TestSettingsPage()
{
}

QWidget *TestSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new TestSettingsWidget;
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void TestSettingsPage::apply()
{
    if (!m_widget) // page was not shown at all
        return;
    const TestSettings newSettings = m_widget->settings();
    bool frameworkSyncNecessary = newSettings.frameworks != m_settings->frameworks;
    bool forceReparse = newSettings.filterScan != m_settings->filterScan ||
            newSettings.whiteListFilters.toSet() != m_settings->whiteListFilters.toSet();
    const QList<Core::Id> changedIds = Utils::filtered(newSettings.frameworksGrouping.keys(),
                                                       [newSettings, this] (const Core::Id &id) {
        return newSettings.frameworksGrouping[id] != m_settings->frameworksGrouping[id];
    });
    *m_settings = newSettings;
    m_settings->toSettings(Core::ICore::settings());
    TestFrameworkManager *frameworkManager = TestFrameworkManager::instance();
    frameworkManager->activateFrameworksFromSettings(m_settings);
    if (frameworkSyncNecessary)
        TestTreeModel::instance()->syncTestFrameworks();
    else if (forceReparse)
        TestTreeModel::instance()->parser()->emitUpdateTestTree();
    else if (!changedIds.isEmpty())
        TestTreeModel::instance()->rebuild(changedIds);
}

} // namespace Internal
} // namespace Autotest
