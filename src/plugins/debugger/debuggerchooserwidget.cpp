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

#include "debuggerchooserwidget.h"

#include <coreplugin/coreconstants.h>

#include <projectexplorer/toolchain.h>

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QSet>

#include <QtGui/QCheckBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QIcon>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtGui/QToolButton>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>

enum Columns { abiColumn, binaryColumn, ColumnCount };

typedef QList<QStandardItem *> StandardItemList;

namespace Debugger {
namespace Internal {

// -----------------------------------------------

// Obtain a tooltip for a debugger binary by running --version
static QString debuggerToolTip(const QString &binary)
{
    if (binary.isEmpty())
        return QString();
    if (!QFileInfo(binary).exists())
        return DebuggerChooserWidget::tr("File not found.");
    QProcess process;
    process.start(binary, QStringList(QLatin1String("--version")));
    process.closeWriteChannel();
    if (!process.waitForStarted())
        return DebuggerChooserWidget::tr("Unable to run '%1': %2")
            .arg(binary, process.errorString());
    process.waitForFinished(); // That should never fail
    QString rc = QDir::toNativeSeparators(binary);
    rc += QLatin1String("\n\n");
    rc += QString::fromLocal8Bit(process.readAllStandardOutput());
    rc.remove(QLatin1Char('\r'));
    return rc;
}

// DebuggerBinaryModel: Show toolchains and associated debugger binaries.
// Provides a delayed tooltip listing the debugger version as
// obtained by running it. Provides conveniences for getting/setting the maps and
// for listing the toolchains used and the ones still available.
class DebuggerBinaryModel : public QStandardItemModel
{
public:
    explicit DebuggerBinaryModel(QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    // get / set data as map.
    QMap<QString, QString> debuggerMapping() const;
    void setDebuggerMapping(const QMap<QString, QString> &m);

    QString binary(int row) const;
    QString abi(int row) const;

    void append(const QString &abi, const QString &binary);

    bool isDirty() const;

    static void setAbiItem(QStandardItem *item, const QString &abi);
    static void setBinaryItem(QStandardItem *item, const QString &binary);
};

DebuggerBinaryModel::DebuggerBinaryModel(QObject *parent) :
    QStandardItemModel(0, ColumnCount, parent)
{
    QStringList headers;
    headers.append(DebuggerChooserWidget::tr("ABI"));
    headers.append(DebuggerChooserWidget::tr("Debugger"));
    setHorizontalHeaderLabels(headers);
}

QVariant DebuggerBinaryModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == Qt::ToolTipRole) {
        // Is there a tooltip set?
        const QString itemToolTip = itemFromIndex(index)->toolTip();
        if (!itemToolTip.isEmpty())
            return QVariant(itemToolTip);
        // Run the debugger and obtain the tooltip
        const QString tooltip = debuggerToolTip(binary(index.row()));
        // Set on the whole row
        item(index.row(), abiColumn)->setToolTip(tooltip);
        item(index.row(), binaryColumn)->setToolTip(tooltip);
        return QVariant(tooltip);
    }
    return QStandardItemModel::data(index, role);
}

bool DebuggerBinaryModel::setData(const QModelIndex &index, const QVariant &value, int role)
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

QMap<QString, QString> DebuggerBinaryModel::debuggerMapping() const
{
    QMap<QString, QString> rc;
    const int binaryCount = rowCount();
    for (int r = 0; r < binaryCount; ++r)
        rc.insert(abi(r), binary(r));
    return rc;
}

void DebuggerBinaryModel::setDebuggerMapping(const QMap<QString, QString> &m)
{
    removeRows(0, rowCount());
    for (QMap<QString, QString>::const_iterator i = m.constBegin(); i != m.constEnd(); ++i)
        append(i.key(), i.value());
}

QString DebuggerBinaryModel::binary(int row) const
{
    return QDir::fromNativeSeparators(
        item(row, binaryColumn)->data(Qt::DisplayRole).toString());
}

QString DebuggerBinaryModel::abi(int row) const
{
    return item(row, abiColumn)->data(Qt::DisplayRole).toString();
}

void DebuggerBinaryModel::setBinaryItem(QStandardItem *item, const QString &binary)
{
    item->setText(binary.isEmpty() ? QString() : QDir::toNativeSeparators(binary));
    item->setToolTip(QString());; // clean out delayed tooltip
    item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable);
    item->setData(false);
}

void DebuggerBinaryModel::setAbiItem(QStandardItem *item, const QString &abi)
{
    item->setText(abi);
    item->setToolTip(QString()); // clean out delayed tooltip
    item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
}

void DebuggerBinaryModel::append(const QString &abi, const QString &binary)
{
    QStandardItem *binaryItem = new QStandardItem;
    QStandardItem *abiItem = new QStandardItem;
    DebuggerBinaryModel::setAbiItem(abiItem, abi);
    DebuggerBinaryModel::setBinaryItem(binaryItem, binary);

    StandardItemList row;
    row << abiItem << binaryItem;
    appendRow(row);
}

bool DebuggerBinaryModel::isDirty() const
{
    for (int i = 0; i < rowCount(); ++i) {
        if (item(i, binaryColumn)->data().toBool())
            return true;
    }
    return false;
}

// ----------- DebuggerChooserWidget
DebuggerChooserWidget::DebuggerChooserWidget(QWidget *parent) :
    QWidget(parent),
    m_treeView(new QTreeView),
    m_model(new DebuggerBinaryModel(m_treeView))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setModel(m_model);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setAllColumnsShowFocus(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_treeView);
}

QMap<QString, QString> DebuggerChooserWidget::debuggerMapping() const
{
    return m_model->debuggerMapping();
}

void DebuggerChooserWidget::setDebuggerMapping(const QMap<QString, QString> &m)
{
    m_model->setDebuggerMapping(m);
    for (int c = 0; c < ColumnCount; c++)
        m_treeView->resizeColumnToContents(c);
}

bool DebuggerChooserWidget::isDirty() const
{
    return m_model->isDirty();
}

} // namespace Internal
} // namespace Debugger
