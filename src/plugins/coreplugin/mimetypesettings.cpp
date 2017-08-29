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

#include "coreconstants.h"
#include "icore.h"
#include "mimetypemagicdialog.h"
#include "mimetypesettings.h"
#include "ui_mimetypesettingspage.h"
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/iexternaleditor.h>

#include <utils/algorithm.h>
#include <utils/headerviewstretcher.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QAbstractTableModel>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QMessageBox>
#include <QPointer>
#include <QScopedPointer>
#include <QSet>
#include <QStringList>
#include <QSortFilterProxyModel>

static const char kModifiedMimeTypesFile[] = "/mimetypes/modifiedmimetypes.xml";

static const char mimeInfoTagC[] = "mime-info";
static const char mimeTypeTagC[] = "mime-type";
static const char mimeTypeAttributeC[] = "type";
static const char patternAttributeC[] = "pattern";
static const char matchTagC[] = "match";
static const char matchValueAttributeC[] = "value";
static const char matchTypeAttributeC[] = "type";
static const char matchOffsetAttributeC[] = "offset";
static const char priorityAttributeC[] = "priority";
static const char matchMaskAttributeC[] = "mask";

namespace Core {
namespace Internal {

class UserMimeType
{
public:
    bool isValid() { return !name.isEmpty(); }
    QString name;
    QStringList globPatterns;
    QMap<int, QList<Utils::Internal::MimeMagicRule> > rules;
};

// MimeTypeSettingsModel
class MimeTypeSettingsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    MimeTypeSettingsModel(QObject *parent = 0)
        : QAbstractTableModel(parent) {}
    virtual ~MimeTypeSettingsModel() {}

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const override;
    virtual QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const override;

    void load();

    QList<Utils::MimeType> m_mimeTypes;
    QHash<QString, QString> m_handlersByMimeType;
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
        return tr("MIME Type");
    else
        return tr("Handler");
}

QVariant MimeTypeSettingsModel::data(const QModelIndex &modelIndex, int role) const
{
    if (!modelIndex.isValid())
        return QVariant();

    const int column = modelIndex.column();
    if (role == Qt::DisplayRole) {
        const QString &type = m_mimeTypes.at(modelIndex.row()).name();
        if (column == 0)
            return type;
        else
            return m_handlersByMimeType.value(type);
    }
    return QVariant();
}

void MimeTypeSettingsModel::load()
{
    beginResetModel();
    m_mimeTypes = Utils::allMimeTypes();
    Utils::sort(m_mimeTypes, [](const Utils::MimeType &a, const Utils::MimeType &b) {
        return a.name().compare(b.name(), Qt::CaseInsensitive) < 0;
    });

    foreach (const Utils::MimeType &mimeType, m_mimeTypes) {
        QString value;
        const QList<IEditorFactory *> factories =
            EditorManager::editorFactories(mimeType);
        if (!factories.isEmpty()) {
            value = factories.front()->displayName();
        } else {
            const QList<IExternalEditor *> externalEditors =
                EditorManager::externalEditors(mimeType);
            if (!externalEditors.isEmpty())
                value = externalEditors.front()->displayName();
            else
                value = tr("Undefined");
        }
        m_handlersByMimeType.insert(mimeType.name(), value);
    }
    endResetModel();
}

// MimeTypeSettingsPrivate
class MimeTypeSettingsPrivate : public QObject
{
    Q_OBJECT

public:
    MimeTypeSettingsPrivate();
    virtual ~MimeTypeSettingsPrivate();

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
    typedef QHash<QString, UserMimeType> UserMimeTypeHash; // name -> mime type
    static const QChar kSemiColon;
    static UserMimeTypeHash readUserModifiedMimeTypes();
    static void applyUserModifiedMimeTypes(const UserMimeTypeHash &mimeTypes);

