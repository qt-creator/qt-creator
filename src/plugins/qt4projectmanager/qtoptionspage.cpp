#include "qtoptionspage.h"
#include "ui_showbuildlog.h"
#include "ui_qtversionmanager.h"
#include "qt4projectmanagerconstants.h"
#include "qtversionmanager.h"

#include <projectexplorer/debugginghelper.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/treewidgetcolumnstretcher.h>
#include <utils/qtcassert.h>
#include <qtconcurrent/runextensions.h>

#include <QtCore/QFuture>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtGui/QHelpEvent>
#include <QtGui/QToolTip>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

///
// DebuggingHelperBuildTask
///

DebuggingHelperBuildTask::DebuggingHelperBuildTask(const QSharedPointerQtVersion &version) :
    m_version(version)
{
}

DebuggingHelperBuildTask::~DebuggingHelperBuildTask()
{
}

void DebuggingHelperBuildTask::run(QFutureInterface<void> &future)
{
    future.setProgressRange(0, 4);
    future.setProgressValue(1);
    const QString output = m_version->buildDebuggingHelperLibrary();
    future.setProgressValue(1);
    emit finished(m_version->name(), output);
    deleteLater();
}

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
    m_widget = new QtOptionsPageWidget(parent, vm->versions(), vm->defaultVersion());
    return m_widget;
}

void QtOptionsPage::apply()
{
    m_widget->finish();

    QtVersionManager *vm = QtVersionManager::instance();
    // Turn into flat list
    QList<QtVersion *> versions;
    foreach(const QSharedPointerQtVersion &spv, m_widget->versions())
        versions.push_back(new QtVersion(*spv));
    vm->setNewQtVersions(versions, m_widget->defaultVersion());
}

//-----------------------------------------------------


