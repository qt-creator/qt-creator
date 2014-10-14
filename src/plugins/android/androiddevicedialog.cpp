/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androiddevicedialog.h"
#include "androidmanager.h"
#include "ui_androiddevicedialog.h"

#include <QMessageBox>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QToolTip>
#include <QtConcurrentRun>

using namespace Android;
using namespace Android::Internal;

namespace Android {
namespace Internal {

// yeah, writing tree models is fun!
class AndroidDeviceModelNode
{
public:
    AndroidDeviceModelNode(AndroidDeviceModelNode *parent, const AndroidDeviceInfo &info, const QString &incompatibleReason = QString())
        : m_parent(parent), m_info(info), m_incompatibleReason(incompatibleReason)
    {
        if (m_parent)
            m_parent->m_children.append(this);
    }

    AndroidDeviceModelNode(AndroidDeviceModelNode *parent, const QString &displayName)
        : m_parent(parent), m_displayName(displayName)
    {
        if (m_parent)
            m_parent->m_children.append(this);
    }

    ~AndroidDeviceModelNode()
    {
        if (m_parent)
            m_parent->m_children.removeOne(this);
        QList<AndroidDeviceModelNode *> children = m_children;
        qDeleteAll(children);
    }

    AndroidDeviceModelNode *parent() const
    {
        return m_parent;
    }

    QList<AndroidDeviceModelNode *> children() const
    {
        return m_children;
    }

    AndroidDeviceInfo deviceInfo() const
    {
        return m_info;
    }

    QString displayName() const
    {
        return m_displayName;
    }

    QString incompatibleReason() const
    {
        return m_incompatibleReason;
    }

private:
    AndroidDeviceModelNode *m_parent;
    AndroidDeviceInfo m_info;
    QString m_incompatibleReason;
    QString m_displayName;
    QList<AndroidDeviceModelNode *> m_children;
};

class AndroidDeviceModelDelegate : public QStyledItemDelegate
{
public:
    AndroidDeviceModelDelegate(QObject * parent = 0)
        : QStyledItemDelegate(parent)
    {

    }

    ~AndroidDeviceModelDelegate()
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItemV4 opt = option;
        initStyleOption(&opt, index);
        painter->save();

        AndroidDeviceModelNode *node = static_cast<AndroidDeviceModelNode *>(index.internalPointer());
        AndroidDeviceInfo device = node->deviceInfo();

        painter->setPen(Qt::NoPen);

        // Paint Background
        QPalette palette = opt.palette; // we always draw enabled
        palette.setCurrentColorGroup(QPalette::Active);
        bool selected = opt.state & QStyle::State_Selected;
        QColor backgroundColor = selected ? palette.highlight().color()
                                          : palette.background().color();
        painter->setBrush(backgroundColor);

        painter->drawRect(0, opt.rect.top(), opt.rect.width() + opt.rect.left(), opt.rect.height());

        QColor textColor;
        // Set Text Color
        if (opt.state & QStyle::State_Selected)
            textColor = palette.highlightedText().color();
        else
            textColor = palette.text().color();
        painter->setPen(textColor);

