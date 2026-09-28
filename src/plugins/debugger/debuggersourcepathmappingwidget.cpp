// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggersourcepathmappingwidget.h"

#include "commonoptionspage.h"
#include "debuggerengine.h"
#include "debuggertr.h"

#include <projectexplorer/abi.h>

#include <qtsupport/baseqtversion.h>

#include <utils/elfreader.h>
#include <utils/fileutils.h>
#include <utils/guard.h>
#include <utils/guiutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/variablechooser.h>

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QtEndian>

#include <optional>

using namespace Utils;

namespace Debugger::Internal {

class SourcePathMappingModel;

enum { SourceColumn, TargetColumn, ColumnCount };

// Neither side is a path on this machine, so both are kept as typed.
using Mapping = QPair<QString, QString>;

class DebuggerSourcePathMappingWidget : public QGroupBox
{
public:
    DebuggerSourcePathMappingWidget();

    SourcePathMap sourcePathMap() const;
    void setSourcePathMap(const SourcePathMap &);

private:
    void slotAdd();
    void slotAddQt();
    void slotRemove();
    void slotCurrentRowChanged(const QModelIndex &,const QModelIndex &);
    void slotEditSourceFieldChanged();
    void slotEditTargetFieldChanged();

    void resizeColumns();
    void updateEnabled();
    QString editSourceField() const;
    QString editTargetField() const;
    void setEditFieldMapping(const Mapping &m);
    int currentRow() const;
    void setCurrentRow(int r);

    SourcePathMappingModel *m_model;
    QTreeView *m_treeView;
    QPushButton *m_addButton;
    QPushButton *m_addQtButton;
    QPushButton *m_removeButton;
    QLineEdit *m_sourceLineEdit;
    Utils::PathChooser *m_targetChooser;
    Utils::Guard m_editFieldGuard;
};

// Qt's various build paths for unpatched versions.
static QStringList qtBuildPaths()
{
    if (HostOsInfo::isWindowsHost()) {
        return {"Q:/qt5_workdir/w/s",
                "C:/work/build/qt5_workdir/w/s",
                "c:/users/qt/work/qt",
                "c:/Users/qt/work/install",
                "/Users/qt/work/qt"};
    } else if (HostOsInfo::isMacHost()) {
        return { "/Users/qt/work/qt" };
    } else {
        return { "/home/qt/work/qt" };
    }
}

/*!
    \class Debugger::Internal::SourcePathMappingModel

    \brief The SourcePathMappingModel class is a model for the
    DebuggerSourcePathMappingWidget class.

    Maintains mappings and a dummy placeholder row for adding new mappings.
*/

class SourcePathMappingModel : public QStandardItemModel
{
public:

    explicit SourcePathMappingModel(QObject *parent);

    SourcePathMap sourcePathMap() const;
    void setSourcePathMap(const SourcePathMap &map);

    Mapping mappingAt(int row) const;
    bool isNewPlaceHolderAt(int row) { return isNewPlaceHolder(rawMappingAt(row)); }

    void addMapping(const QString &source, const QString &target);

    void addNewMappingPlaceHolder()
        { addMapping(m_newSourcePlaceHolder, m_newTargetPlaceHolder); }

    void setSource(int row, const QString &);
    void setTarget(int row, const QString &);

private:
    inline bool isNewPlaceHolder(const Mapping &m) const;
    inline Mapping rawMappingAt(int row) const;