QtOptionsPageWidget::QtOptionsPageWidget(QWidget *parent, QList<QtVersion *> versions, QtVersion *defaultVersion)
    : QWidget(parent)
    , m_debuggingHelperOkPixmap(QLatin1String(":/extensionsystem/images/ok.png"))
    , m_debuggingHelperErrorPixmap(QLatin1String(":/extensionsystem/images/error.png"))
    , m_debuggingHelperOkIcon(m_debuggingHelperOkPixmap)
    , m_debuggingHelperErrorIcon(m_debuggingHelperErrorPixmap)
    , m_specifyNameString(tr("<specify a name>"))
    , m_specifyPathString(tr("<specify a qmake location>"))
    , m_ui(new Internal::Ui::QtVersionManager())
    , m_defaultVersion(versions.indexOf(defaultVersion))
{
    // Initialize m_versions
    foreach(QtVersion *version, versions)
        m_versions.push_back(QSharedPointerQtVersion(new QtVersion(*version)));

    m_ui->setupUi(this);
    m_ui->qmakePath->setExpectedKind(Utils::PathChooser::File);
    m_ui->qmakePath->setPromptDialogTitle(tr("Select QMake Executable"));
    m_ui->mingwPath->setExpectedKind(Utils::PathChooser::Directory);
    m_ui->mingwPath->setPromptDialogTitle(tr("Select the MinGW Directory"));
    m_ui->mwcPath->setExpectedKind(Utils::PathChooser::Directory);
    m_ui->mwcPath->setPromptDialogTitle(tr("Select Carbide Install Directory"));
    m_ui->s60SDKPath->setExpectedKind(Utils::PathChooser::Directory);
    m_ui->s60SDKPath->setPromptDialogTitle(tr("Select S60 SDK Root"));
    m_ui->gccePath->setExpectedKind(Utils::PathChooser::Directory);
    m_ui->gccePath->setPromptDialogTitle(tr("Select the CSL ARM Toolchain (GCCE) Directory"));

    m_ui->addButton->setIcon(QIcon(Core::Constants::ICON_PLUS));
    m_ui->delButton->setIcon(QIcon(Core::Constants::ICON_MINUS));

    new Utils::TreeWidgetColumnStretcher(m_ui->qtdirList, 1);

    // setup parent items for auto-detected and manual versions
    m_ui->qtdirList->header()->setResizeMode(QHeaderView::ResizeToContents);
    QTreeWidgetItem *autoItem = new QTreeWidgetItem(m_ui->qtdirList);
    m_ui->qtdirList->installEventFilter(this);
    autoItem->setText(0, tr("Auto-detected"));
    autoItem->setFirstColumnSpanned(true);
    QTreeWidgetItem *manualItem = new QTreeWidgetItem(m_ui->qtdirList);
    manualItem->setText(0, tr("Manual"));
    manualItem->setFirstColumnSpanned(true);

    for (int i = 0; i < m_versions.count(); ++i) {
        const QtVersion * const version = m_versions.at(i).data();
        QTreeWidgetItem *item = new QTreeWidgetItem(version->isAutodetected()? autoItem : manualItem);
        item->setText(0, version->name());
        item->setText(1, QDir::toNativeSeparators(version->qmakeCommand()));
        item->setData(0, Qt::UserRole, version->uniqueId());

        if (version->isValid()) {
            item->setData(2, Qt::DecorationRole, version->hasDebuggingHelper() ? m_debuggingHelperOkIcon : m_debuggingHelperErrorIcon);
        } else {
            item->setData(2, Qt::DecorationRole, QIcon());
        }

        m_ui->defaultCombo->addItem(version->name());
        if (i == m_defaultVersion)
            m_ui->defaultCombo->setCurrentIndex(i);
    }
    m_ui->qtdirList->expandAll();

    connect(m_ui->nameEdit, SIGNAL(textEdited(const QString &)),
            this, SLOT(updateCurrentQtName()));


    connect(m_ui->qmakePath, SIGNAL(changed(QString)),
            this, SLOT(updateCurrentQMakeLocation()));
    connect(m_ui->mingwPath, SIGNAL(changed(QString)),
            this, SLOT(updateCurrentMingwDirectory()));
#ifdef QTCREATOR_WITH_S60
    connect(m_ui->mwcPath, SIGNAL(changed(QString)),
            this, SLOT(updateCurrentMwcDirectory()));
    connect(m_ui->s60SDKPath, SIGNAL(changed(QString)),
            this, SLOT(updateCurrentS60SDKDirectory()));
    connect(m_ui->gccePath, SIGNAL(changed(QString)),
            this, SLOT(updateCurrentGcceDirectory()));
#endif

    connect(m_ui->addButton, SIGNAL(clicked()),
            this, SLOT(addQtDir()));
    connect(m_ui->delButton, SIGNAL(clicked()),
            this, SLOT(removeQtDir()));

    connect(m_ui->qmakePath, SIGNAL(browsingFinished()),
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

bool QtOptionsPageWidget::eventFilter(QObject *o, QEvent *e)
{
    // Set the items tooltip, which may cause costly initialization
    // of QtVersion and must be up-to-date
    if (o != m_ui->qtdirList || e->type() != QEvent::ToolTip)
        return false;    
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);
    const QPoint treePos = helpEvent->pos() - QPoint(0, m_ui->qtdirList->header()->height());
    QTreeWidgetItem *item = m_ui->qtdirList->itemAt(treePos);
    if (!item)
        return false;
    const int index = indexForTreeItem(item);
    if (index == -1)
        return false;
    const QString tooltip = m_versions.at(index)->toHtml();
    QToolTip::showText(helpEvent->globalPos(), tooltip, m_ui->qtdirList);
    helpEvent->accept();
    return true;
}

int QtOptionsPageWidget::currentIndex() const
{
    if (QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem())
        return indexForTreeItem(currentItem);
    return -1;
}

QtVersion *QtOptionsPageWidget::currentVersion() const
{
    const int currentItemIndex = currentIndex();
    if (currentItemIndex >= 0 && currentItemIndex < m_versions.size())
        return m_versions.at(currentItemIndex).data();
    return 0;
}

static inline int findVersionByName(const QList<QSharedPointerQtVersion> &l, const QString &name)
{
    const int size = l.size();
    for (int i = 0; i < size; i++)
        if (l.at(i)->name() == name)
            return i;
    return -1;
}

// Update with results of terminated helper build
void QtOptionsPageWidget::debuggingHelperBuildFinished(const QString &name, const QString &output)
{
    const int index = findVersionByName(m_versions, name);
    if (index == -1)
        return; // Oops, somebody managed to delete the version
    // Update item view
    QTreeWidgetItem *item = treeItemForIndex(index);
    QTC_ASSERT(item, return)
    item->setData(2, Qt::UserRole, output);
    const bool success = m_versions.at(index)->hasDebuggingHelper();
    item->setData(2, Qt::DecorationRole, success ? m_debuggingHelperOkIcon : m_debuggingHelperErrorIcon);

    // Update bottom control if the selection is still the same
    if (index == currentIndex()) {
        m_ui->showLogButton->setEnabled(true);
        updateDebuggingHelperStateLabel(m_versions.at(index).data());
        if (!success)
            showDebuggingBuildLog();
    }
}

void QtOptionsPageWidget::buildDebuggingHelper()
{
    const int index = currentIndex();
    if (index < 0)
        return;

    m_ui->showLogButton->setEnabled(false);
    // Run a debugging helper build task in the background.
    DebuggingHelperBuildTask *buildTask = new DebuggingHelperBuildTask(m_versions.at(index));
    connect(buildTask, SIGNAL(finished(QString,QString)), this, SLOT(debuggingHelperBuildFinished(QString,QString)),
            Qt::QueuedConnection);
    QFuture<void> task = QtConcurrent::run(&DebuggingHelperBuildTask::run, buildTask);
    const QString taskName = tr("Building helpers");
    Core::ICore::instance()->progressManager()->addTask(task, taskName,
                                                        QLatin1String("Qt4ProjectManager::BuildHelpers"),
                                                        Core::ProgressManager::CloseOnSuccess);
}

void QtOptionsPageWidget::showDebuggingBuildLog()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();

    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    // Show and scroll to bottom
    QDialog dlg;
    Ui_ShowBuildLog ui;
    ui.setupUi(&dlg);
    ui.log->setPlainText(currentItem->data(2, Qt::UserRole).toString());
    ui.log->moveCursor(QTextCursor::End);
    ui.log->ensureCursorVisible();
    dlg.exec();
}

