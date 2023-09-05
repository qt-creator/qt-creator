// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mimetypesettings.h"

#include "coreconstants.h"
#include "coreplugintr.h"
#include "editormanager/ieditorfactory.h"
#include "editormanager/ieditorfactory_p.h"
#include "icore.h"
#include "mimetypemagicdialog.h"

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/headerviewstretcher.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QAbstractTableModel>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGroupBox>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QScopedPointer>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QTreeWidget>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

const char kModifiedMimeTypesFile[] = "mimetypes/modifiedmimetypes.xml";

const char mimeInfoTagC[] = "mime-info";
const char mimeTypeTagC[] = "mime-type";
const char mimeTypeAttributeC[] = "type";
const char patternAttributeC[] = "pattern";
const char matchTagC[] = "match";
const char matchValueAttributeC[] = "value";
const char matchTypeAttributeC[] = "type";
const char matchOffsetAttributeC[] = "offset";
const char priorityAttributeC[] = "priority";
const char matchMaskAttributeC[] = "mask";

using namespace Utils;

namespace Core::Internal {

class MimeEditorDelegate : public QStyledItemDelegate
{
public:
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const final;
    void setEditorData(QWidget *editor, const QModelIndex &index) const final;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const final;
};

class UserMimeType
{
public:
    bool isValid() const { return !name.isEmpty(); }
    QString name;
    QStringList globPatterns;
    QMap<int, QList<Utils::MimeMagicRule> > rules;
};

// MimeTypeSettingsModel
class MimeTypeSettingsModel : public QAbstractTableModel
{
public:
    enum class Role { DefaultHandler = Qt::UserRole, MimeType };

    MimeTypeSettingsModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) final;
    Qt::ItemFlags flags(const QModelIndex &index) const final;

    void load();

    QList<IEditorFactory *> handlersForMimeType(const Utils::MimeType &mimeType) const;
    IEditorFactory *defaultHandlerForMimeType(const Utils::MimeType &mimeType) const;
    void resetUserDefaults();

    QList<Utils::MimeType> m_mimeTypes;
    mutable QHash<Utils::MimeType, QList<IEditorFactory *>> m_handlersByMimeType;
    QHash<QString, IEditorFactory *> m_userDefault;
};

int MimeTypeSettingsModel::rowCount(const QModelIndex &) const
{
    return m_mimeTypes.size();
}

int MimeTypeSettingsModel::columnCount(const QModelIndex &) const
{
    return 2;
}

QVariant MimeTypeSettingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    if (section == 0)
        return Tr::tr("MIME Type");
    else
        return Tr::tr("Handler");
}

QVariant MimeTypeSettingsModel::data(const QModelIndex &modelIndex, int role) const
{
    if (!modelIndex.isValid())
        return QVariant();

    const int column = modelIndex.column();
    if (role == Qt::DisplayRole) {
        const Utils::MimeType &type = m_mimeTypes.at(modelIndex.row());
        if (column == 0) {
            return type.name();
        } else {
            IEditorFactory *defaultHandler = defaultHandlerForMimeType(type);
            return defaultHandler ? defaultHandler->displayName() : QString();
        }
    } else if (role == Qt::EditRole) {
        return QVariant::fromValue(handlersForMimeType(m_mimeTypes.at(modelIndex.row())));
    } else if (role == int(Role::DefaultHandler)) {
        return QVariant::fromValue(defaultHandlerForMimeType(m_mimeTypes.at(modelIndex.row())));
    } else if (role == Qt::FontRole) {
        if (column == 1) {
            const Utils::MimeType &type = m_mimeTypes.at(modelIndex.row());
            if (m_userDefault.contains(type.name())) {
                QFont font = QGuiApplication::font();
                font.setItalic(true);
                return font;
            }
        }
        return QVariant();
    } else if (role == int(Role::MimeType)) {
        return QVariant::fromValue(m_mimeTypes.at(modelIndex.row()));
    }
    return QVariant();
}