    const QString m_newSourcePlaceHolder;
    const QString m_newTargetPlaceHolder;
};

SourcePathMappingModel::SourcePathMappingModel(QObject *parent) :
    QStandardItemModel(0, ColumnCount, parent),
    m_newSourcePlaceHolder(Tr::tr("<new source>")),
    m_newTargetPlaceHolder(Tr::tr("<new target>"))
{
    QStringList headers;
    headers.append(Tr::tr("Source path"));
    headers.append(Tr::tr("Target path"));
    setHorizontalHeaderLabels(headers);
}

SourcePathMap SourcePathMappingModel::sourcePathMap() const
{
    SourcePathMap rc;
    const int rows = rowCount();
    for (int r = 0; r < rows; ++r) {
        const Mapping m = mappingAt(r); // Skip placeholders.
        if (!m.first.isEmpty() && !m.second.isEmpty())
            rc.insert(m.first, m.second);
    }
    return rc;
}

// Check a mapping whether it still contains a placeholder.
bool SourcePathMappingModel::isNewPlaceHolder(const Mapping &m) const
{
    const QChar lessThan('<');
    const QChar greaterThan('>');
    return m.first.isEmpty() || m.first.startsWith(lessThan)
           || m.first.endsWith(greaterThan)
           || m.first == m_newSourcePlaceHolder
           || m.second.isEmpty() || m.second.startsWith(lessThan)
           || m.second.endsWith(greaterThan)
           || m.second == m_newTargetPlaceHolder;
}

// Return raw, unfixed mapping
Mapping SourcePathMappingModel::rawMappingAt(int row) const
{
    return Mapping(item(row, SourceColumn)->text(),
                   item(row, TargetColumn)->text());
}

// Return mapping, empty if it is the place holder.
Mapping SourcePathMappingModel::mappingAt(int row) const
{
    const Mapping raw = rawMappingAt(row);
    return isNewPlaceHolder(raw) ? Mapping() : Mapping(raw.first, raw.second);
}

void SourcePathMappingModel::setSourcePathMap(const SourcePathMap &m)
{
    removeRows(0, rowCount());
    const SourcePathMap::const_iterator cend = m.constEnd();
    for (SourcePathMap::const_iterator it = m.constBegin(); it != cend; ++it)
        addMapping(it.key(), it.value());
}

void SourcePathMappingModel::addMapping(const QString &source, const QString &target)
{
    QList<QStandardItem *> items;
    auto sourceItem = new QStandardItem(source);
    sourceItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    auto targetItem = new QStandardItem(target);
    targetItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    items << sourceItem << targetItem;
    appendRow(items);
}

void SourcePathMappingModel::setSource(int row, const QString &s)
{
    QStandardItem *sourceItem = item(row, SourceColumn);
    QTC_ASSERT(sourceItem, return);
    sourceItem->setText(s.isEmpty() ? m_newSourcePlaceHolder : s);
}

void SourcePathMappingModel::setTarget(int row, const QString &t)
{
    QStandardItem *targetItem = item(row, TargetColumn);
    QTC_ASSERT(targetItem, return);
    targetItem->setText(t.isEmpty() ? m_newTargetPlaceHolder : t);
}

/*!
    \class Debugger::Internal::DebuggerSourcePathMappingWidget

    \brief The DebuggerSourcePathMappingWidget class is a widget for maintaining
    a set of source path mappings for the debugger.

    Path mappings to be applied using source path substitution in GDB.
*/

DebuggerSourcePathMappingWidget::DebuggerSourcePathMappingWidget() :
    m_model(new SourcePathMappingModel(this)),
    m_treeView(new QTreeView(this)),
    m_addButton(new QPushButton(Tr::tr("Add"), this)),
    m_addQtButton(new QPushButton(Tr::tr("Add Qt sources..."), this)),
    m_removeButton(new QPushButton(Tr::tr("Remove"), this)),
    m_sourceLineEdit(new QLineEdit(this)),
    m_targetChooser(new PathChooser(this))
{
    setTitle(Tr::tr("Source Paths Mapping"));
    setToolTip(Tr::tr("<p>Mappings of source file folders to "
                  "be used in the debugger can be entered here.</p>"
                  "<p>This is useful when using a copy of the source tree "
                  "at a location different from the one "
                  "at which the modules were built, for example, while "
                  "doing remote debugging.</p>"
                  "<p>If source is specified as a regular expression by starting it with an "
                  "open parenthesis, the paths in the ELF are matched with the "
                  "regular expression to automatically determine the source path.</p>"
                  "<p>Example: <b>(/home/.*/Project)/KnownSubDir -> D:\\Project</b> will "
                  "substitute ELF built by any user to your local project directory.</p>"));
    // Top list/left part.
    m_treeView->setRootIsDecorated(false);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setModel(m_model);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &DebuggerSourcePathMappingWidget::slotCurrentRowChanged);