        if (!node->displayName().isEmpty()) { // Title
            // We have a top level node
            QFont font = opt.font;
            font.setPointSizeF(font.pointSizeF() * 1.2);
            font.setBold(true);

            QFontMetrics fm(font);
            painter->setFont(font);
            int top = (opt.rect.bottom() + opt.rect.top() - fm.height()) / 2 + fm.ascent();
            painter->drawText(6, top, node->displayName());
        } else {
            QIcon icon(device.type == AndroidDeviceInfo::Hardware ? QLatin1String(":/projectexplorer/images/MaemoDevice.png")
                                                                  : QLatin1String(":/projectexplorer/images/Simulator.png"));
            int size = opt.rect.bottom() - opt.rect.top() - 12;
            QPixmap pixmap = icon.pixmap(size, size);
            painter->drawPixmap(6 + (size - pixmap.width()) / 2, opt.rect.top() + 6 + (size - pixmap.width()) / 2, pixmap);

            QFontMetrics fm(opt.font);
            // TopLeft
            QString topLeft = device.serialNumber;
            if (device.type == AndroidDeviceInfo::Hardware)
                topLeft = AndroidConfigurations::currentConfig().getProductModel(device.serialNumber);
            painter->drawText(size + 12, 2 + opt.rect.top() + fm.ascent(), topLeft);

            QString topRight = device.serialNumber;
            // topRight
            if (device.type == AndroidDeviceInfo::Hardware) // otherwise it's not very informative
                painter->drawText(opt.rect.right() - fm.width(topRight) - 6 , 2 + opt.rect.top() + fm.ascent(), topRight);

            // Directory
            QColor mix;
            mix.setRgbF(0.7 * textColor.redF()   + 0.3 * backgroundColor.redF(),
                        0.7 * textColor.greenF() + 0.3 * backgroundColor.greenF(),
                        0.7 * textColor.blueF()  + 0.3 * backgroundColor.blueF());
            painter->setPen(mix);

            QString lineText;
            if (node->incompatibleReason().isEmpty()) {
                lineText = AndroidManager::androidNameForApiLevel(device.sdk) + QLatin1String("  ");
                lineText += AndroidDeviceDialog::tr("ABI:") + device.cpuAbi.join(QLatin1Char(' '));
            } else {
                lineText = node->incompatibleReason();
            }
            painter->drawText(size + 12, opt.rect.top() + fm.ascent() + fm.height() + 6, lineText);
        }

        // Separator lines
        painter->setPen(QColor::fromRgb(150,150,150));
        painter->drawLine(0, opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItemV4 opt = option;
        initStyleOption(&opt, index);

        QFontMetrics fm(option.font);
        QSize s;
        s.setWidth(option.rect.width());
        s.setHeight(fm.height() * 2 + 10);
        return s;
    }
};

class AndroidDeviceModel : public QAbstractItemModel
{
public:
    AndroidDeviceModel(int apiLevel, const QString &abi);
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    AndroidDeviceInfo device(QModelIndex index);
    void setDevices(const QVector<AndroidDeviceInfo> &devices);

    QModelIndex indexFor(const QString &serial);
private:
    int m_apiLevel;
    QString m_abi;
    AndroidDeviceModelNode *m_root;
};

}
}
/////////////////
// AndroidDeviceModel
/////////////////
AndroidDeviceModel::AndroidDeviceModel(int apiLevel, const QString &abi)
    : m_apiLevel(apiLevel), m_abi(abi), m_root(0)
{
}

QModelIndex AndroidDeviceModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0)
        return QModelIndex();

    if (!m_root)
        return QModelIndex();

    if (!parent.isValid()) {
        if (row < 0 || row >= m_root->children().count())
            return QModelIndex();
        return createIndex(row, column, m_root->children().at(row));
    }

    AndroidDeviceModelNode *node = static_cast<AndroidDeviceModelNode *>(parent.internalPointer());
    if (row < node->children().count())
        return createIndex(row, column, node->children().at(row));

    return QModelIndex();
}

QModelIndex AndroidDeviceModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();
    if (!m_root)
        return QModelIndex();
    AndroidDeviceModelNode *node = static_cast<AndroidDeviceModelNode *>(child.internalPointer());
    if (node == m_root)
        return QModelIndex();
    AndroidDeviceModelNode *parent = node->parent();

    if (parent == m_root)
        return QModelIndex();

    AndroidDeviceModelNode *grandParent = parent->parent();
    return createIndex(grandParent->children().indexOf(parent), 0, parent);
}

int AndroidDeviceModel::rowCount(const QModelIndex &parent) const
{
    if (!m_root)
        return 0;
    if (!parent.isValid())
        return m_root->children().count();
    AndroidDeviceModelNode *node = static_cast<AndroidDeviceModelNode *>(parent.internalPointer());
    return node->children().count();
}

int AndroidDeviceModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant AndroidDeviceModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    AndroidDeviceModelNode *node = static_cast<AndroidDeviceModelNode *>(index.internalPointer());
    if (!node)
        return QVariant();
    return node->deviceInfo().serialNumber;
}

Qt::ItemFlags AndroidDeviceModel::flags(const QModelIndex &index) const
{
    AndroidDeviceModelNode *node = static_cast<AndroidDeviceModelNode *>(index.internalPointer());
    if (node)
        if (node->displayName().isEmpty() && node->incompatibleReason().isEmpty())
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return Qt::NoItemFlags;
}