QtOptionsPageWidget::~QtOptionsPageWidget()
{
    delete m_ui;
}

void QtOptionsPageWidget::addQtDir()
{
    QSharedPointerQtVersion newVersion(new QtVersion(m_specifyNameString, m_specifyPathString));
    m_versions.append(newVersion);

    QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->qtdirList->topLevelItem(1));
    item->setText(0, newVersion->name());
    item->setText(1, QDir::toNativeSeparators(newVersion->qmakeCommand()));
    item->setData(0, Qt::UserRole, newVersion->uniqueId());
    item->setData(2, Qt::DecorationRole, QIcon());

    m_ui->qtdirList->setCurrentItem(item);

    m_ui->nameEdit->setText(newVersion->name());
    m_ui->qmakePath->setPath(newVersion->qmakeCommand());
    m_ui->defaultCombo->addItem(newVersion->name());
    m_ui->nameEdit->setFocus();
    m_ui->nameEdit->selectAll();

    if (!m_versions.at(m_defaultVersion)->isValid()) {
        m_defaultVersion = m_versions.count() - 1;
        m_ui->defaultCombo->setCurrentIndex(m_versions.count() - 1);
    }
}

void QtOptionsPageWidget::removeQtDir()
{
    QTreeWidgetItem *item = m_ui->qtdirList->currentItem();
    int index = indexForTreeItem(item);
    if (index < 0)
        return;

    for (int i = 0; i < m_ui->defaultCombo->count(); ++i) {
        if (m_ui->defaultCombo->itemText(i) == item->text(0)) {
            m_ui->defaultCombo->removeItem(i);
            break;
        }
    }

    delete item;

    m_versions.removeAt(index);
    updateState();
}