    // Top list/Right part: Buttons.
    auto buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_addQtButton);
    m_addQtButton->setVisible(!qtBuildPaths().isEmpty());
    m_addQtButton->setToolTip("<p>" + Tr::tr("Add a mapping for Qt's source folders "
        "when using an unpatched version of Qt."));
    buttonLayout->addWidget(m_removeButton);
    connect(m_addButton, &QAbstractButton::clicked,
            this, &DebuggerSourcePathMappingWidget::slotAdd);
    connect(m_addQtButton, &QAbstractButton::clicked,
            this, &DebuggerSourcePathMappingWidget::slotAddQt);
    connect(m_removeButton, &QAbstractButton::clicked,
            this, &DebuggerSourcePathMappingWidget::slotRemove);
    buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

    // Assemble top
    auto treeHLayout = new QHBoxLayout;
    treeHLayout->addWidget(m_treeView);
    treeHLayout->addLayout(buttonLayout);

    // Edit part
    m_targetChooser->setExpectedKind(PathChooserKind::ExistingDirectory);
    m_targetChooser->setAllowPathFromDevice(true);
    m_targetChooser->setValidationFunction([](const QString &text) -> Result<> {
        if (text.trimmed().isEmpty())
            return ResultError(Tr::tr("The path must not be empty."));
        return ResultOk;
    });
    m_targetChooser->setHistoryCompleter("Debugger.MappingTarget.History");
    connect(m_sourceLineEdit, &QLineEdit::textChanged,
            this, &DebuggerSourcePathMappingWidget::slotEditSourceFieldChanged);
    connect(m_targetChooser, &PathChooser::textChanged,
            this, &DebuggerSourcePathMappingWidget::slotEditTargetFieldChanged);
    auto editLayout = new QFormLayout;
    const QString sourceToolTip = "<p>" + Tr::tr("The source path contained in the "
        "debug information of the executable as reported by the debugger");
    auto editSourceLabel = new QLabel(Tr::tr("&Source path:"));
    editSourceLabel->setToolTip(sourceToolTip);
    m_sourceLineEdit->setToolTip(sourceToolTip);
    editSourceLabel->setBuddy(m_sourceLineEdit);
    editLayout->addRow(editSourceLabel, m_sourceLineEdit);

    const QString targetToolTip = "<p>" + Tr::tr("The location of the source tree as seen by "
        "the debugger. This is either a path on the local machine, or, if the debugger runs "
        "on a device, a path on that device, given either the way the debugger sees it or "
        "with the device scheme in front, such as <b>docker://&lt;image&gt;/&lt;path&gt;</b>.");
    auto editTargetLabel = new QLabel(Tr::tr("&Target path:"));
    editTargetLabel->setToolTip(targetToolTip);
    editTargetLabel->setBuddy(m_targetChooser);
    m_targetChooser->setToolTip(targetToolTip);
    editLayout->addRow(editTargetLabel, m_targetChooser);
    editLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_targetChooser->lineEdit());

    // Main layout
    auto mainLayout = new QVBoxLayout;
    mainLayout->addLayout(treeHLayout);
    mainLayout->addLayout(editLayout);
    setLayout(mainLayout);
    updateEnabled();

    connect(m_sourceLineEdit, &QLineEdit::textEdited, this, markSettingsDirty);
    connect(m_targetChooser->lineEdit(), &QLineEdit::textEdited, this, markSettingsDirty);
}

QString DebuggerSourcePathMappingWidget::editSourceField() const
{
    return normalizedSourcePathPrefix(m_sourceLineEdit->text());
}

QString DebuggerSourcePathMappingWidget::editTargetField() const
{
    return m_targetChooser->unexpandedFilePath().toFSPathString();
}

void DebuggerSourcePathMappingWidget::setEditFieldMapping(const Mapping &m)
{
    const GuardLocker locker(m_editFieldGuard);
    m_sourceLineEdit->setText(m.first);
    m_targetChooser->setFilePath(FilePath::fromUserInput(m.second));
}

void DebuggerSourcePathMappingWidget::slotCurrentRowChanged
    (const QModelIndex &current, const QModelIndex &)
{
    setEditFieldMapping(current.isValid() ? m_model->mappingAt(current.row()) : Mapping());
    updateEnabled();
}