AndroidDeviceInfo AndroidDeviceModel::device(QModelIndex index)
{
    AndroidDeviceModelNode *node = static_cast<AndroidDeviceModelNode *>(index.internalPointer());
    if (!node)
        return AndroidDeviceInfo();
    return node->deviceInfo();
}

void AndroidDeviceModel::setDevices(const QVector<AndroidDeviceInfo> &devices)
{
    beginResetModel();
    delete m_root;
    m_root = new AndroidDeviceModelNode(0, QString());

    AndroidDeviceModelNode *compatibleDevices = new AndroidDeviceModelNode(m_root, AndroidDeviceDialog::tr("Compatible devices"));
    AndroidDeviceModelNode *incompatibleDevices = 0; // created on demand
    foreach (const AndroidDeviceInfo &device, devices) {
        QString error;
        if (device.state == AndroidDeviceInfo::UnAuthorizedState) {
            error = AndroidDeviceDialog::tr("Unauthorized. Please check the confirmation dialog on your device %1.")
                    .arg(device.serialNumber);
        }else if (device.state == AndroidDeviceInfo::OfflineState) {
            error = AndroidDeviceDialog::tr("Offline. Please check the state of your device %1.")
                    .arg(device.serialNumber);
        } else if (!device.cpuAbi.contains(m_abi)) {
            error = AndroidDeviceDialog::tr("ABI is incompatible, device supports ABIs: %1.")
                    .arg(device.cpuAbi.join(QLatin1Char(' ')));
        } else if (device.sdk < m_apiLevel) {
            error = AndroidDeviceDialog::tr("API Level of device is: %1.")
                    .arg(device.sdk);
        } else {
            new AndroidDeviceModelNode(compatibleDevices, device);
            continue;
        }
        if (!incompatibleDevices)
            incompatibleDevices = new AndroidDeviceModelNode(m_root, AndroidDeviceDialog::tr("Incompatible devices"));
        new AndroidDeviceModelNode(incompatibleDevices, device, error);
    }
    endResetModel();
}

QModelIndex AndroidDeviceModel::indexFor(const QString &serial)
{
    foreach (AndroidDeviceModelNode *topLevelNode, m_root->children()) {
        QList<AndroidDeviceModelNode *> deviceNodes = topLevelNode->children();
        for (int i = 0; i < deviceNodes.size(); ++i) {
            if (deviceNodes.at(i)->deviceInfo().serialNumber == serial)
                return createIndex(i, 0, deviceNodes.at(i));
        }
    }
    return QModelIndex();
}

/////////////////
// AndroidDeviceDialog
/////////////////

static inline QString msgConnect()
{
    return AndroidDeviceDialog::tr("<p>Connect an Android device via USB and activate developer mode on it. "
                                   "Some devices require the installation of a USB driver.</p>");

}

static inline QString msgAdbListDevices()
{
    return AndroidDeviceDialog::tr("<p>The adb tool in the Android SDK lists all connected devices if run via &quot;adb devices&quot;.</p>");
}

AndroidDeviceDialog::AndroidDeviceDialog(int apiLevel, const QString &abi, QWidget *parent) :
    QDialog(parent),
    m_model(new AndroidDeviceModel(apiLevel, abi)),
    m_ui(new Ui::AndroidDeviceDialog),
    m_apiLevel(apiLevel),
    m_abi(abi)
{
    m_ui->setupUi(this);
    m_ui->deviceView->setModel(m_model);
    m_ui->deviceView->setItemDelegate(new AndroidDeviceModelDelegate(m_ui->deviceView));
    m_ui->deviceView->setHeaderHidden(true);
    m_ui->deviceView->setRootIsDecorated(false);
    m_ui->deviceView->setUniformRowHeights(true);
    m_ui->deviceView->setExpandsOnDoubleClick(false);

    m_ui->defaultDeviceCheckBox->setText(tr("Always use this device for architecture %1").arg(abi));

    m_ui->noDeviceFoundLabel->setText(QLatin1String("<p align=\"center\"><span style=\" font-size:16pt;\">")
                                      + tr("No Device Found") + QLatin1String("</span></p><br/>")
                                      + msgConnect() + QLatin1String("<br/>")
                                      + msgAdbListDevices());
    connect(m_ui->missingLabel, SIGNAL(linkActivated(QString)),
            this, SLOT(showHelp()));

    connect(m_ui->refreshDevicesButton, SIGNAL(clicked()),
            this, SLOT(refreshDeviceList()));

    connect(m_ui->createAVDButton, SIGNAL(clicked()),
            this, SLOT(createAvd()));
    connect(m_ui->deviceView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(accept()));

    connect(&m_futureWatcher, SIGNAL(finished()),
            this, SLOT(avdAdded()));

    refreshDeviceList();
}