// Format html table tooltip about helpers
static inline QString msgHtmlHelperToolTip(const QFileInfo &fi)
{
    //: Tooltip showing the debugging helper library file.
    return QtOptionsPageWidget::tr("<html><body><table><tr><td>File:</td><td><pre>%1</pre></td></tr>"
                                   "<tr><td>Last&nbsp;modified:</td><td>%2</td></tr>"
                                   "<tr><td>Size:</td><td>%3 Bytes</td></tr></table></body></html>").
                      arg(QDir::toNativeSeparators(fi.absoluteFilePath())).
                      arg(fi.lastModified().toString(Qt::SystemLocaleLongDate)).
                      arg(fi.size());
}

// Update the state label with a pixmap and set a tooltip describing
// the file on neighbouring controls.
void QtOptionsPageWidget::updateDebuggingHelperStateLabel(const QtVersion *version)
{
    QString tooltip;
    if (version && version->isValid()) {
        const bool hasHelper = version->hasDebuggingHelper();
        m_ui->debuggingHelperStateLabel->setPixmap(hasHelper ? m_debuggingHelperOkPixmap : m_debuggingHelperErrorPixmap);
        if (hasHelper)
            tooltip = msgHtmlHelperToolTip(QFileInfo(version->debuggingHelperLibrary()));
    } else {
        m_ui->debuggingHelperStateLabel->setPixmap(QPixmap());
    }
    m_ui->debuggingHelperStateLabel->setToolTip(tooltip);
    m_ui->debuggingHelperLabel->setToolTip(tooltip);
    m_ui->showLogButton->setToolTip(tooltip);
    m_ui->rebuildButton->setToolTip(tooltip);
}

void QtOptionsPageWidget::updateState()
{
    const QtVersion *version  = currentVersion();
    const bool enabled = version != 0;
    const bool isAutodetected = enabled && version->isAutodetected();
    m_ui->delButton->setEnabled(enabled && !isAutodetected);
    m_ui->nameEdit->setEnabled(enabled && !isAutodetected);
    m_ui->qmakePath->setEnabled(enabled && !isAutodetected);
    m_ui->mingwPath->setEnabled(enabled);
    m_ui->mwcPath->setEnabled(enabled);
    bool s60SDKPathEnabled = enabled &&
                             (isAutodetected ? version->s60SDKDirectory().isEmpty() : true);
    m_ui->s60SDKPath->setEnabled(s60SDKPathEnabled);
    m_ui->gccePath->setEnabled(enabled);

    const bool hasLog = enabled && !m_ui->qtdirList->currentItem()->data(2, Qt::UserRole).toString().isEmpty();
    m_ui->showLogButton->setEnabled(hasLog);

    m_ui->rebuildButton->setEnabled(version && version->isValid());
    updateDebuggingHelperStateLabel(version);
}

void QtOptionsPageWidget::makeMingwVisible(bool visible)
{
    m_ui->mingwLabel->setVisible(visible);
    m_ui->mingwPath->setVisible(visible);
}

void QtOptionsPageWidget::makeMSVCVisible(bool visible)
{
    m_ui->msvcLabel->setVisible(visible);
    m_ui->msvcComboBox->setVisible(visible);
    m_ui->msvcNotFoundLabel->setVisible(false);
}

void QtOptionsPageWidget::makeS60Visible(bool visible)
{
    m_ui->mwcLabel->setVisible(visible);
    m_ui->mwcPath->setVisible(visible);
    m_ui->s60SDKLabel->setVisible(visible);
    m_ui->s60SDKPath->setVisible(visible);
    m_ui->gcceLabel->setVisible(visible);
    m_ui->gccePath->setVisible(visible);
}

