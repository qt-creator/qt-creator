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

#include "androiddevicedialog.h"
#include "androidmanager.h"
#include "androidavdmanager.h"
#include "avddialog.h"
#include "ui_androiddevicedialog.h"

#include <utils/environment.h>
#include <utils/progressindicator.h>
#include <utils/algorithm.h>

#include <QMessageBox>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QToolTip>

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
    Q_OBJECT
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
        QStyleOptionViewItem opt = option;
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
            QString topLeft;
            if (device.type == AndroidDeviceInfo::Hardware)
                topLeft = AndroidConfigurations::currentConfig().getProductModel(device.serialNumber);
            else
                topLeft = device.avdname;
            painter->drawText(size + 12, 2 + opt.rect.top() + fm.ascent(), topLeft);


            // topRight
            auto drawTopRight = [&](const QString text, const QFontMetrics &fm) {
                painter->drawText(opt.rect.right() - fm.width(text) - 6 , 2 + opt.rect.top() + fm.ascent(), text);
            };

            if (device.type == AndroidDeviceInfo::Hardware) {
                drawTopRight(device.serialNumber, fm);
            } else {
                AndroidConfig::OpenGl openGl = AndroidConfigurations::currentConfig().getOpenGLEnabled(device.avdname);
                if (openGl == AndroidConfig::OpenGl::Enabled) {
                    drawTopRight(tr("OpenGL enabled"), fm);
                } else if (openGl == AndroidConfig::OpenGl::Disabled) {
                    QFont font = painter->font();
                    font.setBold(true);
                    painter->setFont(font);
                    QFontMetrics fmBold(font);
                    drawTopRight(tr("OpenGL disabled"), fmBold);
                    font.setBold(false);
                    painter->setFont(font);
                }
            }

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
                QFont f = painter->font();
                f.setBold(true);
                painter->setFont(f);
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
        QStyleOptionViewItem opt = option;
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
    Q_OBJECT
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

    QModelIndex indexFor(AndroidDeviceInfo::AndroidDeviceType type, const QString &serial);
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