    static UserMimeTypeHash m_userModifiedMimeTypes; // these are already in mime database
    MimeTypeSettingsModel *m_model;
    QSortFilterProxyModel *m_filterModel;
    UserMimeTypeHash m_pendingModifiedMimeTypes; // currently edited in the options page
    QString m_filterPattern;
    Ui::MimeTypeSettingsPage m_ui;
    QPointer<QWidget> m_widget;
};

const QChar MimeTypeSettingsPrivate::kSemiColon(QLatin1Char(';'));
MimeTypeSettingsPrivate::UserMimeTypeHash MimeTypeSettingsPrivate::m_userModifiedMimeTypes
    = MimeTypeSettingsPrivate::UserMimeTypeHash();

MimeTypeSettingsPrivate::MimeTypeSettingsPrivate()
    : m_model(new MimeTypeSettingsModel(this))
    , m_filterModel(new QSortFilterProxyModel(this))
{
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterKeyColumn(-1);
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &MimeTypeSettingsPrivate::writeUserModifiedMimeTypes);
}

MimeTypeSettingsPrivate::~MimeTypeSettingsPrivate()
{}

void MimeTypeSettingsPrivate::configureUi(QWidget *w)
{
    m_ui.setupUi(w);
    m_ui.filterLineEdit->setText(m_filterPattern);

    m_model->load();
    connect(m_ui.filterLineEdit, &QLineEdit::textChanged,
            this, &MimeTypeSettingsPrivate::setFilterPattern);
    m_ui.mimeTypesTreeView->setModel(m_filterModel);

    new Utils::HeaderViewStretcher(m_ui.mimeTypesTreeView->header(), 1);

    connect(m_ui.mimeTypesTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MimeTypeSettingsPrivate::syncData);
    connect(m_ui.mimeTypesTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MimeTypeSettingsPrivate::updatePatternEditAndMagicButtons);
    connect(m_ui.patternsLineEdit, &QLineEdit::textEdited,
            this, &MimeTypeSettingsPrivate::handlePatternEdited);
    connect(m_ui.addMagicButton, &QPushButton::clicked,
            this, &MimeTypeSettingsPrivate::addMagicHeader);
    // TODO
    connect(m_ui.removeMagicButton, &QPushButton::clicked,
            this, &MimeTypeSettingsPrivate::removeMagicHeader);
    connect(m_ui.editMagicButton, &QPushButton::clicked,
            this, &MimeTypeSettingsPrivate::editMagicHeader);
    connect(m_ui.resetButton, &QPushButton::clicked,
            this, &MimeTypeSettingsPrivate::resetMimeTypes);
    connect(m_ui.magicHeadersTreeWidget, &QTreeWidget::itemSelectionChanged,
            this, &MimeTypeSettingsPrivate::updatePatternEditAndMagicButtons);

    updatePatternEditAndMagicButtons();
}

void MimeTypeSettingsPrivate::syncData(const QModelIndex &current,
                                       const QModelIndex &previous)
{
    Q_UNUSED(previous)
    m_ui.patternsLineEdit->clear();
    m_ui.magicHeadersTreeWidget->clear();

    if (current.isValid()) {
        const Utils::MimeType &currentMimeType =
                m_model->m_mimeTypes.at(m_filterModel->mapToSource(current).row());
        UserMimeType modifiedType = m_pendingModifiedMimeTypes.value(currentMimeType.name());
        m_ui.patternsLineEdit->setText(
                    modifiedType.isValid() ? modifiedType.globPatterns.join(kSemiColon)
                                           : currentMimeType.globPatterns().join(kSemiColon));

        QMap<int, QList<Utils::Internal::MimeMagicRule> > rules =
                modifiedType.isValid() ? modifiedType.rules
                                       : Utils::magicRulesForMimeType(currentMimeType);
        for (auto it = rules.constBegin(); it != rules.constEnd(); ++it) {
            int priority = it.key();
            foreach (const Utils::Internal::MimeMagicRule &rule, it.value()) {
                addMagicHeaderRow(MagicData(rule, priority));
            }
        }
    }
}

void MimeTypeSettingsPrivate::updatePatternEditAndMagicButtons()
{
    const QModelIndex &mimeTypeIndex = m_ui.mimeTypesTreeView->currentIndex();
    const bool mimeTypeValid = mimeTypeIndex.isValid();
    m_ui.patternsLineEdit->setEnabled(mimeTypeValid);
    m_ui.addMagicButton->setEnabled(mimeTypeValid);

    const QModelIndex &magicIndex = m_ui.magicHeadersTreeWidget->currentIndex();
    const bool magicValid = magicIndex.isValid();

    m_ui.removeMagicButton->setEnabled(magicValid);
    m_ui.editMagicButton->setEnabled(magicValid);
}