bool MimeTypeSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != int(Role::DefaultHandler) || index.column() != 1)
        return false;
    auto factory = value.value<IEditorFactory *>();
    QTC_ASSERT(factory, return false);
    const int row = index.row();
    QTC_ASSERT(row >= 0 && row < m_mimeTypes.size(), return false);
    const Utils::MimeType mimeType = m_mimeTypes.at(row);
    const QList<IEditorFactory *> handlers = handlersForMimeType(mimeType);
    QTC_ASSERT(handlers.contains(factory), return false);
    if (handlers.first() == factory) // selection is the default anyhow
        m_userDefault.remove(mimeType.name());
    else
        m_userDefault.insert(mimeType.name(), factory);
    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags MimeTypeSettingsModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0 || handlersForMimeType(m_mimeTypes.at(index.row())).size() < 2)
        return QAbstractTableModel::flags(index);
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

void MimeTypeSettingsModel::load()
{
    beginResetModel();
    m_userDefault = Core::Internal::userPreferredEditorTypes();
    m_mimeTypes = Utils::sorted(Utils::allMimeTypes(),
                                [](const Utils::MimeType &a, const Utils::MimeType &b) {
        return a.name().compare(b.name(), Qt::CaseInsensitive) < 0;
    });
    m_handlersByMimeType.clear();
    endResetModel();
}

QList<IEditorFactory *> MimeTypeSettingsModel::handlersForMimeType(const Utils::MimeType &mimeType) const
{
    if (!m_handlersByMimeType.contains(mimeType))
        m_handlersByMimeType.insert(mimeType, IEditorFactory::defaultEditorFactories(mimeType));
    return m_handlersByMimeType.value(mimeType);
}

IEditorFactory *MimeTypeSettingsModel::defaultHandlerForMimeType(const Utils::MimeType &mimeType) const
{
    if (m_userDefault.contains(mimeType.name()))
        return m_userDefault.value(mimeType.name());
    const QList<IEditorFactory *> handlers = handlersForMimeType(mimeType);
    return handlers.isEmpty() ? nullptr : handlers.first();
}

void MimeTypeSettingsModel::resetUserDefaults()
{
    beginResetModel();
    m_userDefault.clear();
    endResetModel();
}

class MimeFilterModel : public QSortFilterProxyModel
{
public:
    explicit MimeFilterModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

MimeFilterModel::MimeFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool MimeFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    const QModelIndex &index = sourceModel()->index(source_row, 0, source_parent);
    const MimeType mt
        = sourceModel()->data(index, int(MimeTypeSettingsModel::Role::MimeType)).value<MimeType>();
    const QModelIndex &handlerIndex = sourceModel()->index(source_row, 1, source_parent);
    const QString handlerText = sourceModel()->data(handlerIndex, Qt::DisplayRole).toString();

    const QStringList matchStrings = mt.globPatterns() << mt.name() << handlerText;
    const QRegularExpression regex = filterRegularExpression();
    for (const QString &str : matchStrings) {
        if (regex.match(str).hasMatch())
            return true;
    }
    return false;
}

// MimeTypeSettingsPrivate
class MimeTypeSettingsPrivate : public QObject
{
public:
    MimeTypeSettingsPrivate();
    ~MimeTypeSettingsPrivate() override;

    void configureUi(QWidget *w);

private:
    void addMagicHeaderRow(const MagicData &data);
    void editMagicHeaderRowData(const int row, const MagicData &data);

    void setFilterPattern(const QString &pattern);
    void syncData(const QModelIndex &current, const QModelIndex &previous);
    void updatePatternEditAndMagicButtons();
    void handlePatternEdited();
    void addMagicHeader();
    void removeMagicHeader();
    void editMagicHeader();
    void resetMimeTypes();

    void ensurePendingMimeType(const Utils::MimeType &mimeType);

    static void writeUserModifiedMimeTypes();

public:
    using UserMimeTypeHash = QHash<QString, UserMimeType>; // name -> mime type
    static const QChar kSemiColon;
    static UserMimeTypeHash readUserModifiedMimeTypes();
    static void applyUserModifiedMimeTypes(const UserMimeTypeHash &mimeTypes);

