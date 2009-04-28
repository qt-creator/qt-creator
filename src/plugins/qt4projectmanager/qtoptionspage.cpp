#include "qtoptionspage.h"
#include "ui_showbuildlog.h"
#include "ui_qtversionmanager.h"
#include "qt4projectmanagerconstants.h"
#include "qtversionmanager.h"
#include <coreplugin/coreconstants.h>

#include <QtCore/QDir>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
///
// QtOptionsPage
///

QtOptionsPage::QtOptionsPage()
{

}

QtOptionsPage::~QtOptionsPage()
{

}

QString QtOptionsPage::id() const
{
    return QLatin1String(Constants::QTVERSION_PAGE);
}

QString QtOptionsPage::trName() const
{
    return tr(Constants::QTVERSION_PAGE);
}

QString QtOptionsPage::category() const
{
    return Constants::QT_CATEGORY;
}

QString QtOptionsPage::trCategory() const
{
    return tr(Constants::QT_CATEGORY);
}

QWidget *QtOptionsPage::createPage(QWidget *parent)
{
    QtVersionManager *vm = QtVersionManager::instance();
    m_widget = new QtOptionsPageWidget(parent, vm->versions(), vm->currentQtVersion());
    return m_widget;
}

void QtOptionsPage::apply()
{
    m_widget->finish();

    QtVersionManager *vm = QtVersionManager::instance();
    vm->setNewQtVersions(m_widget->versions(), m_widget->defaultVersion());
}

//-----------------------------------------------------
QtOptionsPageWidget::QtOptionsPageWidget(QWidget *parent, QList<QtVersion *> versions, QtVersion *defaultVersion)
    : QWidget(parent)
    , m_defaultVersion(versions.indexOf(defaultVersion))
    , m_specifyNameString(tr("<specify a name>"))
    , m_specifyPathString(tr("<specify a path>"))
{
    // Initialize m_versions
    foreach(QtVersion *version, versions) {
        m_versions.append(new QtVersion(*version));
    }


    m_ui = new Internal::Ui::QtVersionManager();
    m_ui->setupUi(this);
    m_ui->qtPath->setExpectedKind(Core::Utils::PathChooser::Directory);
    m_ui->qtPath->setPromptDialogTitle(tr("Select QTDIR"));
    m_ui->mingwPath->setExpectedKind(Core::Utils::PathChooser::Directory);
    m_ui->qtPath->setPromptDialogTitle(tr("Select the Qt Directory"));

    m_ui->addButton->setIcon(QIcon(Core::Constants::ICON_PLUS));
    m_ui->delButton->setIcon(QIcon(Core::Constants::ICON_MINUS));

    for (int i = 0; i < m_versions.count(); ++i) {
        const QtVersion * const version = m_versions.at(i);
        QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->qtdirList);
        item->setText(0, version->name());
        item->setText(1, QDir::toNativeSeparators(version->path()));
        item->setData(0, Qt::UserRole, version->uniqueId());

        if (version->isValid()) {
            if (version->hasDebuggingHelper())
                item->setData(2, Qt::DecorationRole, QIcon(":/extensionsystem/images/ok.png"));
            else
                item->setData(2, Qt::DecorationRole, QIcon(":/extensionsystem/images/error.png"));
        } else {
            item->setData(2, Qt::DecorationRole, QIcon());
        }

        m_ui->defaultCombo->addItem(version->name());
        if (i == m_defaultVersion)
            m_ui->defaultCombo->setCurrentIndex(i);
    }

    connect(m_ui->nameEdit, SIGNAL(textEdited(const QString &)),
            this, SLOT(updateCurrentQtName()));


    connect(m_ui->qtPath, SIGNAL(changed()),
            this, SLOT(updateCurrentQtPath()));
    connect(m_ui->mingwPath, SIGNAL(changed()),
            this, SLOT(updateCurrentMingwDirectory()));

    connect(m_ui->addButton, SIGNAL(clicked()),
            this, SLOT(addQtDir()));
    connect(m_ui->delButton, SIGNAL(clicked()),
            this, SLOT(removeQtDir()));

    connect(m_ui->qtPath, SIGNAL(browsingFinished()),
            this, SLOT(onQtBrowsed()));
    connect(m_ui->mingwPath, SIGNAL(browsingFinished()),
            this, SLOT(onMingwBrowsed()));

    connect(m_ui->qtdirList, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this, SLOT(versionChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
    connect(m_ui->defaultCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(defaultChanged(int)));

    connect(m_ui->msvcComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(msvcVersionChanged()));

    connect(m_ui->rebuildButton, SIGNAL(clicked()),
            this, SLOT(buildDebuggingHelper()));
    connect(m_ui->showLogButton, SIGNAL(clicked()),
            this, SLOT(showDebuggingBuildLog()));

    showEnvironmentPage(0);
    updateState();
}

void QtOptionsPageWidget::buildDebuggingHelper()
{
    // Find the qt version for this button..
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    if (!currentItem)
        return;

    int currentItemIndex = m_ui->qtdirList->indexOfTopLevelItem(currentItem);
    QtVersion *version = m_versions[currentItemIndex];

    QString result = m_versions.at(currentItemIndex)->buildDebuggingHelperLibrary();
    currentItem->setData(2, Qt::UserRole, result);

    if (version->hasDebuggingHelper()) {
        m_ui->debuggingHelperStateLabel->setPixmap(QPixmap(":/extensionsystem/images/ok.png"));
        currentItem->setData(2, Qt::DecorationRole, QIcon(":/extensionsystem/images/ok.png"));
    } else {
        m_ui->debuggingHelperStateLabel->setPixmap(QPixmap(":/extensionsystem/images/error.png"));
        currentItem->setData(2, Qt::DecorationRole, QIcon(":/extensionsystem/images/error.png"));
    }
    m_ui->showLogButton->setEnabled(true);
}

void QtOptionsPageWidget::showDebuggingBuildLog()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    if (!currentItem)
        return;

    int currentItemIndex = m_ui->qtdirList->indexOfTopLevelItem(currentItem);
    QDialog dlg;
    Ui_ShowBuildLog ui;
    ui.setupUi(&dlg);
    ui.log->setPlainText(m_ui->qtdirList->topLevelItem(currentItemIndex)->data(2, Qt::UserRole).toString());
    dlg.exec();
}

