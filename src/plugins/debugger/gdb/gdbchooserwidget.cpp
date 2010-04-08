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

#include <QtCore/QDebug>
#include <QtCore/QSet>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>

enum { binaryRole = Qt::UserRole + 1, toolChainRole = Qt::UserRole + 2 };
enum Columns { binaryColumn, toolChainColumn, ColumnCount };

typedef QList<QStandardItem *> StandardItemList;

Q_DECLARE_METATYPE(QList<int>)

static QList<int> allGdbToolChains()
{
    QList<int> rc;
    rc
#ifdef Q_OS_UNIX
       << ProjectExplorer::ToolChain::GCC
#endif
#ifdef Q_OS_WIN
       << ProjectExplorer::ToolChain::MinGW
       << ProjectExplorer::ToolChain::WINSCW
       << ProjectExplorer::ToolChain::GCCE
       << ProjectExplorer::ToolChain::RVCT_ARMV5
       << ProjectExplorer::ToolChain::RVCT_ARMV6
#endif
       << ProjectExplorer::ToolChain::GCC_MAEMO
#ifdef Q_OS_UNIX
       << ProjectExplorer::ToolChain::GCCE_GNUPOC
       << ProjectExplorer::ToolChain::RVCT_ARMV5_GNUPOC
#endif
       << ProjectExplorer::ToolChain::OTHER
       << ProjectExplorer::ToolChain::UNKNOWN;
    return rc;
}

static inline QString toolChainName(int tc)
{
    return ProjectExplorer::ToolChain::toolChainName(static_cast<ProjectExplorer::ToolChain::ToolChainType>(tc));
}