    static UserMimeTypeHash m_userModifiedMimeTypes; // these are already in mime database
    MimeTypeSettingsModel *m_model;
    MimeFilterModel *m_filterModel;
    UserMimeTypeHash m_pendingModifiedMimeTypes; // currently edited in the options page
    QString m_filterPattern;
    QPointer<QWidget> m_widget;
    MimeEditorDelegate m_delegate;

    QTreeView *m_mimeTypesTreeView;
    QPushButton *m_resetHandlersButton;
    QLineEdit *m_patternsLineEdit;
    QTreeWidget *m_magicHeadersTreeWidget;
    QPushButton *m_addMagicButton;
    QPushButton *m_editMagicButton;
    QPushButton *m_removeMagicButton;
};

const QChar MimeTypeSettingsPrivate::kSemiColon(QLatin1Char(';'));
MimeTypeSettingsPrivate::UserMimeTypeHash MimeTypeSettingsPrivate::m_userModifiedMimeTypes
    = MimeTypeSettingsPrivate::UserMimeTypeHash();

MimeTypeSettingsPrivate::MimeTypeSettingsPrivate()
    : m_model(new MimeTypeSettingsModel(this))
    , m_filterModel(new MimeFilterModel(this))
{
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterKeyColumn(-1);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &MimeTypeSettingsPrivate::writeUserModifiedMimeTypes);
}

MimeTypeSettingsPrivate::~MimeTypeSettingsPrivate() = default;

class MimeTypeSettingsWidget : public IOptionsPageWidget
{
public:
    MimeTypeSettingsWidget(MimeTypeSettingsPrivate *d)
        : d(d)
    {
        d->configureUi(this);
    }

    void apply() final
    {
        MimeTypeSettingsPrivate::applyUserModifiedMimeTypes(d->m_pendingModifiedMimeTypes);
        Core::Internal::setUserPreferredEditorTypes(d->m_model->m_userDefault);
        d->m_pendingModifiedMimeTypes.clear();
        d->m_model->load();
    }

    void finish() final
    {
        d->m_pendingModifiedMimeTypes.clear();
    }

    MimeTypeSettingsPrivate *d;
};

