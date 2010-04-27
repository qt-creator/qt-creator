/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "s60devicespreferencepane.h"
#include "ui_s60devicespreferencepane.h"
#include "s60devices.h"

#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <utils/qtcassert.h>
#include <coreplugin/coreconstants.h>

#include <QtCore/QDir>
#include <QtCore/QtDebug>
#include <QtCore/QSharedPointer>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QIcon>
#include <QtGui/QApplication>
#include <QtGui/QStyle>
#include <QtGui/QStandardItemModel>
#include <QtGui/QStandardItem>

enum { deviceRole = Qt::UserRole + 1 };

enum Columns { DefaultColumn, EpocColumn, QtColumn, ColumnCount };

typedef QSharedPointer<Qt4ProjectManager::Internal::S60Devices::Device> DevicePtr;
Q_DECLARE_METATYPE(DevicePtr)

typedef QList<QStandardItem *> StandardItemList;

namespace Qt4ProjectManager {
namespace Internal {

static inline DevicePtr deviceFromItem(const QStandardItem *item)
{
    return qvariant_cast<DevicePtr>(item->data(deviceRole));
}

// Device model storing a shared pointer to the device as user data.
// Provides a checkable 'default' column which works exclusively.
class S60DevicesModel : public QStandardItemModel {
    Q_OBJECT
public:
    typedef QList<S60Devices::Device> DeviceList;

    explicit S60DevicesModel(bool defaultColumnCheckable, QObject *parent = 0);

    void setDevices(const DeviceList &list);
    DeviceList devices() const;
    void appendDevice(const S60Devices::Device &device);

private slots:
    void slotItemChanged(QStandardItem *item);

private:
    const bool m_defaultColumnCheckable;
};

S60DevicesModel::S60DevicesModel(bool defaultColumnCheckable, QObject *parent) :
        QStandardItemModel(0, ColumnCount, parent),
        m_defaultColumnCheckable(defaultColumnCheckable)
{
    QStringList headers;
    headers << S60DevicesBaseWidget::tr("Default")
            << S60DevicesBaseWidget::tr("SDK Location")
            << S60DevicesBaseWidget::tr("Qt Location");
    setHorizontalHeaderLabels(headers);

    if (m_defaultColumnCheckable)
        connect(this, SIGNAL(itemChanged(QStandardItem*)),
                this, SLOT(slotItemChanged(QStandardItem*)));
}

void S60DevicesModel::appendDevice(const S60Devices::Device &device)
{
    // Create SDK/Qt column items with shared pointer to entry as data.
    const QVariant deviceData = qVariantFromValue(DevicePtr(new S60Devices::Device(device)));

    const Qt::ItemFlags flags = Qt::ItemIsEnabled|Qt::ItemIsSelectable;

    QStandardItem *defaultItem = new QStandardItem;
    if (m_defaultColumnCheckable) {
        defaultItem->setCheckable(true);
        defaultItem->setCheckState(device.isDefault ? Qt::Checked : Qt::Unchecked);
        // Item is only checkable if it is not the default.
        Qt::ItemFlags checkFlags = flags;
        if (!device.isDefault)
            checkFlags |= Qt::ItemIsUserCheckable;
        defaultItem->setFlags(checkFlags);
    } else {
        defaultItem->setIcon(device.isDefault ? QIcon(QLatin1String(":/extensionsystem/images/ok.png")) : QIcon());
    }

    defaultItem->setData(deviceData);

    QStandardItem *epocItem = new QStandardItem(QDir::toNativeSeparators(device.epocRoot));
    epocItem->setFlags(flags);
    epocItem->setData(deviceData);

    const QString qtDesc = device.qt.isEmpty() ?
                           S60DevicesModel::tr("No Qt installed") :
                           QDir::toNativeSeparators(device.qt);
    QStandardItem *qtItem = new QStandardItem(qtDesc);
    qtItem->setFlags(flags);
    qtItem->setData(deviceData);

    const QString tooltip = device.toHtml();
    epocItem->setToolTip(tooltip);
    qtItem->setToolTip(tooltip);

    StandardItemList row;
    row << defaultItem << epocItem << qtItem;
    appendRow(row);
}

void S60DevicesModel::setDevices(const DeviceList &list)
{
    removeRows(0, rowCount());
    foreach(const S60Devices::Device &device, list)
        appendDevice(device);
}

S60DevicesModel::DeviceList S60DevicesModel::devices() const
{
    S60DevicesModel::DeviceList rc;
    const int count = rowCount();
    for (int r = 0; r < count; r++)
        rc.push_back(S60Devices::Device(*deviceFromItem(item(r, 0))));
    return rc;
}

void S60DevicesModel::slotItemChanged(QStandardItem *changedItem)
{
    // Sync all "default" checkmarks. Emulate an exclusive group
    // by enabling only the unchecked items (preventing the user from unchecking)
    // and uncheck all other items. Protect against recursion.
    if (changedItem->column() != DefaultColumn || changedItem->checkState() != Qt::Checked)
        return;
    const int row = changedItem->row();
    const int count = rowCount();
    for (int r = 0; r < count; r++) {
        QStandardItem *rowItem = item(r, DefaultColumn);
        if (r == row) { // Prevent uncheck.
            rowItem->setFlags(rowItem->flags() & ~Qt::ItemIsUserCheckable);
            deviceFromItem(rowItem)->isDefault = true;
        } else {
            // Uncheck others.
            rowItem->setCheckState(Qt::Unchecked);
            rowItem->setFlags(rowItem->flags() | Qt::ItemIsUserCheckable);
            deviceFromItem(rowItem)->isDefault = false;
        }
    }
}

// --------------- S60DevicesBaseWidget
S60DevicesBaseWidget::S60DevicesBaseWidget(unsigned flags, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::S60DevicesPreferencePane),
    m_model(new S60DevicesModel(flags & DeviceDefaultCheckable))
{
    m_ui->setupUi(this);
    m_ui->addButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_PLUS)));
    connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(addDevice()));
    m_ui->removeButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_MINUS)));
    connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(removeDevice()));
    m_ui->refreshButton->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
    connect(m_ui->refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
    m_ui->changeQtButton->setIcon(QIcon(QLatin1String(":/welcome/images/qt_logo.png")));
    connect(m_ui->changeQtButton, SIGNAL(clicked()), this, SLOT(changeQtVersion()));

    m_ui->list->setModel(m_model);
    connect(m_ui->list->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentChanged(QModelIndex,QModelIndex)));

    m_ui->addButton->setVisible(flags & ShowAddButton);
    m_ui->removeButton->setVisible(flags & ShowAddButton);
    m_ui->removeButton->setEnabled(false);
    m_ui->changeQtButton->setVisible(flags & ShowChangeQtButton);
    m_ui->removeButton->setEnabled(false);
    m_ui->refreshButton->setVisible(flags & ShowRefreshButton);
}