namespace Debugger {
namespace Internal {

// -----------------------------------------------

// Obtain a tooltip for a gdb binary by running --version
static inline QString gdbToolTip(const QString &binary)
{
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
    typedef GdbChooserWidget::BinaryToolChainMap BinaryToolChainMap;

    explicit GdbBinaryModel(QObject * parent = 0);
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    // get / set data as map.
    BinaryToolChainMap gdbBinaries() const;
    void setGdbBinaries(const BinaryToolChainMap &m);

    QString binary(int row) const;
    QList<int> toolChains(int row) const;

    QStringList binaries() const;
    QList<int> usedToolChains() const;
    QSet<int> unusedToolChainSet() const;
    QList<int> unusedToolChains() const;

    void append(const QString &binary, const QList<int> &toolChains);

    static void setBinaryItem(QStandardItem *item, const QString &binary);
    static void setToolChainItem(QStandardItem *item, const QList<int> &toolChain);
};

GdbBinaryModel::GdbBinaryModel(QObject *parent) :
    QStandardItemModel(0, ColumnCount, parent)
{
    QStringList headers;
    headers << GdbChooserWidget::tr("Binary") << GdbChooserWidget::tr("Toolchains");
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
        item(index.row(), binaryColumn)->setToolTip(tooltip);
        item(index.row(), toolChainColumn)->setToolTip(tooltip);
        return QVariant(tooltip);
    }
    return QStandardItemModel::data(index, role);
}

QStringList GdbBinaryModel::binaries() const
{
    QStringList rc;
    const int binaryCount = rowCount();
    for (int b = 0; b < binaryCount; b++)
        rc.push_back(binary(b));
    return rc;
}

QList<int> GdbBinaryModel::usedToolChains() const
{
    // Loop over model and collect all toolchains.
    QList<int> rc;
    const int binaryCount = rowCount();
    for (int b = 0; b < binaryCount; b++)
        foreach(int tc, toolChains(b))
            rc.push_back(tc);
    return rc;
}

QSet<int> GdbBinaryModel::unusedToolChainSet() const
{
    const QSet<int> used = usedToolChains().toSet();
    QSet<int> all = allGdbToolChains().toSet();
    return all.subtract(used);
}

QList<int> GdbBinaryModel::unusedToolChains() const
{
    QList<int> unused = unusedToolChainSet().toList();
    qSort(unused);
    return unused;
}

GdbBinaryModel::BinaryToolChainMap GdbBinaryModel::gdbBinaries() const
{
    BinaryToolChainMap rc;
    const int binaryCount = rowCount();
    for (int r = 0; r < binaryCount; r++) {
        const QString bin = binary(r);
        foreach(int tc, toolChains(r))
            rc.insert(bin, tc);
    }
    return rc;
}

void GdbBinaryModel::setGdbBinaries(const BinaryToolChainMap &m)
{
    removeRows(0, rowCount());
    foreach(const QString &binary, m.uniqueKeys())
        append(binary, m.values(binary));
}

QString GdbBinaryModel::binary(int row) const
{
    return item(row, binaryColumn)->data(binaryRole).toString();
}

QList<int> GdbBinaryModel::toolChains(int row) const
{
    const QVariant data = item(row, toolChainColumn)->data(toolChainRole);
    return qVariantValue<QList<int> >(data);
}

void GdbBinaryModel::setBinaryItem(QStandardItem *item, const QString &binary)
{
    const QFileInfo fi(binary);
    item->setText(fi.isAbsolute() ? fi.fileName() : QDir::toNativeSeparators(binary));
    item->setToolTip(QString());; // clean out delayed tooltip
    item->setData(QVariant(binary), binaryRole);
    item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
}

void GdbBinaryModel::setToolChainItem(QStandardItem *item, const QList<int> &toolChains)
{
    // Format comma-separated list
    const QString toolChainSeparator = QLatin1String(", ");
    QString toolChainDesc;
    const int count = toolChains.size();
    for (int i = 0; i < count; i++) {
        if (i)
            toolChainDesc += toolChainSeparator;
        toolChainDesc += toolChainName(toolChains.at(i));
    }

    item->setText(toolChainDesc);
    item->setToolTip(QString());; // clean out delayed tooltip
    item->setData(qVariantFromValue(toolChains), toolChainRole);
    item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
}

void GdbBinaryModel::append(const QString &binary, const QList<int> &toolChains)
{
    QStandardItem *binaryItem = new QStandardItem;
    QStandardItem *toolChainItem = new QStandardItem;
    GdbBinaryModel::setBinaryItem(binaryItem, binary);
    GdbBinaryModel::setToolChainItem(toolChainItem, toolChains);
    StandardItemList row;
    row << binaryItem << toolChainItem;
    appendRow(row);
}

// ----------- GdbChooserWidget
GdbChooserWidget::GdbChooserWidget(QWidget *parent) :
    QWidget(parent),
    m_treeView(new QTreeView),
    m_model(new GdbBinaryModel),
    m_addButton(new QToolButton),
    m_deleteButton(new QToolButton)
{
    QHBoxLayout *mainHLayout = new QHBoxLayout;

    m_treeView->setRootIsDecorated(false);
    m_treeView->setModel(m_model);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setAllColumnsShowFocus(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_treeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)));
    connect(m_treeView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(slotDoubleClicked(QModelIndex)));
    mainHLayout->addWidget(m_treeView);

    m_addButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_PLUS)));
    connect(m_addButton, SIGNAL(clicked()), this, SLOT(slotAdd()));

    m_deleteButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_MINUS)));
    m_deleteButton->setEnabled(false);
    connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(slotRemove()));

    QVBoxLayout *vButtonLayout = new QVBoxLayout;
    vButtonLayout->addWidget(m_addButton);
    vButtonLayout->addWidget(m_deleteButton);
    vButtonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

    mainHLayout->addLayout(vButtonLayout);
    setLayout(mainHLayout);
}

QStandardItem *GdbChooserWidget::currentItem() const
{
    // Return the column-0-item
    QModelIndex currentIndex = m_treeView->currentIndex();
    if (!currentIndex.isValid())
        return 0;
    if (currentIndex.column() != binaryColumn)
        currentIndex = currentIndex.sibling(currentIndex.row(), binaryColumn);
    return m_model->itemFromIndex(currentIndex);
}

void GdbChooserWidget::slotAdd()
{
    // Any toolchains left?
    const QList<int> unusedToolChains = m_model->unusedToolChains();
    if (unusedToolChains.isEmpty())
        return;

    // On a binary or no current item: Add binary + toolchain
    BinaryToolChainDialog binaryDialog(this);
    binaryDialog.setToolChainChoices(unusedToolChains);
    if (binaryDialog.exec() != QDialog::Accepted)
        return;
    // Refuse binaries that already exist
    const QString path = binaryDialog.path();
    if (m_model->binaries().contains(path)) {
        QMessageBox::warning(this, tr("Duplicate binary"),
                             tr("The binary '%1' already exists.").arg(path));
        return;
    }
    // Add binary + toolchain to model
    m_model->append(path, binaryDialog.toolChains());
}