void MimeTypeSettingsPrivate::configureUi(QWidget *w)
{
    auto filterLineEdit = new FancyLineEdit;
    filterLineEdit->setObjectName("filterLineEdit");
    filterLineEdit->setFiltering(true);
    m_filterModel->setFilterWildcard({});

    m_mimeTypesTreeView = new QTreeView;
    m_mimeTypesTreeView->setObjectName("mimeTypesTreeView");
    m_mimeTypesTreeView->setEditTriggers(QAbstractItemView::DoubleClicked
                 |QAbstractItemView::EditKeyPressed|QAbstractItemView::SelectedClicked);
    m_mimeTypesTreeView->setRootIsDecorated(false);
    m_mimeTypesTreeView->setUniformRowHeights(true);

    auto resetButton = new QPushButton;
    resetButton->setToolTip(Tr::tr("Reset all MIME type definitions to their defaults."));
    resetButton->setText(Tr::tr("Reset MIME Types"));

    m_resetHandlersButton = new QPushButton;
    m_resetHandlersButton->setToolTip(Tr::tr("Reset the assigned handler for all MIME type definitions to the default."));
    m_resetHandlersButton->setText(Tr::tr("Reset Handlers"));

    auto patternsLabel = new QLabel;
    patternsLabel->setText(Tr::tr("Patterns:"));

    m_patternsLineEdit = new QLineEdit;
    m_patternsLineEdit->setObjectName("patternsLineEdit");
    m_patternsLineEdit->setToolTip(Tr::tr("A semicolon-separated list of wildcarded file names."));

    m_magicHeadersTreeWidget = new QTreeWidget;
    m_magicHeadersTreeWidget->setHeaderItem(new QTreeWidgetItem({
        Tr::tr("Magic Header"),
        Tr::tr("Type"),
        Tr::tr("Range"),
        Tr::tr("Priority")
    }));
    m_magicHeadersTreeWidget->setRootIsDecorated(false);
    m_magicHeadersTreeWidget->setUniformRowHeights(true);
    m_magicHeadersTreeWidget->setColumnCount(4);

    m_addMagicButton = new QPushButton;
    m_addMagicButton->setText(Tr::tr("Add..."));

    m_editMagicButton = new QPushButton;
    m_editMagicButton->setText(Tr::tr("Edit..."));

    m_removeMagicButton = new QPushButton;
    m_removeMagicButton->setText(Tr::tr("Remove"));

    auto mimeTypesGroupBox = new QGroupBox;
    mimeTypesGroupBox->setTitle(Tr::tr("Registered MIME Types"));

    auto detailsGroupBox = new QGroupBox;
    detailsGroupBox->setTitle(Tr::tr("Details"));

    auto splitter = new QSplitter(w);
    splitter->setOrientation(Qt::Vertical);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(mimeTypesGroupBox);
    splitter->addWidget(detailsGroupBox);

    using namespace Layouting;

    Column {
        filterLineEdit,
        m_mimeTypesTreeView,
        Row { resetButton, m_resetHandlersButton, st }
    }.attachTo(mimeTypesGroupBox);

    Column {
        Form { patternsLabel, m_patternsLineEdit, br, },
        Row {
            m_magicHeadersTreeWidget,
            Column {
                m_addMagicButton,
                m_editMagicButton,
                m_removeMagicButton,
                st
            }
        }
    }.attachTo(detailsGroupBox);

    Column {
        splitter
    }.attachTo(w);

    m_model->load();
    connect(filterLineEdit, &FancyLineEdit::textChanged,
            this, &MimeTypeSettingsPrivate::setFilterPattern);
    m_mimeTypesTreeView->setModel(m_filterModel);
    m_mimeTypesTreeView->setItemDelegate(&m_delegate);

    new Utils::HeaderViewStretcher(m_mimeTypesTreeView->header(), 1);

    connect(m_mimeTypesTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MimeTypeSettingsPrivate::syncData);
    connect(m_mimeTypesTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MimeTypeSettingsPrivate::updatePatternEditAndMagicButtons);
    connect(m_patternsLineEdit, &QLineEdit::textEdited,
            this, &MimeTypeSettingsPrivate::handlePatternEdited);
    connect(m_addMagicButton, &QPushButton::clicked,
            this, &MimeTypeSettingsPrivate::addMagicHeader);
    // TODO
    connect(m_removeMagicButton, &QPushButton::clicked,
            this, &MimeTypeSettingsPrivate::removeMagicHeader);
    connect(m_editMagicButton, &QPushButton::clicked,
            this, &MimeTypeSettingsPrivate::editMagicHeader);
    connect(resetButton, &QPushButton::clicked,
            this, &MimeTypeSettingsPrivate::resetMimeTypes);
    connect(m_resetHandlersButton, &QPushButton::clicked,
            m_model, &MimeTypeSettingsModel::resetUserDefaults);
    connect(m_magicHeadersTreeWidget, &QTreeWidget::itemSelectionChanged,
            this, &MimeTypeSettingsPrivate::updatePatternEditAndMagicButtons);

    updatePatternEditAndMagicButtons();
}

void MimeTypeSettingsPrivate::syncData(const QModelIndex &current,
                                       const QModelIndex &previous)
{
    Q_UNUSED(previous)
    m_patternsLineEdit->clear();
    m_magicHeadersTreeWidget->clear();

    if (current.isValid()) {
        const Utils::MimeType &currentMimeType =
                m_model->m_mimeTypes.at(m_filterModel->mapToSource(current).row());
        UserMimeType modifiedType = m_pendingModifiedMimeTypes.value(currentMimeType.name());
        m_patternsLineEdit->setText(
                    modifiedType.isValid() ? modifiedType.globPatterns.join(kSemiColon)
                                           : currentMimeType.globPatterns().join(kSemiColon));

        QMap<int, QList<Utils::MimeMagicRule> > rules =
                modifiedType.isValid() ? modifiedType.rules
                                       : Utils::magicRulesForMimeType(currentMimeType);
        for (auto it = rules.constBegin(); it != rules.constEnd(); ++it) {
            int priority = it.key();
            for (const Utils::MimeMagicRule &rule : it.value()) {
                addMagicHeaderRow(MagicData(rule, priority));
            }
        }
    }
}