QModelIndex AndroidDeviceModel::indexFor(AndroidDeviceInfo::AndroidDeviceType type, const QString &serial)
{
    foreach (AndroidDeviceModelNode *topLevelNode, m_root->children()) {
        QList<AndroidDeviceModelNode *> deviceNodes = topLevelNode->children();
        for (int i = 0; i < deviceNodes.size(); ++i) {
            const AndroidDeviceInfo &info = deviceNodes.at(i)->deviceInfo();
            if (info.type != type)
                continue;
            if ((type == AndroidDeviceInfo::Hardware && serial == info.serialNumber)
                    || (type == AndroidDeviceInfo::Emulator && serial == info.avdname))
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

AndroidDeviceDialog::AndroidDeviceDialog(int apiLevel, const QString &abi,
                                         const QString &serialNumber, QWidget *parent) :
    QDialog(parent),
    m_model(new AndroidDeviceModel(apiLevel, abi)),
    m_ui(new Ui::AndroidDeviceDialog),
    m_apiLevel(apiLevel),
    m_abi(abi),
    m_defaultDevice(serialNumber),
    m_avdManager(new AndroidAvdManager)
{
    m_ui->setupUi(this);
    m_ui->deviceView->setModel(m_model);
    m_ui->deviceView->setItemDelegate(new AndroidDeviceModelDelegate(m_ui->deviceView));
    m_ui->deviceView->setHeaderHidden(true);
    m_ui->deviceView->setRootIsDecorated(false);
    m_ui->deviceView->setUniformRowHeights(true);
    m_ui->deviceView->setExpandsOnDoubleClick(false);

    m_ui->defaultDeviceCheckBox->setText(tr("Always use this device for architecture %1 for this project").arg(abi));

    m_ui->noDeviceFoundLabel->setText(QLatin1String("<p align=\"center\"><span style=\" font-size:16pt;\">")
                                      + tr("No Device Found") + QLatin1String("</span></p><br/>")
                                      + msgConnect() + QLatin1String("<br/>")
                                      + msgAdbListDevices());
    connect(m_ui->missingLabel, &QLabel::linkActivated,
            this, &AndroidDeviceDialog::showHelp);

    connect(m_ui->refreshDevicesButton, &QAbstractButton::clicked,
            this, &AndroidDeviceDialog::refreshDeviceList);

    connect(m_ui->createAVDButton, &QAbstractButton::clicked,
            this, &AndroidDeviceDialog::createAvd);
    connect(m_ui->deviceView, &QAbstractItemView::doubleClicked,
            this, &QDialog::accept);

    connect(&m_futureWatcherAddDevice, &QFutureWatcherBase::finished,
            this, &AndroidDeviceDialog::avdAdded);
    connect(&m_futureWatcherRefreshDevices, &QFutureWatcherBase::finished,
            this, &AndroidDeviceDialog::devicesRefreshed);

    connect(m_ui->deviceView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &AndroidDeviceDialog::enableOkayButton);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Large, this);
    m_progressIndicator->attachToWidget(m_ui->deviceView);

    if (serialNumber.isEmpty()) {
        m_ui->lookingForDevice->setVisible(false);
        m_ui->lookingForDeviceCancel->setVisible(false);
    } else {
        m_ui->lookingForDevice->setVisible(true);
        m_ui->lookingForDevice->setText(tr("Looking for default device <b>%1</b>.").arg(serialNumber));
        m_ui->lookingForDeviceCancel->setVisible(true);
    }

    connect(m_ui->lookingForDeviceCancel, &QPushButton::clicked,
            this, &AndroidDeviceDialog::defaultDeviceClear);

    m_connectedDevices = AndroidConfig::connectedDevices(AndroidConfigurations::currentConfig().adbToolPath().toString());
}

AndroidDeviceDialog::~AndroidDeviceDialog()
{
    m_futureWatcherAddDevice.waitForFinished();
    m_futureWatcherRefreshDevices.waitForFinished();
    delete m_ui;
}

AndroidDeviceInfo AndroidDeviceDialog::device()
{
    if (!m_defaultDevice.isEmpty()) {
        auto device = std::find_if(m_connectedDevices.begin(), m_connectedDevices.end(), [this](const AndroidDeviceInfo& info) {
            return info.serialNumber == m_defaultDevice ||
                    info.avdname == m_defaultDevice;
        });

        if (device != m_connectedDevices.end())
            return *device;
        m_defaultDevice.clear();
    }

    refreshDeviceList();

    if (exec() == QDialog::Accepted)
        return m_model->device(m_ui->deviceView->currentIndex());
    return AndroidDeviceInfo();
}

bool AndroidDeviceDialog::saveDeviceSelection() const
{
    return m_ui->defaultDeviceCheckBox->isChecked();
}

void AndroidDeviceDialog::refreshDeviceList()
{
    m_ui->refreshDevicesButton->setEnabled(false);
    m_progressIndicator->show();
    m_connectedDevices = AndroidConfig::connectedDevices(AndroidConfigurations::currentConfig().adbToolPath().toString());
    m_futureWatcherRefreshDevices.setFuture(m_avdManager->avdList());
}

void AndroidDeviceDialog::devicesRefreshed()
{
    m_progressIndicator->hide();
    QString serialNumber;
    AndroidDeviceInfo::AndroidDeviceType deviceType = AndroidDeviceInfo::Hardware;
    QModelIndex currentIndex = m_ui->deviceView->currentIndex();
    if (currentIndex.isValid()) { // save currently selected index
        AndroidDeviceInfo info = m_model->device(currentIndex);
        deviceType = info.type;
        serialNumber = deviceType == AndroidDeviceInfo::Hardware ? info.serialNumber : info.avdname;
    }

    AndroidDeviceInfoList devices = m_futureWatcherRefreshDevices.result();
    QSet<QString> startedAvds = Utils::transform<QSet>(m_connectedDevices,
                                                       [] (const AndroidDeviceInfo &info) {
                                                           return info.avdname;
                                                       });

    for (const AndroidDeviceInfo &dev : devices)
        if (!startedAvds.contains(dev.avdname))
            m_connectedDevices << dev;

    m_model->setDevices(m_connectedDevices);

    m_ui->deviceView->expand(m_model->index(0, 0));
    if (m_model->rowCount() > 1) // we have a incompatible device node
        m_ui->deviceView->expand(m_model->index(1, 0));

    // Smartly select a index
    QModelIndex newIndex;
    if (!m_defaultDevice.isEmpty()) {
        newIndex = m_model->indexFor(AndroidDeviceInfo::Hardware, m_defaultDevice);
        if (!newIndex.isValid())
            newIndex = m_model->indexFor(AndroidDeviceInfo::Emulator, m_defaultDevice);
        if (!newIndex.isValid()) // not found the default device
            defaultDeviceClear();
    }

    if (!newIndex.isValid() && !m_avdNameFromAdd.isEmpty()) {
        newIndex = m_model->indexFor(AndroidDeviceInfo::Emulator, m_avdNameFromAdd);
        m_avdNameFromAdd.clear();
    }

    if (!newIndex.isValid() && !serialNumber.isEmpty())
        newIndex = m_model->indexFor(deviceType, serialNumber);

    if (!newIndex.isValid() && !m_connectedDevices.isEmpty()) {
        AndroidDeviceInfo info = m_connectedDevices.first();
        const QString &name = info.type == AndroidDeviceInfo::Hardware ? info.serialNumber : info.avdname;
        newIndex = m_model->indexFor(info.type, name);
    }

    m_ui->deviceView->setCurrentIndex(newIndex);

    m_ui->stackedWidget->setCurrentIndex(m_connectedDevices.isEmpty() ? 1 : 0);

    m_ui->refreshDevicesButton->setEnabled(true);
    m_connectedDevices.clear();
}

void AndroidDeviceDialog::createAvd()
{
    m_ui->createAVDButton->setEnabled(false);
    CreateAvdInfo info = AvdDialog::gatherCreateAVDInfo(this, AndroidConfigurations::sdkManager(),
                                                        m_apiLevel, m_abi);

    if (!info.isValid()) {
        m_ui->createAVDButton->setEnabled(true);
        return;
    }

    m_futureWatcherAddDevice.setFuture(m_avdManager->createAvd(info));
}

void AndroidDeviceDialog::avdAdded()
{
    m_ui->createAVDButton->setEnabled(true);
    CreateAvdInfo info = m_futureWatcherAddDevice.result();
    if (!info.error.isEmpty()) {
        QMessageBox::critical(this, QApplication::translate("AndroidConfig", "Error Creating AVD"), info.error);
        return;
    }

    m_avdNameFromAdd = info.name;
    refreshDeviceList();
}

void AndroidDeviceDialog::enableOkayButton()
{
    AndroidDeviceModelNode *node = static_cast<AndroidDeviceModelNode *>(m_ui->deviceView->currentIndex().internalPointer());
    bool enable = node && (!node->deviceInfo().serialNumber.isEmpty() || !node->deviceInfo().avdname.isEmpty());
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
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

void AndroidDeviceDialog::defaultDeviceClear()
{
    m_ui->lookingForDevice->setVisible(false);
    m_ui->lookingForDeviceCancel->setVisible(false);
    m_defaultDevice.clear();
}

#include "androiddevicedialog.moc"