void GdbChooserWidget::slotRemove()
{
    if (QStandardItem *item = currentItem())
        removeItem(item);
}

void GdbChooserWidget::removeItem(QStandardItem *item)
{
    m_model->removeRow(item->row());
}

void GdbChooserWidget::slotCurrentChanged(const QModelIndex &current, const QModelIndex &)
{
    const bool hasItem = current.isValid() && m_model->itemFromIndex(current);
    m_deleteButton->setEnabled(hasItem);
}

void GdbChooserWidget::slotDoubleClicked(const QModelIndex &current)
{
    QTC_ASSERT(current.isValid(), return)
    // Show dialog to edit. Make all unused toolchains including the ones
    // previously assigned to that binary available.
    const int row = current.row();
    const QString oldBinary = m_model->binary(row);
    const QList<int> oldToolChains = m_model->toolChains(row);
    const QSet<int> toolChainChoices = m_model->unusedToolChainSet().unite(oldToolChains.toSet());

    BinaryToolChainDialog dialog(this);
    dialog.setPath(oldBinary);
    dialog.setToolChainChoices(toolChainChoices.toList());
    dialog.setToolChains(oldToolChains);
    if (dialog.exec() != QDialog::Accepted)
        return;
    // Check if anything changed.
    const QString newBinary = dialog.path();
    const QList<int> newToolChains = dialog.toolChains();
    if (newBinary == oldBinary && newToolChains == oldToolChains)
        return;

    GdbBinaryModel::setBinaryItem(m_model->item(row, binaryColumn), newBinary);
    GdbBinaryModel::setToolChainItem(m_model->item(row, toolChainColumn), newToolChains);
}

GdbChooserWidget::BinaryToolChainMap GdbChooserWidget::gdbBinaries() const
{
    return m_model->gdbBinaries();
}

void GdbChooserWidget::setGdbBinaries(const BinaryToolChainMap &m)
{
    m_model->setGdbBinaries(m);
    for (int c = 0; c < ColumnCount; c++)
        m_treeView->resizeColumnToContents(c);
}

// -------------- ToolChainSelectorWidget
static const char *toolChainPropertyC = "toolChain";

static inline int toolChainOfCheckBox(const QCheckBox *c)
{
    return c->property(toolChainPropertyC).toInt();
}

static inline QVBoxLayout *createGroupBox(const QString &title, QVBoxLayout *lt)
{
    QGroupBox *gb = new QGroupBox(title);
    QVBoxLayout *gbLayout = new QVBoxLayout;
    gb->setLayout(gbLayout);
    lt->addWidget(gb);
    return gbLayout;
}

ToolChainSelectorWidget::ToolChainSelectorWidget(QWidget *parent) :
    QWidget(parent), m_valid(false)
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    QVBoxLayout *desktopLayout = createGroupBox(tr("Desktop/General"), mainLayout);
    QVBoxLayout *symbianLayout = createGroupBox(tr("Symbian"), mainLayout);
    QVBoxLayout *maemoLayout = createGroupBox(tr("Maemo"), mainLayout);

    // Group checkboxes into categories
    foreach(int tc, allGdbToolChains()) {
        switch (tc) {
        case ProjectExplorer::ToolChain::GCC:
        case ProjectExplorer::ToolChain::MinGW:
        case ProjectExplorer::ToolChain::OTHER:
        case ProjectExplorer::ToolChain::UNKNOWN:
            desktopLayout->addWidget(createToolChainCheckBox(tc));
            break;
        case ProjectExplorer::ToolChain::MSVC:
        case ProjectExplorer::ToolChain::WINCE:
            break;
        case ProjectExplorer::ToolChain::WINSCW:
        case ProjectExplorer::ToolChain::GCCE:
        case ProjectExplorer::ToolChain::RVCT_ARMV5:
        case ProjectExplorer::ToolChain::RVCT_ARMV6:
        case ProjectExplorer::ToolChain::GCCE_GNUPOC:
        case ProjectExplorer::ToolChain::RVCT_ARMV5_GNUPOC:
            symbianLayout->addWidget(createToolChainCheckBox(tc));
            break;
        case ProjectExplorer::ToolChain::GCC_MAEMO:
            maemoLayout->addWidget(createToolChainCheckBox(tc));
            break;
        case ProjectExplorer::ToolChain::INVALID:
            break;
        }
    }
    setLayout(mainLayout);
}