void DebuggerSourcePathMappingWidget::resizeColumns()
{
    m_treeView->resizeColumnToContents(SourceColumn);
}

void DebuggerSourcePathMappingWidget::updateEnabled()
{
    // Allow for removing the current item.
    const int row = currentRow();
    const bool hasCurrent = row >= 0;
    m_sourceLineEdit->setEnabled(hasCurrent);
    m_targetChooser->setEnabled(hasCurrent);
    m_removeButton->setEnabled(hasCurrent);
    // Allow for adding only if the current item no longer is the place
    // holder for new items.
    const bool canAdd = !hasCurrent || !m_model->isNewPlaceHolderAt(row);
    m_addButton->setEnabled(canAdd);
    m_addQtButton->setEnabled(canAdd);
}

SourcePathMap DebuggerSourcePathMappingWidget::sourcePathMap() const
{
    return m_model->sourcePathMap();
}

void DebuggerSourcePathMappingWidget::setSourcePathMap(const SourcePathMap &m)
{
    m_model->setSourcePathMap(m);
    if (!m.isEmpty())
        resizeColumns();
}

int DebuggerSourcePathMappingWidget::currentRow() const
{
    const QModelIndex index = m_treeView->selectionModel()->currentIndex();
    return index.isValid() ? index.row() : -1;
}

void DebuggerSourcePathMappingWidget::setCurrentRow(int r)
{
    m_treeView->selectionModel()->setCurrentIndex(m_model->index(r, 0),
                                                  QItemSelectionModel::ClearAndSelect
                                                  |QItemSelectionModel::Current
                                                  |QItemSelectionModel::Rows);
}

void DebuggerSourcePathMappingWidget::slotAdd()
{
    m_model->addNewMappingPlaceHolder();
    setCurrentRow(m_model->rowCount() - 1);
    markSettingsDirty();
}

void DebuggerSourcePathMappingWidget::slotAddQt()
{
    // Add a mapping for various Qt build locations in case of unpatched builds.
    const FilePath qtSourcesPath = FileUtils::getExistingDirectory(Tr::tr("Qt Sources"));
    if (qtSourcesPath.isEmpty())
        return;
    for (const QString &buildPath : qtBuildPaths())
        m_model->addMapping(buildPath, qtSourcesPath.toFSPathString());
    resizeColumns();
    setCurrentRow(m_model->rowCount() - 1);
    markSettingsDirty();
}

void DebuggerSourcePathMappingWidget::slotRemove()
{
    const int row = currentRow();
    if (row >= 0) {
        m_model->removeRow(row);
        markSettingsDirty();
    }
}

void DebuggerSourcePathMappingWidget::slotEditSourceFieldChanged()
{
    if (m_editFieldGuard.isLocked())
        return;
    const int row = currentRow();
    if (row >= 0) {
        m_model->setSource(row, editSourceField());
        updateEnabled();
    }
}

void DebuggerSourcePathMappingWidget::slotEditTargetFieldChanged()
{
    if (m_editFieldGuard.isLocked())
        return;
    const int row = currentRow();
    if (row >= 0) {
        m_model->setTarget(row, editTargetField());
        updateEnabled();
    }
}

QString normalizedSourcePathPrefix(const QString &input)
{
    QString prefix = input.trimmed();
    return prefix.replace('\\', '/');
}

SourcePathMap mergeStartParametersSourcePathMap(const DebuggerRunParameters &sp,
                                                const SourcePathMap &in)
{
    // Do not overwrite user settings.
    SourcePathMap rc = sp.sourcePathMap();
    for (auto it = in.constBegin(), end = in.constEnd(); it != end; ++it) {
        // Entries that start with parenthesis are handled in
        // DebuggerEngine::validateRunParameters
        if (!it.key().startsWith('('))
            rc.insert(it.key(), sp.macroExpander()->expand(it.value()));
    }
    return rc;
}

static bool hasQtSources(const FilePath &qtSourceLocation)
{
    static const QString qglobal = "qtbase/src/corelib/global/qglobal.h";
    return (qtSourceLocation / qglobal).exists();
}