S60DevicesBaseWidget::~S60DevicesBaseWidget()
{
    delete m_ui;
}

QStandardItem *S60DevicesBaseWidget::currentItem() const
{
    // Return the column-0 item.
    QModelIndex current = m_ui->list->currentIndex();
    if (current.isValid()) {
        if (current.row() != 0)
            current = current.sibling(current.row(), 0);
        return m_model->itemFromIndex(current);
    }
    return 0;
}

S60DevicesBaseWidget::DeviceList S60DevicesBaseWidget::devices() const
{
    return m_model->devices();
}

void S60DevicesBaseWidget::setDevices(const DeviceList &s60devices,
                                      const QString &errorString)
{
    m_model->setDevices(s60devices);

    for (int c = 0; c < ColumnCount; c++)
        m_ui->list->resizeColumnToContents(c);

    if (errorString.isEmpty()) {
        clearErrorLabel();
    } else {
        setErrorLabel(errorString);
    }
}

void S60DevicesBaseWidget::changeQtVersion()
{
    if (const QStandardItem *item = currentItem()) {
        const QString qtDir = promptDirectory(tr("Choose Qt folder"));
        if (!qtDir.isEmpty()) {
            const DevicePtr device = deviceFromItem(item);
            device->qt = qtDir;
        }
    }
}

void S60DevicesBaseWidget::removeDevice()
{
    if (const QStandardItem *item = currentItem())
        m_model->removeRows(item->row(), 1);
}

void S60DevicesBaseWidget::currentChanged(const QModelIndex &current,
                                          const QModelIndex & /* previous */)
{
    const bool hasItem = current.isValid();
    m_ui->changeQtButton->setEnabled(hasItem);
    m_ui->removeButton->setEnabled(hasItem);
}

void S60DevicesBaseWidget::setErrorLabel(const QString& t)
{
    m_ui->errorLabel->setText(t);
    m_ui->errorLabel->setVisible(true);
}

void S60DevicesBaseWidget::clearErrorLabel()
{
    m_ui->errorLabel->setVisible(false);
}

QString S60DevicesBaseWidget::promptDirectory(const QString &title)
{
    return QFileDialog::getExistingDirectory(this, title);
}