void MimeTypeSettingsPrivate::handlePatternEdited()
{
    const QModelIndex &modelIndex = m_ui.mimeTypesTreeView->currentIndex();
    QTC_ASSERT(modelIndex.isValid(), return);

    int index = m_filterModel->mapToSource(modelIndex).row();
    const Utils::MimeType mt = m_model->m_mimeTypes.at(index);
    ensurePendingMimeType(mt);
    m_pendingModifiedMimeTypes[mt.name()].globPatterns
            = m_ui.patternsLineEdit->text().split(kSemiColon, QString::SkipEmptyParts);
}

void MimeTypeSettingsPrivate::addMagicHeaderRow(const MagicData &data)
{
    const int row = m_ui.magicHeadersTreeWidget->topLevelItemCount();
    editMagicHeaderRowData(row, data);
}

void MimeTypeSettingsPrivate::editMagicHeaderRowData(const int row, const MagicData &data)
{
    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText(0, QString::fromUtf8(data.m_rule.value()));
    item->setText(1, QString::fromLatin1(Utils::Internal::MimeMagicRule::typeName(data.m_rule.type())));
    item->setText(2, QString::fromLatin1("%1:%2").arg(data.m_rule.startPos()).arg(data.m_rule.endPos()));
    item->setText(3, QString::number(data.m_priority));
    item->setData(0, Qt::UserRole, qVariantFromValue(data));
    m_ui.magicHeadersTreeWidget->takeTopLevelItem(row);
    m_ui.magicHeadersTreeWidget->insertTopLevelItem(row, item);
    m_ui.magicHeadersTreeWidget->setCurrentItem(item);
}

void MimeTypeSettingsPrivate::addMagicHeader()
{
    const QModelIndex &mimeTypeIndex = m_ui.mimeTypesTreeView->currentIndex();
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
    const QModelIndex &mimeTypeIndex = m_ui.mimeTypesTreeView->currentIndex();
    QTC_ASSERT(mimeTypeIndex.isValid(), return);

    const QModelIndex &magicIndex = m_ui.magicHeadersTreeWidget->currentIndex();
    QTC_ASSERT(magicIndex.isValid(), return);

    int index = m_filterModel->mapToSource(mimeTypeIndex).row();
    const Utils::MimeType mt = m_model->m_mimeTypes.at(index);

    QTreeWidgetItem *item = m_ui.magicHeadersTreeWidget->topLevelItem(magicIndex.row());
    QTC_ASSERT(item, return);
    const MagicData data = item->data(0, Qt::UserRole).value<MagicData>();

    ensurePendingMimeType(mt);
    m_pendingModifiedMimeTypes[mt.name()].rules[data.m_priority].removeOne(data.m_rule);
    syncData(mimeTypeIndex, mimeTypeIndex);
}