// A root on a Unix build machine, or one with a drive letter on a Windows one.
static bool isAbsoluteBuildRoot(QByteArrayView root)
{
    if (root.startsWith('/'))
        return true;
    if (root.size() < 3 || root.at(1) != ':' || root.at(2) != '/')
        return false;
    const char drive = root.at(0);
    return (drive >= 'a' && drive <= 'z') || (drive >= 'A' && drive <= 'Z');
}

QStringList qtBuildSourceRoots(const QByteArray &debugStrings)
{
    static const QByteArray marker = "/qtbase/src/";
    QStringList roots;
    for (qsizetype hit = debugStrings.indexOf(marker); hit >= 0;
         hit = debugStrings.indexOf(marker, hit + marker.size())) {
        // The recorded path is the NUL terminated string around the marker, its
        // root everything before the marker. Only absolute roots can be mapped.
        const qsizetype start = debugStrings.lastIndexOf('\0', hit) + 1;
        const QByteArrayView recorded(debugStrings.constData() + start, hit - start);
        if (!isAbsoluteBuildRoot(recorded))
            continue;
        const QString root = QString::fromUtf8(recorded);
        if (!roots.contains(root))
            roots.append(root);
    }
    return roots;
}

// Source paths live in .debug_str (DWARF <= 4) or .debug_line_str (DWARF 5).
static QStringList sourceRootsFromSections(ElfReader &reader, const FilePath &binary)
{
    static const QByteArray sections[] = {".debug_str", ".debug_line_str"};
    const ElfData elfData = reader.readHeaders();
    QStringList roots;
    for (const QByteArray &section : sections) {
        if (elfData.indexOf(section) == -1)
            continue;
        const std::unique_ptr<ElfMapper> mapper = reader.readSection(section);
        if (!mapper) {
            qWarning() << "Cannot read" << section << "of" << binary << ":"
                       << reader.errorString();
            continue;
        }
        roots += qtBuildSourceRoots(
            QByteArray::fromRawData(mapper->start, qsizetype(mapper->fdlen)));
    }
    roots.removeDuplicates();
    return roots;
}

// An installed Qt library usually carries no debug information itself, only a
// .gnu_debuglink or a build id naming a companion file. The candidates and their
// order are gdb's, minus the checksum in the debug link.
FilePath debugInfoFile(const FilePath &library, const QByteArray &debugLink,
                       const QByteArray &buildId, const FilePath &debugInfoDir)
{
    const FilePath dir = library.parentDir();
    FilePaths candidates;
    // The build id comes first because it identifies the companion exactly,
    // where the debug link is only a name. Distributions ship the companion in
    // the build id tree and name it without a directory in the debug link, so
    // this is also the only candidate that finds it there.
    if (buildId.size() > 2 && !debugInfoDir.isEmpty()) {
        const QString id = QString::fromLatin1(buildId);
        candidates << debugInfoDir / ".build-id" / id.left(2) / (id.mid(2) + ".debug");
    }
    if (!debugLink.isEmpty()) {
        const QString name = QString::fromUtf8(debugLink);
        candidates << dir / name << dir / ".debug" / name;
        if (!debugInfoDir.isEmpty())
            candidates << debugInfoDir.pathAppended(dir.path()) / name;
    }

    for (const FilePath &candidate : candidates) {
        if (candidate != library && candidate.isReadableFile())
            return candidate;
    }
    return library;
}

static QStringList sourceRootsFromLibrary(const FilePath &library, const FilePath &debugInfoDir)
{
    ElfReader reader(library);
    const ElfData elfData = reader.readHeaders();
    const FilePath companion = debugInfoFile(library, elfData.debugLink, elfData.buildId,
                                             debugInfoDir);
    if (companion == library)
        return sourceRootsFromSections(reader, library);

    ElfReader companionReader(companion);
    return sourceRootsFromSections(companionReader, companion);
}