QtOptionsPageWidget::~QtOptionsPageWidget()
{
    qDeleteAll(m_versions);
    delete m_ui;
}

void QtOptionsPageWidget::addQtDir()
{
    QtVersion *newVersion = new QtVersion(m_specifyNameString, m_specifyPathString);
    m_versions.append(newVersion);

    QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->qtdirList);
    item->setText(0, newVersion->name());
    item->setText(1, QDir::toNativeSeparators(newVersion->path()));
    item->setData(0, Qt::UserRole, newVersion->uniqueId());
    item->setData(2, Qt::DecorationRole, QIcon());

    m_ui->qtdirList->setCurrentItem(item);

    m_ui->nameEdit->setText(newVersion->name());
    m_ui->qtPath->setPath(newVersion->path());
    m_ui->defaultCombo->addItem(newVersion->name());
    m_ui->nameEdit->setFocus();
    m_ui->nameEdit->selectAll();
}

void QtOptionsPageWidget::removeQtDir()
{
    QTreeWidgetItem *item = m_ui->qtdirList->currentItem();
    int index = m_ui->qtdirList->indexOfTopLevelItem(item);
    if (index < 0)
        return;

    for (int i = 0; i < m_ui->defaultCombo->count(); ++i) {
        if (m_ui->defaultCombo->itemText(i) == item->text(0)) {
            m_ui->defaultCombo->removeItem(i);
            break;
        }
    }

    delete item;

    delete m_versions.takeAt(index);
    updateState();
}

void QtOptionsPageWidget::updateState()
{
    bool enabled = (m_ui->qtdirList->currentItem() != 0);
    bool isSystemVersion = (enabled
        && m_versions.at(m_ui->qtdirList->indexOfTopLevelItem(m_ui->qtdirList->currentItem()))->isSystemVersion());
    m_ui->delButton->setEnabled(enabled && !isSystemVersion);
    m_ui->nameEdit->setEnabled(enabled && !isSystemVersion);
    m_ui->qtPath->setEnabled(enabled && !isSystemVersion);
    m_ui->mingwPath->setEnabled(enabled);

    bool hasLog = enabled && !m_ui->qtdirList->currentItem()->data(2, Qt::UserRole).toString().isEmpty();
    m_ui->showLogButton->setEnabled(hasLog);

    QtVersion *version = 0;
    if (enabled)
        version = m_versions.at(m_ui->qtdirList->indexOfTopLevelItem(m_ui->qtdirList->currentItem()));
    if (version) {
        m_ui->rebuildButton->setEnabled(version->isValid());
        if (version->hasDebuggingHelper())
            m_ui->debuggingHelperStateLabel->setPixmap(QPixmap(":/extensionsystem/images/ok.png"));
        else
            m_ui->debuggingHelperStateLabel->setPixmap(QPixmap(":/extensionsystem/images/error.png"));
    } else {
        m_ui->rebuildButton->setEnabled(false);
        m_ui->debuggingHelperStateLabel->setPixmap(QPixmap());
    }
}
void QtOptionsPageWidget::makeMingwVisible(bool visible)
{
    m_ui->mingwLabel->setVisible(visible);
    m_ui->mingwPath->setVisible(visible);
}