void MimeTypeSettingsPrivate::updatePatternEditAndMagicButtons()
{
    const QModelIndex &mimeTypeIndex = m_mimeTypesTreeView->currentIndex();
    const bool mimeTypeValid = mimeTypeIndex.isValid();
    m_patternsLineEdit->setEnabled(mimeTypeValid);
    m_addMagicButton->setEnabled(mimeTypeValid);

    const QModelIndex &magicIndex = m_magicHeadersTreeWidget->currentIndex();
    const bool magicValid = magicIndex.isValid();

    m_removeMagicButton->setEnabled(magicValid);
    m_editMagicButton->setEnabled(magicValid);
}

void MimeTypeSettingsPrivate::handlePatternEdited()
{
    const QModelIndex &modelIndex = m_mimeTypesTreeView->currentIndex();
    QTC_ASSERT(modelIndex.isValid(), return);

    int index = m_filterModel->mapToSource(modelIndex).row();
    const Utils::MimeType mt = m_model->m_mimeTypes.at(index);
    ensurePendingMimeType(mt);
    m_pendingModifiedMimeTypes[mt.name()].globPatterns
            = m_patternsLineEdit->text().split(kSemiColon, Qt::SkipEmptyParts);
}

void MimeTypeSettingsPrivate::addMagicHeaderRow(const MagicData &data)
{
    const int row = m_magicHeadersTreeWidget->topLevelItemCount();
    editMagicHeaderRowData(row, data);
}

void MimeTypeSettingsPrivate::editMagicHeaderRowData(const int row, const MagicData &data)
{
    auto item = new QTreeWidgetItem;
    item->setText(0, QString::fromUtf8(data.m_rule.value()));
    item->setText(1, QString::fromLatin1(Utils::MimeMagicRule::typeName(data.m_rule.type())));
    item->setText(2, QString::fromLatin1("%1:%2").arg(data.m_rule.startPos()).arg(data.m_rule.endPos()));
    item->setText(3, QString::number(data.m_priority));
    item->setData(0, Qt::UserRole, QVariant::fromValue(data));
    m_magicHeadersTreeWidget->takeTopLevelItem(row);
    m_magicHeadersTreeWidget->insertTopLevelItem(row, item);
    m_magicHeadersTreeWidget->setCurrentItem(item);
}

void MimeTypeSettingsPrivate::addMagicHeader()
{
    const QModelIndex &mimeTypeIndex = m_mimeTypesTreeView->currentIndex();
    QTC_ASSERT(mimeTypeIndex.isValid(), return);

    int index = m_filterModel->mapToSource(mimeTypeIndex).row();
    const Utils::MimeType mt = m_model->m_mimeTypes.at(index);
    MimeTypeMagicDialog dlg;
    if (dlg.exec()) {
        const MagicData &data = dlg.magicData();
        ensurePendingMimeType(mt);
        m_pendingModifiedMimeTypes[mt.name()].rules[data.m_priority].append(data.m_rule);
        addMagicHeaderRow(data);
    }
}