void QtOptionsPageWidget::showEnvironmentPage(QTreeWidgetItem *item)
{
    if (item) {
        int index = indexForTreeItem(item);
        if (index < 0) {
            makeMSVCVisible(false);
            makeMingwVisible(false);
            makeS60Visible(false);
            return;
        }
        m_ui->errorLabel->setText("");
        QList<ProjectExplorer::ToolChain::ToolChainType> types = m_versions.at(index)->possibleToolChainTypes();
        if (types.contains(ProjectExplorer::ToolChain::MinGW)) {
            makeMSVCVisible(false);
            makeMingwVisible(true);
            makeS60Visible(false);
            m_ui->mingwPath->setPath(m_versions.at(index)->mingwDirectory());
        } else if (types.contains(ProjectExplorer::ToolChain::MSVC) || types.contains(ProjectExplorer::ToolChain::WINCE)){
            makeMSVCVisible(false);
            makeMingwVisible(false);
            makeS60Visible(false);
            QStringList msvcEnvironments = ProjectExplorer::ToolChain::availableMSVCVersions();
            if (msvcEnvironments.count() == 0) {
                m_ui->msvcLabel->setVisible(true);
                m_ui->msvcNotFoundLabel->setVisible(true);
            } else {
                 makeMSVCVisible(true);
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
#ifdef QTCREATOR_WITH_S60
        } else if (types.contains(ProjectExplorer::ToolChain::WINSCW)
                || types.contains(ProjectExplorer::ToolChain::RVCT_ARMV5)
                || types.contains(ProjectExplorer::ToolChain::RVCT_ARMV6)
                || types.contains(ProjectExplorer::ToolChain::GCCE)) {
            makeMSVCVisible(false);
            makeMingwVisible(false);
            makeS60Visible(true);
            m_ui->mwcPath->setPath(m_versions.at(index)->mwcDirectory());
            m_ui->s60SDKPath->setPath(m_versions.at(index)->s60SDKDirectory());
            m_ui->gccePath->setPath(m_versions.at(index)->gcceDirectory());
#endif
        } else if (types.contains(ProjectExplorer::ToolChain::INVALID)) {
            makeMSVCVisible(false);
            makeMingwVisible(false);
            makeS60Visible(false);
            if (!m_versions.at(index)->isInstalled())
                m_ui->errorLabel->setText(tr("The Qt Version identified by %1 is not installed. Run make install")
                                           .arg(QDir::toNativeSeparators(m_versions.at(index)->qmakeCommand())));
            else
                m_ui->errorLabel->setText(tr("%1 does not specify a valid Qt installation").arg(QDir::toNativeSeparators(m_versions.at(index)->qmakeCommand())));
        } else { //ProjectExplorer::ToolChain::GCC
            makeMSVCVisible(false);
            makeMingwVisible(false);
            makeS60Visible(false);
            m_ui->errorLabel->setText(tr("Found Qt version %1, using mkspec %2")
                                     .arg(m_versions.at(index)->qtVersionString(),
                                          m_versions.at(index)->mkspec()));
        }
    } else {
        makeMSVCVisible(false);
        makeMingwVisible(false);
        makeS60Visible(false);
    }
}

int QtOptionsPageWidget::indexForTreeItem(const QTreeWidgetItem *item) const
{
    if (!item || !item->parent())
        return -1;
    const int uniqueId = item->data(0, Qt::UserRole).toInt();
    for (int index = 0; index < m_versions.size(); ++index) {
        if (m_versions.at(index)->uniqueId() == uniqueId)
            return index;
    }
    return -1;
}

QTreeWidgetItem *QtOptionsPageWidget::treeItemForIndex(int index) const
{
    const int uniqueId = m_versions.at(index)->uniqueId();
    for (int i = 0; i < m_ui->qtdirList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *toplevelItem = m_ui->qtdirList->topLevelItem(i);
        for (int j = 0; j < toplevelItem->childCount(); ++j) {
            QTreeWidgetItem *item = toplevelItem->child(j);
            if (item->data(0, Qt::UserRole).toInt() == uniqueId) {
                return item;
            }
        }
    }
    return 0;
}

void QtOptionsPageWidget::versionChanged(QTreeWidgetItem *item, QTreeWidgetItem *old)
{
    if (old) {
        fixQtVersionName(indexForTreeItem(old));
    }
    int itemIndex = indexForTreeItem(item);
    if (itemIndex >= 0) {
        m_ui->nameEdit->setText(item->text(0));
        m_ui->qmakePath->setPath(item->text(1));
    } else {
        m_ui->nameEdit->clear();
        m_ui->qmakePath->setPath(QString()); // clear()
    }
    showEnvironmentPage(item);
    updateState();
}

void QtOptionsPageWidget::onQtBrowsed()
{
    const QString dir = m_ui->qmakePath->path();
    if (dir.isEmpty())
        return;

    updateCurrentQMakeLocation();
    if (m_ui->nameEdit->text().isEmpty() || m_ui->nameEdit->text() == m_specifyNameString) {
        QString name = ProjectExplorer::DebuggingHelperLibrary::qtVersionForQMake(QDir::cleanPath(dir));
        if (!name.isEmpty())
            m_ui->nameEdit->setText(name);
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
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    m_versions[currentItemIndex]->setName(m_ui->nameEdit->text());
    currentItem->setText(0, m_versions[currentItemIndex]->name());

    m_ui->defaultCombo->setItemText(currentItemIndex, m_versions[currentItemIndex]->name());
}


void QtOptionsPageWidget::finish()
{
    if (QTreeWidgetItem *item = m_ui->qtdirList->currentItem())
        fixQtVersionName(indexForTreeItem(item));
}

/* Checks that the qt version name is unique
 * and otherwise changes the name
 *
 */
void QtOptionsPageWidget::fixQtVersionName(int index)
{
    if (index < 0)
        return;
    int count = m_versions.count();
    QString name = m_versions.at(index)->name();
    for (int i = 0; i < count; ++i) {
        if (i != index) {
            if (m_versions.at(i)->name() == m_versions.at(index)->name()) {
                // Same name, find new name
                QRegExp regexp("^(.*)\\((\\d)\\)$");
                if (regexp.exactMatch(name)) {
                    // Alreay in Name (#) format
                    name = regexp.cap(1) + "(" + QString().setNum(regexp.cap(2).toInt() + 1) + ")";
                } else {
                    name = name + " (2)";
                }
                // set new name
                m_versions[index]->setName(name);
                treeItemForIndex(index)->setText(0, name);
                m_ui->defaultCombo->setItemText(index, name);

                // Now check again...
                fixQtVersionName(index);
            }
        }
    }
}

void QtOptionsPageWidget::updateCurrentQMakeLocation()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    QtVersion *version = m_versions.at(currentItemIndex).data();
    if (version->qmakeCommand() == m_ui->qmakePath->path())
        return;
    version->setQMakeCommand(m_ui->qmakePath->path());
    currentItem->setText(1, QDir::toNativeSeparators(version->qmakeCommand()));
    showEnvironmentPage(currentItem);

    if (version->isValid()) {
        const bool hasLog = !currentItem->data(2, Qt::UserRole).toString().isEmpty();
        currentItem->setData(2, Qt::DecorationRole, version->hasDebuggingHelper() ? m_debuggingHelperOkIcon : m_debuggingHelperErrorIcon);
        m_ui->showLogButton->setEnabled(hasLog);
        m_ui->rebuildButton->setEnabled(true);
    } else {
        currentItem->setData(2, Qt::DecorationRole, QIcon());
        m_ui->rebuildButton->setEnabled(false);
    }
    updateDebuggingHelperStateLabel(version);
}

void QtOptionsPageWidget::updateCurrentMingwDirectory()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    m_versions[currentItemIndex]->setMingwDirectory(m_ui->mingwPath->path());
}

void QtOptionsPageWidget::msvcVersionChanged()
{
    const QString &msvcVersion = m_ui->msvcComboBox->currentText();
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    m_versions[currentItemIndex]->setMsvcVersion(msvcVersion);
}

#ifdef QTCREATOR_WITH_S60
void QtOptionsPageWidget::updateCurrentMwcDirectory()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    m_versions[currentItemIndex]->setMwcDirectory(m_ui->mwcPath->path());
}
void QtOptionsPageWidget::updateCurrentS60SDKDirectory()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    m_versions[currentItemIndex]->setS60SDKDirectory(m_ui->s60SDKPath->path());
}
void QtOptionsPageWidget::updateCurrentGcceDirectory()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    m_versions[currentItemIndex]->setGcceDirectory(m_ui->gccePath->path());
}
#endif

QList<QSharedPointerQtVersion> QtOptionsPageWidget::versions() const
{
    return m_versions;
}

int QtOptionsPageWidget::defaultVersion() const
{
    return m_defaultVersion;
}