void S60DevicesBaseWidget::appendDevice(const S60Devices::Device &d)
{
    m_model->appendDevice(d);
}

int S60DevicesBaseWidget::deviceCount() const
{
    return m_model->rowCount();
}

// ============ AutoDetectS60DevicesWidget

AutoDetectS60DevicesWidget::AutoDetectS60DevicesWidget(QWidget *parent,
                                                       AutoDetectS60Devices *devices,
                                                       bool changeQtVersionEnabled) :
    S60DevicesBaseWidget(ShowRefreshButton | (changeQtVersionEnabled ? unsigned(ShowChangeQtButton) : 0u),
                         parent),
    m_devices(devices)
{
    refresh();
}

void AutoDetectS60DevicesWidget::refresh()
{
    m_devices->detectDevices();
    setDevices(m_devices->devices(), m_devices->errorString());
}

// ============ GnuPocS60DevicesWidget
GnuPocS60DevicesWidget::GnuPocS60DevicesWidget(QWidget *parent) :
    S60DevicesBaseWidget(ShowAddButton|ShowRemoveButton|ShowChangeQtButton|DeviceDefaultCheckable,
                         parent)
{
}

void GnuPocS60DevicesWidget::addDevice()
{
    // 1) Prompt for GnuPoc
    const QString epocRoot = promptDirectory(tr("Step 1 of 2: Choose GnuPoc folder"));
    if (epocRoot.isEmpty())
        return;
    // 2) Prompt for Qt. Catch equal inputs just in case someone clicks very rapidly.
    QString qtDir;
    while (true) {
        qtDir = promptDirectory(tr("Step 2 of 2: Choose Qt folder"));
        if (qtDir.isEmpty())
            return;
        if (qtDir == epocRoot) {
            QMessageBox::warning(this, tr("Adding GnuPoc"),
                                 tr("GnuPoc and Qt folders must not be identical."));
        } else {
            break;
        }
    }
    // Add a device, make default if first.
    S60Devices::Device device = GnuPocS60Devices::createDevice(epocRoot, qtDir);
    if (deviceCount() == 0)
        device.isDefault = true;
    appendDevice(device);
}

// ================= S60DevicesPreferencePane
S60DevicesPreferencePane::S60DevicesPreferencePane(S60Devices *devices, QObject *parent)
        : Core::IOptionsPage(parent),
        m_widget(0),
        m_devices(devices)
{
}

S60DevicesPreferencePane::~S60DevicesPreferencePane()
{
}

QString S60DevicesPreferencePane::id() const
{
    return QLatin1String("Z.S60 SDKs");
}

QString S60DevicesPreferencePane::displayName() const
{
    return tr("S60 SDKs");
}

QString S60DevicesPreferencePane::category() const
{
    return QLatin1String(Constants::QT_SETTINGS_CATEGORY);
}

QString S60DevicesPreferencePane::displayCategory() const
{
    return QCoreApplication::translate("Qt4ProjectManager", Constants::QT_SETTINGS_CATEGORY);
}

QIcon S60DevicesPreferencePane::categoryIcon() const
{
    return QIcon(Constants::QT_SETTINGS_CATEGORY_ICON);
}

S60DevicesBaseWidget *S60DevicesPreferencePane::createWidget(QWidget *parent) const
{
    // Symbian ABLD/Raptor: Qt installed into SDK, cannot change
    if (AutoDetectS60QtDevices *aqd = qobject_cast<AutoDetectS60QtDevices *>(m_devices))
        return new AutoDetectS60DevicesWidget(parent, aqd, false);
    // Not used yet: Manual association of Qt with auto-detected SDK
    if (AutoDetectS60Devices *ad = qobject_cast<AutoDetectS60Devices *>(m_devices))
        return new AutoDetectS60DevicesWidget(parent, ad, true);
    if (GnuPocS60Devices *gd = qobject_cast<GnuPocS60Devices*>(m_devices)) {
        GnuPocS60DevicesWidget *gw = new GnuPocS60DevicesWidget(parent);
        gw->setDevices(gd->devices());
        return gw;
    }
    return 0; // Huh?
}

QWidget *S60DevicesPreferencePane::createPage(QWidget *parent)
{
    if (m_widget)
        delete m_widget;
    m_widget = createWidget(parent);
    QTC_ASSERT(m_widget, return 0)
    return m_widget;
}

void S60DevicesPreferencePane::apply()
{
    QTC_ASSERT(m_widget, return)

    m_devices->setDevices(m_widget->devices());
}

void S60DevicesPreferencePane::finish()
{
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "s60devicespreferencepane.moc"