void MimeTypeSettingsPrivate::removeMagicHeader()
{
    const QModelIndex &mimeTypeIndex = m_mimeTypesTreeView->currentIndex();
    QTC_ASSERT(mimeTypeIndex.isValid(), return);

    const QModelIndex &magicIndex = m_magicHeadersTreeWidget->currentIndex();
    QTC_ASSERT(magicIndex.isValid(), return);

    int index = m_filterModel->mapToSource(mimeTypeIndex).row();
    const Utils::MimeType mt = m_model->m_mimeTypes.at(index);

    QTreeWidgetItem *item = m_magicHeadersTreeWidget->topLevelItem(magicIndex.row());
    QTC_ASSERT(item, return);
    const MagicData data = item->data(0, Qt::UserRole).value<MagicData>();

    ensurePendingMimeType(mt);
    m_pendingModifiedMimeTypes[mt.name()].rules[data.m_priority].removeOne(data.m_rule);
    syncData(mimeTypeIndex, mimeTypeIndex);
}

void MimeTypeSettingsPrivate::editMagicHeader()
{
    const QModelIndex &mimeTypeIndex = m_mimeTypesTreeView->currentIndex();
    QTC_ASSERT(mimeTypeIndex.isValid(), return);

    const QModelIndex &magicIndex = m_magicHeadersTreeWidget->currentIndex();
    QTC_ASSERT(magicIndex.isValid(), return);

    int index = m_filterModel->mapToSource(mimeTypeIndex).row();
    const Utils::MimeType mt = m_model->m_mimeTypes.at(index);

    QTreeWidgetItem *item = m_magicHeadersTreeWidget->topLevelItem(magicIndex.row());
    QTC_ASSERT(item, return);
    const MagicData oldData = item->data(0, Qt::UserRole).value<MagicData>();

    MimeTypeMagicDialog dlg;
    dlg.setMagicData(oldData);
    if (dlg.exec()) {
        if (dlg.magicData() != oldData) {
            ensurePendingMimeType(mt);
            const MagicData &dialogData = dlg.magicData();
            int ruleIndex = m_pendingModifiedMimeTypes[mt.name()].rules[oldData.m_priority].indexOf(oldData.m_rule);
            if (oldData.m_priority != dialogData.m_priority) {
                m_pendingModifiedMimeTypes[mt.name()].rules[oldData.m_priority].removeAt(ruleIndex);
                m_pendingModifiedMimeTypes[mt.name()].rules[dialogData.m_priority].append(dialogData.m_rule);
            } else {
                m_pendingModifiedMimeTypes[mt.name()].rules[oldData.m_priority][ruleIndex] = dialogData.m_rule;
            }
            editMagicHeaderRowData(magicIndex.row(), dialogData);
        }
    }
}

void MimeTypeSettingsPrivate::resetMimeTypes()
{
    m_pendingModifiedMimeTypes.clear();
    m_userModifiedMimeTypes.clear(); // settings file will be removed with next settings-save
    QMessageBox::information(ICore::dialogParent(),
                             Tr::tr("Reset MIME Types"),
                             Tr::tr("Changes will take effect after restart."));
}

void MimeTypeSettingsPrivate::setFilterPattern(const QString &pattern)
{
    m_filterPattern = pattern;
    m_filterModel->setFilterWildcard(pattern);
}

void MimeTypeSettingsPrivate::ensurePendingMimeType(const Utils::MimeType &mimeType)
{
    if (!m_pendingModifiedMimeTypes.contains(mimeType.name())) {
        // get a copy of the mime type into pending modified types
        UserMimeType userMt;
        userMt.name = mimeType.name();
        userMt.globPatterns = mimeType.globPatterns();
        userMt.rules = Utils::magicRulesForMimeType(mimeType);
        m_pendingModifiedMimeTypes.insert(userMt.name, userMt);
    }
}