void QtOptionsPageWidget::showEnvironmentPage(QTreeWidgetItem *item)
{
    m_ui->msvcComboBox->setVisible(false);
    if (item) {
        int index = m_ui->qtdirList->indexOfTopLevelItem(item);
        m_ui->errorLabel->setText("");
        ProjectExplorer::ToolChain::ToolChainType t = m_versions.at(index)->toolchainType();
        if (t == ProjectExplorer::ToolChain::MinGW) {
            m_ui->msvcComboBox->setVisible(false);
            makeMingwVisible(true);
            m_ui->mingwPath->setPath(m_versions.at(index)->mingwDirectory());
        } else if (t == ProjectExplorer::ToolChain::MSVC || t == ProjectExplorer::ToolChain::WINCE){
            m_ui->msvcComboBox->setVisible(false);
            makeMingwVisible(false);
            QStringList msvcEnvironments = ProjectExplorer::ToolChain::availableMSVCVersions();
            if (msvcEnvironments.count() == 0) {
            } else if (msvcEnvironments.count() == 1) {
            } else {
                 m_ui->msvcComboBox->setVisible(true);
                 bool block = m_ui->msvcComboBox->blockSignals(true);
                 m_ui->msvcComboBox->clear();
                 foreach(const QString &msvcenv, msvcEnvironments) {
                     m_ui->msvcComboBox->addItem(msvcenv);
                     if (msvcenv == m_versions.at(index)->msvcVersion()) {
                         m_ui->msvcComboBox->setCurrentIndex(m_ui->msvcComboBox->count() - 1);
                     }
                 }
                 m_ui->msvcComboBox->blockSignals(block);
            }
        } else if (t == ProjectExplorer::ToolChain::INVALID) {
            m_ui->msvcComboBox->setVisible(false);
            makeMingwVisible(false);
            if (!m_versions.at(index)->isInstalled())
                m_ui->errorLabel->setText(tr("The Qt Version %1 is not installed. Run make install")
                                           .arg(QDir::toNativeSeparators(m_versions.at(index)->path())));
            else
                m_ui->errorLabel->setText(tr("%1 is not a valid qt directory").arg(QDir::toNativeSeparators(m_versions.at(index)->path())));
        } else { //ProjectExplorer::ToolChain::GCC
            m_ui->msvcComboBox->setVisible(false);
            makeMingwVisible(false);
            m_ui->errorLabel->setText(tr("Found Qt version %1, using mkspec %2")
                                     .arg(m_versions.at(index)->qtVersionString(),
                                          m_versions.at(index)->mkspec()));
        }
    } else {
        m_ui->msvcComboBox->setVisible(false);
        makeMingwVisible(false);
    }
}

void QtOptionsPageWidget::versionChanged(QTreeWidgetItem *item, QTreeWidgetItem *old)
{
    if (old) {
        fixQtVersionName(m_ui->qtdirList->indexOfTopLevelItem(old));
    }
    if (item) {
        m_ui->nameEdit->setText(item->text(0));
        m_ui->qtPath->setPath(item->text(1));
    } else {
        m_ui->nameEdit->clear();
        m_ui->qtPath->setPath(""); // clear()
    }
    showEnvironmentPage(item);
    updateState();
}

void QtOptionsPageWidget::onQtBrowsed()
{
    const QString dir = m_ui->qtPath->path();
    if (dir.isEmpty())
        return;

    updateCurrentQtPath();
    if (m_ui->nameEdit->text().isEmpty() || m_ui->nameEdit->text() == m_specifyNameString) {
        QStringList dirSegments = dir.split(QDir::separator(), QString::SkipEmptyParts);
        if (!dirSegments.isEmpty())
            m_ui->nameEdit->setText(dirSegments.last());
        updateCurrentQtName();
    }
    updateState();
}