// gdb's own debug-file-directory default does not depend on the
// autoEnrichParameters() setting that fills debugInfoLocation() in, so the
// companion of a distro-packaged Qt has to be looked for either way.
FilePath debugInfoDirectory(const DebuggerRunParameters &sp)
{
    const FilePath location = sp.debugInfoLocation();
    return location.isEmpty() ? sp.sysRoot() / "/usr/lib/debug" : location;
}

static std::optional<quint32> readUInt32(QByteArrayView data, qsizetype offset)
{
    if (offset < 0 || offset > data.size() - 4)
        return {};
    return qFromLittleEndian<quint32>(data.data() + offset);
}

// A PDB is an MSF container: its streams are scattered over fixed size blocks,
// and a directory lists the blocks of each. The layout is the one described in
// LLVM's "The PDB File Format".
class MsfReader
{
public:
    explicit MsfReader(const FilePath &pdb) : m_pdb(pdb) {}

    bool readDirectory();
    QByteArray stream(quint32 index) const;

private:
    QByteArray readBlocks(QByteArrayView blockList, quint32 size) const;

    const FilePath m_pdb;
    quint32 m_blockSize = 0;
    quint32 m_blockCount = 0;
    QList<quint32> m_streamSizes;
    QList<QByteArray> m_streamBlocks;
};

QByteArray MsfReader::readBlocks(QByteArrayView blockList, quint32 size) const
{
    const quint32 count = (quint64(size) + m_blockSize - 1) / m_blockSize;
    if (quint64(count) * 4 > quint64(blockList.size()))
        return {};
    QByteArray result;
    result.reserve(size);
    // Linkers mostly write a stream in consecutive blocks. Reading those in one go
    // saves the round trips on a remote device.
    for (quint32 first = 0, next = 0; first < count; first = next) {
        const quint32 block = *readUInt32(blockList, first * 4);
        for (next = first + 1; next < count; ++next) {
            if (*readUInt32(blockList, next * 4) != block + (next - first))
                break;
        }
        if (quint64(block) + (next - first) > m_blockCount)
            return {};
        const qint64 wanted = qMin(qint64(next - first) * m_blockSize,
                                   qint64(size) - result.size());
        const Result<QByteArray> data = m_pdb.fileContents(wanted, qint64(block) * m_blockSize);
        if (!data || data->size() != wanted)
            return {};
        result += *data;
    }
    return result;
}

bool MsfReader::readDirectory()
{
    static const QByteArray magic("Microsoft C/C++ MSF 7.00\r\n\x1a" "DS\0\0\0", 32);
    const Result<QByteArray> super = m_pdb.fileContents(56);
    if (!super || super->size() != 56 || !super->startsWith(magic))
        return false;
    m_blockSize = *readUInt32(*super, 32);
    m_blockCount = *readUInt32(*super, 40);
    const quint32 directorySize = *readUInt32(*super, 44);
    if (m_blockSize < 512 || (m_blockSize & (m_blockSize - 1))
        || quint64(m_blockCount) * m_blockSize > quint64(m_pdb.fileSize())
        || directorySize > quint64(m_blockCount) * m_blockSize) {
        return false;
    }

    // The superblock ends with the index of the one block that lists the blocks
    // of the directory.
    const QByteArray blockMap = readBlocks(QByteArrayView(*super).sliced(52), m_blockSize);
    const QByteArray directory = readBlocks(blockMap, directorySize);
    const std::optional<quint32> streamCount = readUInt32(directory, 0);
    if (!streamCount || *streamCount > (directorySize - 4) / 4)
        return false;

    qsizetype offset = 4 + qsizetype(*streamCount) * 4;
    for (quint32 i = 0; i < *streamCount; ++i) {
        quint32 size = *readUInt32(directory, 4 + i * 4);
        if (size == 0xffffffff) // A deleted stream.
            size = 0;
        const qsizetype listSize = ((quint64(size) + m_blockSize - 1) / m_blockSize) * 4;
        if (offset + listSize > directory.size())
            return false;
        m_streamSizes.append(size);
        m_streamBlocks.append(directory.mid(offset, listSize));
        offset += listSize;
    }
    return true;
}

QByteArray MsfReader::stream(quint32 index) const
{
    if (index >= quint32(m_streamSizes.size()))
        return {};
    return readBlocks(m_streamBlocks.at(index), m_streamSizes.at(index));
}