void MimeTypeSettingsPrivate::writeUserModifiedMimeTypes()
{
    static Utils::FilePath modifiedMimeTypesFile = ICore::userResourcePath(kModifiedMimeTypesFile);

    if (QFileInfo::exists(modifiedMimeTypesFile.toString())
            || QDir().mkpath(modifiedMimeTypesFile.parentDir().toString())) {
        QFile file(modifiedMimeTypesFile.toString());
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            // Notice this file only represents user modifications. It is writen in a
            // convienient way for synchronization, which is similar to but not exactly the
            // same format we use for the embedded mime type files.
            QXmlStreamWriter writer(&file);
            writer.setAutoFormatting(true);
            writer.writeStartDocument();
            writer.writeStartElement(QLatin1String(mimeInfoTagC));

            for (const UserMimeType &mt : std::as_const(m_userModifiedMimeTypes)) {
                writer.writeStartElement(QLatin1String(mimeTypeTagC));
                writer.writeAttribute(QLatin1String(mimeTypeAttributeC), mt.name);
                writer.writeAttribute(QLatin1String(patternAttributeC),
                                      mt.globPatterns.join(kSemiColon));
                for (auto prioIt = mt.rules.constBegin(); prioIt != mt.rules.constEnd(); ++prioIt) {
                    const QString priorityString = QString::number(prioIt.key());
                    for (const Utils::MimeMagicRule &rule : prioIt.value()) {
                        writer.writeStartElement(QLatin1String(matchTagC));
                        writer.writeAttribute(QLatin1String(matchValueAttributeC),
                                              QString::fromUtf8(rule.value()));
                        writer.writeAttribute(QLatin1String(matchTypeAttributeC),
                                              QString::fromUtf8(Utils::MimeMagicRule::typeName(rule.type())));
                        writer.writeAttribute(QLatin1String(matchOffsetAttributeC),
                                              QString::fromLatin1("%1:%2").arg(rule.startPos())
                                              .arg(rule.endPos()));
                        writer.writeAttribute(QLatin1String(priorityAttributeC),
                                              priorityString);
                        writer.writeAttribute(QLatin1String(matchMaskAttributeC),
                                              QString::fromLatin1(MagicData::normalizedMask(rule)));
                        writer.writeEndElement();
                    }
                }
                writer.writeEndElement();
            }
            writer.writeEndElement();
            writer.writeEndDocument();
            file.close();
        }
    }
}

static QPair<int, int> rangeFromString(const QString &offset)
{
    const QStringList list = offset.split(QLatin1Char(':'));
    QPair<int, int> range;
    QTC_ASSERT(list.size() > 0, return range);
    range.first = list.at(0).toInt();
    if (list.size() > 1)
        range.second = list.at(1).toInt();
    else
        range.second = range.first;
    return range;
}

MimeTypeSettingsPrivate::UserMimeTypeHash MimeTypeSettingsPrivate::readUserModifiedMimeTypes()
{
    static Utils::FilePath modifiedMimeTypesPath = ICore::userResourcePath(kModifiedMimeTypesFile);
    UserMimeTypeHash userMimeTypes;
    QFile file(modifiedMimeTypesPath.toString());
    if (file.open(QFile::ReadOnly)) {
        UserMimeType mt;
        QXmlStreamReader reader(&file);
        QXmlStreamAttributes atts;
        while (!reader.atEnd()) {
            switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                atts = reader.attributes();
                if (reader.name() == QLatin1String(mimeTypeTagC)) {
                    mt.name = atts.value(QLatin1String(mimeTypeAttributeC)).toString();
                    mt.globPatterns = atts.value(QLatin1String(patternAttributeC)).toString()
                            .split(kSemiColon, Qt::SkipEmptyParts);
                } else if (reader.name() == QLatin1String(matchTagC)) {
                    QByteArray value = atts.value(QLatin1String(matchValueAttributeC)).toUtf8();
                    QByteArray typeName = atts.value(QLatin1String(matchTypeAttributeC)).toUtf8();
                    const QString rangeString = atts.value(QLatin1String(matchOffsetAttributeC)).toString();
                    QPair<int, int> range = rangeFromString(rangeString);
                    int priority = atts.value(QLatin1String(priorityAttributeC)).toString().toInt();
                    QByteArray mask = atts.value(QLatin1String(matchMaskAttributeC)).toLatin1();
                    QString errorMessage;
                    Utils::MimeMagicRule rule(Utils::MimeMagicRule::type(typeName),
                                                        value, range.first, range.second, mask,
                                                        &errorMessage);
                    if (rule.isValid()) {
                        mt.rules[priority].append(rule);
                    } else {
                        qWarning("Error reading magic rule in custom mime type %s: %s",
                                 qPrintable(mt.name), qPrintable(errorMessage));
                    }
                }
                break;
            case QXmlStreamReader::EndElement:
                if (reader.name() == QLatin1String(mimeTypeTagC)) {
                    userMimeTypes.insert(mt.name, mt);
                    mt.name.clear();
                    mt.globPatterns.clear();
                    mt.rules.clear();
                }
                break;
            default:
                break;
            }
        }
        if (reader.hasError())
            qWarning() << modifiedMimeTypesPath << reader.errorString() << reader.lineNumber()
                       << reader.columnNumber();
        file.close();
    }
    return userMimeTypes;
}

