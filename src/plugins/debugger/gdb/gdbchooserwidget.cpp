/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "gdbchooserwidget.h"

#include <utils/pathchooser.h>
#include <projectexplorer/toolchain.h>
#include <coreplugin/coreconstants.h>

#include <utils/qtcassert.h>

#include <QtGui/QTreeView>
#include <QtGui/QStandardItemModel>
#include <QtGui/QStandardItem>
#include <QtGui/QToolButton>
#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QIcon>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtCore/QSet>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>

enum Columns { abiColumn, binaryColumn, ColumnCount };

typedef QList<QStandardItem *> StandardItemList;

namespace Debugger {
namespace Internal {

// -----------------------------------------------

// Obtain a tooltip for a gdb binary by running --version
static inline QString gdbToolTip(const QString &binary)
{
    if (binary.isEmpty())
        return QString();
    if (!QFileInfo(binary).exists())
        return GdbChooserWidget::tr("File not found.");
    QProcess process;
    process.start(binary, QStringList(QLatin1String("--version")));
    process.closeWriteChannel();
    if (!process.waitForStarted())
        return GdbChooserWidget::tr("Unable to run '%1': %2").arg(binary, process.errorString());
    process.waitForFinished(); // That should never fail
    QString rc = QDir::toNativeSeparators(binary);
    rc += QLatin1String("\n\n");
    rc += QString::fromLocal8Bit(process.readAllStandardOutput());
    rc.remove(QLatin1Char('\r'));
    return rc;
}

// GdbBinaryModel: Show gdb binaries and associated toolchains as a list.
// Provides a delayed tooltip listing the gdb version as
// obtained by running it. Provides conveniences for getting/setting the maps and
// for listing the toolchains used and the ones still available.
class GdbBinaryModel : public QStandardItemModel {
public:
    explicit GdbBinaryModel(QObject * parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    // get / set data as map.
    QMap<QString, QString> gdbMapping() const;
    void setGdbMapping(const QMap<QString, QString> &m);

    QString binary(int row) const;
    QString abi(int row) const;

    void append(const QString &abi, const QString &binary);

    bool isDirty() const;

    static void setAbiItem(QStandardItem *item, const QString &abi);
    static void setBinaryItem(QStandardItem *item, const QString &binary);
};

GdbBinaryModel::GdbBinaryModel(QObject *parent) :
    QStandardItemModel(0, ColumnCount, parent)
{
    QStringList headers;
    headers << GdbChooserWidget::tr("ABI") << GdbChooserWidget::tr("Debugger");
    setHorizontalHeaderLabels(headers);
}

QVariant GdbBinaryModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == Qt::ToolTipRole) {
        // Is there a tooltip set?
        const QString itemToolTip = itemFromIndex(index)->toolTip();
        if (!itemToolTip.isEmpty())
            return QVariant(itemToolTip);
        // Run the gdb and obtain the tooltip
        const QString tooltip = gdbToolTip(binary(index.row()));
        // Set on the whole row
        item(index.row(), abiColumn)->setToolTip(tooltip);
        item(index.row(), binaryColumn)->setToolTip(tooltip);
        return QVariant(tooltip);
    }
    return QStandardItemModel::data(index, role);
}

bool GdbBinaryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        Q_ASSERT(index.column() == binaryColumn);
        item(index.row(), abiColumn)->setToolTip(QString());
        item(index.row(), binaryColumn)->setToolTip(QString());
        item(index.row(), binaryColumn)->setData(true);
        QFont f(item(index.row(), binaryColumn)->font());
        f.setBold(true);
        item(index.row(), binaryColumn)->setFont(f);
    }
    return QStandardItemModel::setData(index, value, role);
}

QMap<QString, QString> GdbBinaryModel::gdbMapping() const
{
    QMap<QString, QString> rc;
    const int binaryCount = rowCount();
    for (int r = 0; r < binaryCount; ++r)
        rc.insert(abi(r), binary(r));
    return rc;
}

void GdbBinaryModel::setGdbMapping(const QMap<QString, QString> &m)
{
    removeRows(0, rowCount());
    for (QMap<QString, QString>::const_iterator i = m.constBegin(); i != m.constEnd(); ++i)
        append(i.key(), i.value());
}

QString GdbBinaryModel::binary(int row) const
{
    return QDir::fromNativeSeparators(item(row, binaryColumn)->data(Qt::DisplayRole).toString());
}

QString GdbBinaryModel::abi(int row) const
{
    return item(row, abiColumn)->data(Qt::DisplayRole).toString();
}

void GdbBinaryModel::setBinaryItem(QStandardItem *item, const QString &binary)
{
    item->setText(binary.isEmpty() ? QString() : QDir::toNativeSeparators(binary));
    item->setToolTip(QString());; // clean out delayed tooltip
    item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable);
    item->setData(false);
}

void GdbBinaryModel::setAbiItem(QStandardItem *item, const QString &abi)
{
    item->setText(abi);
    item->setToolTip(QString()); // clean out delayed tooltip
    item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
}

void GdbBinaryModel::append(const QString &abi, const QString &binary)
{
    QStandardItem *binaryItem = new QStandardItem;
    QStandardItem *abiItem = new QStandardItem;
    GdbBinaryModel::setAbiItem(abiItem, abi);
    GdbBinaryModel::setBinaryItem(binaryItem, binary);

    StandardItemList row;
    row << abiItem << binaryItem;
    appendRow(row);
}

bool GdbBinaryModel::isDirty() const
{
    for (int i = 0; i < rowCount(); ++i) {
        if (item(i, binaryColumn)->data().toBool())
            return true;
    }
    return false;
}

// ----------- GdbChooserWidget
GdbChooserWidget::GdbChooserWidget(QWidget *parent) :
    QWidget(parent),
    m_treeView(new QTreeView),
    m_model(new GdbBinaryModel(m_treeView))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setModel(m_model);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setAllColumnsShowFocus(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_treeView);
}

QMap<QString, QString> GdbChooserWidget::gdbMapping() const
{
    return m_model->gdbMapping();
}

void GdbChooserWidget::setGdbMapping(const QMap<QString, QString> &m)
{
    m_model->setGdbMapping(m);
    for (int c = 0; c < ColumnCount; c++)
        m_treeView->resizeColumnToContents(c);
}

bool GdbChooserWidget::isDirty() const
{
    return m_model->isDirty();
}

} // namespace Internal
} // namespace Debugger