// The index of a stream by its name, looked up in the named stream map at the end
// of the PDB info stream.
static std::optional<quint32> pdbNamedStream(const QByteArray &info, QByteArrayView name)
{
    // Version, signature, age and GUID come first.
    const std::optional<quint32> namesSize = readUInt32(info, 28);
    if (!namesSize || *namesSize > quint64(info.size() - 32))
        return {};
    const QByteArrayView names = QByteArrayView(info).sliced(32, *namesSize);

    // A hash table follows: the number of entries, the capacity, and the bit
    // vectors of present and deleted buckets, then the entries themselves as
    // pairs of name offset and stream index.
    qsizetype offset = 32 + names.size();
    const std::optional<quint32> entryCount = readUInt32(info, offset);
    if (!entryCount)
        return {};
    offset += 8;
    for (int bitVector = 0; bitVector < 2; ++bitVector) {
        const std::optional<quint32> words = readUInt32(info, offset);
        if (!words || *words > quint64(info.size()))
            return {};
        offset += 4 + qsizetype(*words) * 4;
    }
    for (quint32 i = 0; i < *entryCount; ++i, offset += 8) {
        const std::optional<quint32> nameOffset = readUInt32(info, offset);
        const std::optional<quint32> index = readUInt32(info, offset + 4);
        if (!nameOffset || !index || *nameOffset >= quint64(names.size()))
            return {};
        const QByteArrayView entry = names.sliced(*nameOffset);
        if (entry.startsWith(name) && entry.size() > name.size() && entry.at(name.size()) == '\0')
            return index;
    }
    return {};
}

QByteArray pdbSourceFileNames(const FilePath &pdb)
{
    MsfReader reader(pdb);
    if (!reader.readDirectory())
        return {};

    // Stream 1 is the PDB info stream.
    const std::optional<quint32> namesStream = pdbNamedStream(reader.stream(1), "/names");
    if (!namesStream)
        return {};

    // A signature, a hash version and the size of the strings that follow.
    const QByteArray names = reader.stream(*namesStream);
    const std::optional<quint32> signature = readUInt32(names, 0);
    const std::optional<quint32> size = readUInt32(names, 8);
    if (signature != 0xeffeeffe || !size || *size > quint64(names.size() - 12))
        return {};
    return names.mid(12, *size);
}

// The Qt installers put the PDBs next to the DLLs. Release and debug build share
// the sources, so either will do.
static QStringList pdbQtBuildSourceRoots(const QtSupport::QtVersion *qt)
{
    const QString core = QString("Qt%1Core").arg(qt->qtVersion().majorVersion());
    for (const char *suffix : {".pdb", "d.pdb"}) {
        const FilePath pdb = qt->binPath() / QString(core + suffix);
        if (pdb.isReadableFile())
            return qtBuildSourceRoots(pdbSourceFileNames(pdb).replace('\\', '/'));
    }
    return {};
}

QStringList qtBuildSourceRoots(const DebuggerRunParameters &sp, const QtSupport::QtVersion *qt)
{
    if (!qt || !hasQtSources(sp.qtSourceLocation()))
        return {};
    if (sp.toolChainAbi().binaryFormat() == ProjectExplorer::Abi::PEFormat)
        return pdbQtBuildSourceRoots(qt);
    if (sp.toolChainAbi().binaryFormat() != ProjectExplorer::Abi::ElfFormat)
        return {};

    const FilePath library = qt->libraryPath()
        / QString("libQt%1Core.so.%1").arg(qt->qtVersion().majorVersion());
    if (!library.isLocal())
        return {};

    const FilePath debugInfoDir = debugInfoDirectory(sp);
    // The library is local, so a companion on another device is of no use.
    if (!debugInfoDir.isLocal())
        return {};

    return sourceRootsFromLibrary(library, debugInfoDir);
}