static void registerUserModifiedMimeTypes(const MimeTypeSettingsPrivate::UserMimeTypeHash &mimeTypes)
{
    for (auto it = mimeTypes.constBegin(); it != mimeTypes.constEnd(); ++it) {
        Utils::MimeType mt = Utils::mimeTypeForName(it.key());
        if (!mt.isValid())
            continue;
        Utils::setGlobPatternsForMimeType(mt, it.value().globPatterns);
        Utils::setMagicRulesForMimeType(mt, it.value().rules);
    }
}

void MimeTypeSettingsPrivate::applyUserModifiedMimeTypes(const UserMimeTypeHash &mimeTypes)
{
    // register in mime data base, and remember for later
    for (auto it = mimeTypes.constBegin(); it != mimeTypes.constEnd(); ++it)
        m_userModifiedMimeTypes.insert(it.key(), it.value());
    registerUserModifiedMimeTypes(mimeTypes);
}

// MimeTypeSettingsPage

MimeTypeSettings::MimeTypeSettings()
    : d(new MimeTypeSettingsPrivate)
{
    setId(Constants::SETTINGS_ID_MIMETYPES);
    setDisplayName(Tr::tr("MIME Types"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setWidgetCreator([this] { return new MimeTypeSettingsWidget(d); });
}

MimeTypeSettings::~MimeTypeSettings()
{
    delete d;
}

QStringList MimeTypeSettings::keywords() const
{
    return {
        Tr::tr("Reset MIME Types"),
        Tr::tr("Reset Handlers"),
        Tr::tr("Registered MIME Types"),
        Tr::tr("Patterns:"),
        Tr::tr("Add..."),
        Tr::tr("Edit..."),
        Tr::tr("Remove"),
        Tr::tr("Details")
    };
}

void MimeTypeSettings::restoreSettings()
{
    MimeTypeSettingsPrivate::UserMimeTypeHash mimetypes
        = MimeTypeSettingsPrivate::readUserModifiedMimeTypes();
    MimeTypeSettingsPrivate::m_userModifiedMimeTypes = mimetypes;
    Utils::addMimeInitializer([mimetypes] { registerUserModifiedMimeTypes(mimetypes); });
}

QWidget *MimeEditorDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return new QComboBox(parent);
}

void MimeEditorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto box = static_cast<QComboBox *>(editor);
    const auto factories = index.model()->data(index, Qt::EditRole).value<QList<IEditorFactory *>>();
    for (IEditorFactory *factory : factories)
        box->addItem(factory->displayName(), QVariant::fromValue(factory));
    int currentIndex = factories.indexOf(
        index.model()
            ->data(index, int(MimeTypeSettingsModel::Role::DefaultHandler))
            .value<IEditorFactory *>());
    if (QTC_GUARD(currentIndex != -1))
        box->setCurrentIndex(currentIndex);
}

void MimeEditorDelegate::setModelData(QWidget *editor,
                                      QAbstractItemModel *model,
                                      const QModelIndex &index) const
{
    auto box = static_cast<QComboBox *>(editor);
    model->setData(index,
                   box->currentData(Qt::UserRole),
                   int(MimeTypeSettingsModel::Role::DefaultHandler));
}

} // Core::Internal