void QtOptionsPageWidget::onMingwBrowsed()
{
    const QString dir = m_ui->mingwPath->path();
    if (dir.isEmpty())
        return;

    updateCurrentMingwDirectory();
    updateState();
}

void QtOptionsPageWidget::defaultChanged(int)
{
    for (int i=0; i<m_ui->defaultCombo->count(); ++i) {
        if (m_versions.at(i)->name() == m_ui->defaultCombo->currentText()) {
            m_defaultVersion = i;
            return;
        }
    }

    m_defaultVersion = 0;
}

void QtOptionsPageWidget::updateCurrentQtName()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = m_ui->qtdirList->indexOfTopLevelItem(currentItem);
    m_versions[currentItemIndex]->setName(m_ui->nameEdit->text());
    currentItem->setText(0, m_versions[currentItemIndex]->name());

    m_ui->defaultCombo->setItemText(currentItemIndex, m_versions[currentItemIndex]->name());
}


void QtOptionsPageWidget::finish()
{
    if (QTreeWidgetItem *item = m_ui->qtdirList->currentItem())
        fixQtVersionName(m_ui->qtdirList->indexOfTopLevelItem(item));
}

/* Checks that the qt version name is unique
 * and otherwise changes the name
 *
 */
void QtOptionsPageWidget::fixQtVersionName(int index)
{
    int count = m_versions.count();
    for (int i = 0; i < count; ++i) {
        if (i != index) {
            if (m_versions.at(i)->name() == m_versions.at(index)->name()) {
                // Same name, find new name
                QString name = m_versions.at(index)->name();
                QRegExp regexp("^(.*)\\((\\d)\\)$");
                if (regexp.exactMatch(name)) {
                    // Alreay in Name (#) format
                    name = regexp.cap(1) + "(" + QString().setNum(regexp.cap(2).toInt() + 1) + ")";
                } else {
                    name = name + " (2)";
                }
                // set new name
                m_versions[index]->setName(name);
                m_ui->qtdirList->topLevelItem(index)->setText(0, name);
                m_ui->defaultCombo->setItemText(index, name);

                // Now check again...
                fixQtVersionName(index);
            }
        }
    }
}

void QtOptionsPageWidget::updateCurrentQtPath()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = m_ui->qtdirList->indexOfTopLevelItem(currentItem);
    if (m_versions[currentItemIndex]->path() == m_ui->qtPath->path())
        return;
    m_versions[currentItemIndex]->setPath(m_ui->qtPath->path());
    currentItem->setText(1, QDir::toNativeSeparators(m_versions[currentItemIndex]->path()));

    showEnvironmentPage(currentItem);

    if (m_versions[currentItemIndex]->isValid()) {
        bool hasLog = !currentItem->data(2, Qt::UserRole).toString().isEmpty();
        bool hasHelper = m_versions[currentItemIndex]->hasDebuggingHelper();
        if (hasHelper) {
            currentItem->setData(2, Qt::DecorationRole, QIcon(":/extensionsystem/images/ok.png"));
            m_ui->debuggingHelperStateLabel->setPixmap(QPixmap(":/extensionsystem/images/ok.png"));
        } else {
            currentItem->setData(2, Qt::DecorationRole, QIcon(":/extensionsystem/images/error.png"));
            m_ui->debuggingHelperStateLabel->setPixmap(QPixmap(":/extensionsystem/images/error.png"));
        }
        m_ui->showLogButton->setEnabled(hasLog);
    } else {
        currentItem->setData(2, Qt::DecorationRole, QIcon());
        m_ui->debuggingHelperStateLabel->setPixmap(QPixmap());
    }
}

void QtOptionsPageWidget::updateCurrentMingwDirectory()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = m_ui->qtdirList->indexOfTopLevelItem(currentItem);
    m_versions[currentItemIndex]->setMingwDirectory(m_ui->mingwPath->path());
}

void QtOptionsPageWidget::msvcVersionChanged()
{
    const QString &msvcVersion = m_ui->msvcComboBox->currentText();
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = m_ui->qtdirList->indexOfTopLevelItem(currentItem);
    m_versions[currentItemIndex]->setMsvcVersion(msvcVersion);
}

QList<QtVersion *> QtOptionsPageWidget::versions() const
{
    return m_versions;
}

int QtOptionsPageWidget::defaultVersion() const
{
    return m_defaultVersion;
}