void MimeTypeSettingsPrivate::editMagicHeader()
{
    const QModelIndex &mimeTypeIndex = m_ui.mimeTypesTreeView->currentIndex();
    QTC_ASSERT(mimeTypeIndex.isValid(), return);

    const QModelIndex &magicIndex = m_ui.magicHeadersTreeWidget->currentIndex();
    QTC_ASSERT(magicIndex.isValid(), return);

    int index = m_filterModel->mapToSource(mimeTypeIndex).row();
    const Utils::MimeType mt = m_model->m_mimeTypes.at(index);

    QTreeWidgetItem *item = m_ui.magicHeadersTreeWidget->topLevelItem(magicIndex.row());
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
                             tr("Reset MIME Types"),
                             tr("Changes will take effect after restart."));
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
    static Utils::FileName modifiedMimeTypesFile = Utils::FileName::fromString(
                ICore::userResourcePath() + QLatin1String(kModifiedMimeTypesFile));

    if (QFile::exists(modifiedMimeTypesFile.toString())
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

            foreach (const UserMimeType &mt, m_userModifiedMimeTypes) {
                writer.writeStartElement(QLatin1String(mimeTypeTagC));
                writer.writeAttribute(QLatin1String(mimeTypeAttributeC), mt.name);
                writer.writeAttribute(QLatin1String(patternAttributeC),
                                      mt.globPatterns.join(kSemiColon));
                for (auto prioIt = mt.rules.constBegin(); prioIt != mt.rules.constEnd(); ++prioIt) {
                    const QString priorityString = QString::number(prioIt.key());
                    foreach (const Utils::Internal::MimeMagicRule &rule, prioIt.value()) {
                        writer.writeStartElement(QLatin1String(matchTagC));
                        writer.writeAttribute(QLatin1String(matchValueAttributeC),
                                              QString::fromUtf8(rule.value()));
                        writer.writeAttribute(QLatin1String(matchTypeAttributeC),
                                              QString::fromUtf8(Utils::Internal::MimeMagicRule::typeName(rule.type())));
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
    static QString modifiedMimeTypesPath = ICore::userResourcePath()
            + QLatin1String(kModifiedMimeTypesFile);
    UserMimeTypeHash userMimeTypes;
    QFile file(modifiedMimeTypesPath);
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
                            .split(kSemiColon, QString::SkipEmptyParts);
                } else if (reader.name() == QLatin1String(matchTagC)) {
                    QByteArray value = atts.value(QLatin1String(matchValueAttributeC)).toUtf8();
                    QByteArray typeName = atts.value(QLatin1String(matchTypeAttributeC)).toUtf8();
                    const QString rangeString = atts.value(QLatin1String(matchOffsetAttributeC)).toString();
                    QPair<int, int> range = rangeFromString(rangeString);
                    int priority = atts.value(QLatin1String(priorityAttributeC)).toString().toInt();
                    QByteArray mask = atts.value(QLatin1String(matchMaskAttributeC)).toLatin1();
                    QString errorMessage;
                    Utils::Internal::MimeMagicRule rule(Utils::Internal::MimeMagicRule::type(typeName),
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

void MimeTypeSettingsPrivate::applyUserModifiedMimeTypes(const UserMimeTypeHash &mimeTypes)
{
    // register in mime data base, and remember for later
    for (auto it = mimeTypes.constBegin(); it != mimeTypes.constEnd(); ++it) {
        Utils::MimeType mt = Utils::mimeTypeForName(it.key());
        if (!mt.isValid()) // loaded from settings
            continue;
        m_userModifiedMimeTypes.insert(it.key(), it.value());
        Utils::setGlobPatternsForMimeType(mt, it.value().globPatterns);
        Utils::setMagicRulesForMimeType(mt, it.value().rules);
    }
}

// MimeTypeSettingsPage
MimeTypeSettings::MimeTypeSettings(QObject *parent)
    : IOptionsPage(parent)
    , d(new MimeTypeSettingsPrivate)
{
    setId(Constants::SETTINGS_ID_MIMETYPES);
    setDisplayName(tr("MIME Types"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setDisplayCategory(QCoreApplication::translate("Core",
        Constants::SETTINGS_TR_CATEGORY_CORE));
    setCategoryIcon(Utils::Icon(Constants::SETTINGS_CATEGORY_CORE_ICON));
}

MimeTypeSettings::~MimeTypeSettings()
{
    delete d;
}

QWidget *MimeTypeSettings::widget()
{
    if (!d->m_widget) {
        d->m_widget = new QWidget;
        d->configureUi(d->m_widget);
    }
    return d->m_widget;
}

void MimeTypeSettings::apply()
{
    MimeTypeSettingsPrivate::applyUserModifiedMimeTypes(d->m_pendingModifiedMimeTypes);
    d->m_pendingModifiedMimeTypes.clear();
    d->m_model->load();
}

void MimeTypeSettings::finish()
{
    d->m_pendingModifiedMimeTypes.clear();
    delete d->m_widget;
}

void MimeTypeSettings::restoreSettings()
{
    MimeTypeSettingsPrivate::UserMimeTypeHash mimetypes
            = MimeTypeSettingsPrivate::readUserModifiedMimeTypes();
    MimeTypeSettingsPrivate::applyUserModifiedMimeTypes(mimetypes);
}

} // Internal
} // Core

#include "mimetypesettings.moc"