/* Merge settings for an installed Qt (unless another setting is already in the map. */
SourcePathMap mergePlatformQtPath(const QString &qtSourceLocation,
                                  const QStringList &qtBuildSourceRoots,
                                  const SourcePathMap &in)
{
    SourcePathMap rc = in;
    // The root recorded in the debug information covers builds whose path is
    // not one of the qtBuildPaths() guesses.
    for (const QString &buildRoot : qtBuildSourceRoots) {
        if (buildRoot != qtSourceLocation && !rc.contains(buildRoot))
            rc.insert(buildRoot, qtSourceLocation);
    }
    for (const QString &buildPath : qtBuildPaths()) {
        if (!rc.contains(buildPath)) // Do not overwrite user settings.
            rc.insert(buildPath, qtSourceLocation);
    }
    return rc;
}

SourcePathMap mergePlatformQtPath(const DebuggerRunParameters &sp, const SourcePathMap &in)
{
    const FilePath sourceLocation = sp.qtSourceLocation();
    if (!hasQtSources(sourceLocation))
        return in;

    return mergePlatformQtPath(sourceLocation.path(), sp.qtBuildSourceRoots(), in);
}

//
// SourcePathMapAspect
//

class SourcePathMapAspectPrivate
{
public:
    QPointer<DebuggerSourcePathMappingWidget> m_widget;
};


SourcePathMapAspect::SourcePathMapAspect(AspectContainer *container)
    : TypedAspect(container), d(new SourcePathMapAspectPrivate)
{
}

SourcePathMapAspect::~SourcePathMapAspect()
{
    delete d;
}

void SourcePathMapAspect::fromMap(const Store &)
{
    QTC_CHECK(false); // This is only used via read/writeSettings
}

void SourcePathMapAspect::toMap(Store &) const
{
    QTC_CHECK(false);
}

bool SourcePathMapAspect::isDirty() const
{
    const_cast<SourcePathMapAspect *>(this)->guiToVolatileValue();
    return m_value != m_volatileValue;
}

void SourcePathMapAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    QTC_CHECK(!d->m_widget);
    d->m_widget = createSubWidget<DebuggerSourcePathMappingWidget>();
    d->m_widget->setSourcePathMap(value());
    parent.addItem(d->m_widget.data());
}

bool SourcePathMapAspect::guiToVolatileValue()
{
    const SourcePathMap old = m_volatileValue;
    if (d->m_widget)
        m_volatileValue = d->m_widget->sourcePathMap();
    return m_volatileValue != old;
}

void SourcePathMapAspect::volatileValueToGui()
{
    if (d->m_widget)
        d->m_widget->setSourcePathMap(m_volatileValue);
}

const char sourcePathMappingArrayNameC[] = "SourcePathMappings";
const char sourcePathMappingSourceKeyC[] = "Source";
const char sourcePathMappingTargetKeyC[] = "Target";

void SourcePathMapAspect::writeSettings() const
{
    const SourcePathMap sourcePathMap = value();
    QtcSettings &s = userSettings();
    s.beginWriteArray(sourcePathMappingArrayNameC);
    if (!sourcePathMap.isEmpty()) {
        const Key sourcePathMappingSourceKey(sourcePathMappingSourceKeyC);
        const Key sourcePathMappingTargetKey(sourcePathMappingTargetKeyC);
        int i = 0;
        for (auto it = sourcePathMap.constBegin(), cend = sourcePathMap.constEnd();
             it != cend;
             ++it, ++i) {
            s.setArrayIndex(i);
            s.setValue(sourcePathMappingSourceKey, it.key());
            s.setValue(sourcePathMappingTargetKey, it.value());
        }
    }
    s.endArray();
}

void SourcePathMapAspect::readSettings()
{
    QtcSettings &s = userSettings();
    SourcePathMap sourcePathMap;
    if (const int count = s.beginReadArray(sourcePathMappingArrayNameC)) {
        const Key sourcePathMappingSourceKey(sourcePathMappingSourceKeyC);
        const Key sourcePathMappingTargetKey(sourcePathMappingTargetKeyC);
        for (int i = 0; i < count; ++i) {
             s.setArrayIndex(i);
             const QString key = s.value(sourcePathMappingSourceKey).toString();
             const QString value = s.value(sourcePathMappingTargetKey).toString();
             sourcePathMap.insert(key, value);
        }
    }
    s.endArray();
    setValue(sourcePathMap);
}

} // Debugger::Internal