AndroidDeviceDialog::~AndroidDeviceDialog()
{
    delete m_ui;
    m_futureWatcher.waitForFinished();
}

AndroidDeviceInfo AndroidDeviceDialog::device()
{
    if (result() == QDialog::Accepted)
        return m_model->device(m_ui->deviceView->currentIndex());
    return AndroidDeviceInfo();
}

void AndroidDeviceDialog::accept()
{
    QDialog::accept();
}

bool AndroidDeviceDialog::saveDeviceSelection()
{
    return m_ui->defaultDeviceCheckBox->isChecked();
}

void AndroidDeviceDialog::refreshDeviceList()
{
    QString serialNumber;
    QModelIndex currentIndex = m_ui->deviceView->currentIndex();
    if (currentIndex.isValid())
        serialNumber = m_model->device(currentIndex).serialNumber;

    QVector<AndroidDeviceInfo> devices;
    foreach (const AndroidDeviceInfo &info, AndroidConfigurations::currentConfig().connectedDevices())
        if (info.type == AndroidDeviceInfo::Hardware)
            devices << info;

    devices += AndroidConfigurations::currentConfig().androidVirtualDevices();
    m_model->setDevices(devices);

    m_ui->deviceView->expand(m_model->index(0, 0));
    if (m_model->rowCount() > 1) // we have a incompatible device node
        m_ui->deviceView->expand(m_model->index(1, 0));

    // Smartly select a index
    QModelIndex newIndex;
    if (!serialNumber.isEmpty())
        newIndex = m_model->indexFor(serialNumber);

    if (!newIndex.isValid() && !devices.isEmpty())
        newIndex = m_model->indexFor(devices.first().serialNumber);

    m_ui->deviceView->setCurrentIndex(newIndex);

    m_ui->stackedWidget->setCurrentIndex(devices.isEmpty() ? 1 : 0);
}

void AndroidDeviceDialog::createAvd()
{
    m_ui->createAVDButton->setEnabled(false);
    AndroidConfig::CreateAvdInfo info = AndroidConfigurations::currentConfig().gatherCreateAVDInfo(this, m_apiLevel, m_abi);

    if (info.target.isEmpty()) {
        m_ui->createAVDButton->setEnabled(true);
        return;
    }

    m_futureWatcher.setFuture(AndroidConfigurations::currentConfig().createAVD(info));
}

void AndroidDeviceDialog::avdAdded()
{
    m_ui->createAVDButton->setEnabled(true);
    AndroidConfig::CreateAvdInfo info = m_futureWatcher.result();
    if (!info.error.isEmpty()) {
        QMessageBox::critical(this, QApplication::translate("AndroidConfig", "Error Creating AVD"), info.error);
        return;
    }

    refreshDeviceList();
    QModelIndex index = m_model->indexFor(info.name);
    m_ui->deviceView->setCurrentIndex(index);
}

// Does not work.
void AndroidDeviceDialog::clickedOnView(const QModelIndex &idx)
{
    if (idx.isValid()) {
        AndroidDeviceModelNode *node = static_cast<AndroidDeviceModelNode *>(idx.internalPointer());
        if (!node->displayName().isEmpty()) {
            if (m_ui->deviceView->isExpanded(idx))
                m_ui->deviceView->collapse(idx);
            else
                m_ui->deviceView->expand(idx);
        }
    }
}

void AndroidDeviceDialog::showHelp()
{
    QPoint pos = m_ui->missingLabel->pos();
    pos = m_ui->missingLabel->parentWidget()->mapToGlobal(pos);
    QToolTip::showText(pos, msgConnect() + msgAdbListDevices(), this);
}