QCheckBox *ToolChainSelectorWidget::createToolChainCheckBox(int tc)
{
    // Add checkbox
    QCheckBox *cb = new QCheckBox(toolChainName(tc));
    cb->setProperty(toolChainPropertyC, QVariant(tc));
    connect(cb, SIGNAL(stateChanged(int)), this, SLOT(slotCheckStateChanged(int)));
    m_checkBoxes.push_back(cb);
    return cb;
}

void ToolChainSelectorWidget::setEnabledToolChains(const QList<int> &enabled)
{
    foreach(QCheckBox *cb, m_checkBoxes)
        if (!enabled.contains(toolChainOfCheckBox(cb)))
            cb->setEnabled(false);
}

void ToolChainSelectorWidget::setCheckedToolChains(const QList<int> &checked)
{
    foreach(QCheckBox *cb, m_checkBoxes)
        if (checked.contains(toolChainOfCheckBox(cb)))
            cb->setChecked(true);
    // Trigger 'valid changed'
    slotCheckStateChanged(checked.isEmpty() ? Qt::Unchecked : Qt::Checked);
}

QList<int> ToolChainSelectorWidget::checkedToolChains() const
{
    QList<int> rc;
    foreach(const QCheckBox *cb, m_checkBoxes)
        if (cb->isChecked())
            rc.push_back(toolChainOfCheckBox(cb));
    return rc;
}

bool ToolChainSelectorWidget::isValid() const
{
    return m_valid;
}

void ToolChainSelectorWidget::slotCheckStateChanged(int state)
{
    // Emit signal if valid state changed
    const bool newValid = state == Qt::Checked || hasCheckedToolChain();
    if (newValid != m_valid) {
        m_valid = newValid;
        emit validChanged(m_valid);
    }
}

bool ToolChainSelectorWidget::hasCheckedToolChain() const
{
    foreach(const QCheckBox *cb, m_checkBoxes)
        if (cb->isChecked())
            return true;
    return false;
}

// -------------- ToolChainDialog
BinaryToolChainDialog::BinaryToolChainDialog(QWidget *parent) :
    QDialog(parent),
    m_toolChainSelector(new ToolChainSelectorWidget),
    m_mainLayout(new QFormLayout),
    m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel)),
    m_pathChooser(new Utils::PathChooser)
{

    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Select binary and toolchains"));

    m_pathChooser->setExpectedKind(Utils::PathChooser::Command);
    m_pathChooser->setPromptDialogTitle(tr("Gdb binary"));
    connect(m_pathChooser, SIGNAL(validChanged()), this, SLOT(slotValidChanged()));
    m_mainLayout->addRow(tr("Path:"), m_pathChooser);

    connect(m_toolChainSelector, SIGNAL(validChanged(bool)), this, SLOT(slotValidChanged()));
    m_mainLayout->addRow(m_toolChainSelector);

    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    m_mainLayout->addRow(m_buttonBox);
    setLayout(m_mainLayout);

    setOkButtonEnabled(false);
    m_pathChooser->setFocus();
}

void BinaryToolChainDialog::setToolChainChoices(const QList<int> &tcs)
{
    m_toolChainSelector->setEnabledToolChains(tcs);
}

void BinaryToolChainDialog::setToolChains(const QList<int> &tcs)
{
    m_toolChainSelector->setCheckedToolChains(tcs);
}

QList<int> BinaryToolChainDialog::toolChains() const
{
    return m_toolChainSelector->checkedToolChains();
}

void BinaryToolChainDialog::setOkButtonEnabled(bool v)
{
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(v);
}

void BinaryToolChainDialog::setPath(const QString &p)
{
    m_pathChooser->setPath(p);
}

QString BinaryToolChainDialog::path() const
{
    return m_pathChooser->path();
}

void BinaryToolChainDialog::slotValidChanged()
{
    setOkButtonEnabled(m_pathChooser->isValid() && m_toolChainSelector->isValid());
}

} // namespace Internal
} // namespace Debugger
