/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "profilemodel.h"

#include "profile.h"
#include "profileconfigwidget.h"
#include "profilemanager.h"

#include <utils/qtcassert.h>

#include <QApplication>
#include <QLayout>
#include <QMessageBox>

namespace ProjectExplorer {
namespace Internal {

class ProfileNode
{
public:
    explicit ProfileNode(ProfileNode *pn, Profile *p = 0, bool c = false) :
        parent(pn), profile(p), changed(c)
    {
        if (pn)
            pn->childNodes.append(this);
        widget = ProfileManager::instance()->createConfigWidget(p);
        if (widget) {
            if (p && p->isAutoDetected())
                widget->makeReadOnly();
            widget->setVisible(false);
        }
    }

    ~ProfileNode()
    {
        if (parent)
            parent->childNodes.removeOne(this);

        // deleting a child removes it from childNodes
        // so operate on a temporary list
        QList<ProfileNode *> tmp = childNodes;
        qDeleteAll(tmp);
        Q_ASSERT(childNodes.isEmpty());
    }

    ProfileNode *parent;
    QString newName;
    QList<ProfileNode *> childNodes;
    Profile *profile;
    ProfileConfigWidget *widget;
    bool changed;
};

// --------------------------------------------------------------------------
// ProfileModel
// --------------------------------------------------------------------------

ProfileModel::ProfileModel(QBoxLayout *parentLayout, QObject *parent) :
    QAbstractItemModel(parent),
    m_parentLayout(parentLayout),
    m_defaultNode(0)
{
    Q_ASSERT(m_parentLayout);

    connect(ProfileManager::instance(), SIGNAL(profileAdded(ProjectExplorer::Profile*)),
            this, SLOT(addProfile(ProjectExplorer::Profile*)));
    connect(ProfileManager::instance(), SIGNAL(profileRemoved(ProjectExplorer::Profile*)),
            this, SLOT(removeProfile(ProjectExplorer::Profile*)));
    connect(ProfileManager::instance(), SIGNAL(profileUpdated(ProjectExplorer::Profile*)),
            this, SLOT(updateProfile(ProjectExplorer::Profile*)));
    connect(ProfileManager::instance(), SIGNAL(defaultProfileChanged()),
            this, SLOT(changeDefaultProfile()));

    m_root = new ProfileNode(0);
    m_autoRoot = new ProfileNode(m_root);
    m_manualRoot = new ProfileNode(m_root);

    foreach (Profile *p, ProfileManager::instance()->profiles())
        addProfile(p);

    changeDefaultProfile();
}

ProfileModel::~ProfileModel()
{
    delete m_root;
}

QModelIndex ProfileModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row >= 0 && row < m_root->childNodes.count())
            return createIndex(row, column, m_root->childNodes.at(row));
    }
    ProfileNode *node = static_cast<ProfileNode *>(parent.internalPointer());
    if (row < node->childNodes.count() && column == 0)
        return createIndex(row, column, node->childNodes.at(row));
    else
        return QModelIndex();
}

QModelIndex ProfileModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();
    ProfileNode *node = static_cast<ProfileNode *>(idx.internalPointer());
    if (node->parent == m_root)
        return QModelIndex();
    return index(node->parent);
}

int ProfileModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_root->childNodes.count();
    ProfileNode *node = static_cast<ProfileNode *>(parent.internalPointer());
    return node->childNodes.count();
}

int ProfileModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant ProfileModel::data(const QModelIndex &index, int role) const
{
    static QIcon warningIcon(":/projectexplorer/images/compile_warning.png");

    if (!index.isValid() || index.column() != 0)
        return QVariant();

    ProfileNode *node = static_cast<ProfileNode *>(index.internalPointer());
    QTC_ASSERT(node, return QVariant());
    if (node == m_autoRoot && role == Qt::DisplayRole)
        return tr("Auto-detected");
    if (node == m_manualRoot && role == Qt::DisplayRole)
        return tr("Manual");
    if (node->profile) {
        if (role == Qt::FontRole) {
            QFont f = QApplication::font();
            if (node->changed)
                f.setBold(!f.bold());
            if (node == m_defaultNode)
                f.setItalic(f.style() != QFont::StyleItalic);
            return f;
        } else if (role == Qt::DisplayRole) {
            QString baseName = node->newName.isEmpty() ? node->profile->displayName() : node->newName;
            if (node == m_defaultNode)
                //: Mark up a profile as the default one.
                baseName = tr("%1 (default)").arg(baseName);
            return baseName;
        } else if (role == Qt::EditRole) {
            return node->newName.isEmpty() ? node->profile->displayName() : node->newName;
        } else if (role == Qt::DecorationRole) {
            return node->profile->isValid() ? QIcon() : warningIcon;
        } else if (role == Qt::ToolTipRole) {
            return node->profile->toHtml();
        }
    }
    return QVariant();
}

bool ProfileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    ProfileNode *node = static_cast<ProfileNode *>(index.internalPointer());
    Q_ASSERT(node);
    if (index.column() != 0 || !node->profile || role != Qt::EditRole)
        return false;
    node->newName = value.toString();
    if (!node->newName.isEmpty() && node->newName != node->profile->displayName())
        node->changed = true;
    return true;
}

Qt::ItemFlags ProfileModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    ProfileNode *node = static_cast<ProfileNode *>(index.internalPointer());
    Q_ASSERT(node);
    if (!node->profile)
        return Qt::ItemIsEnabled;

    if (node->profile->isAutoDetected())
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QVariant ProfileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return tr("Name");
    return QVariant();
}

Profile *ProfileModel::profile(const QModelIndex &index)
{
    if (!index.isValid())
        return 0;
    ProfileNode *node = static_cast<ProfileNode *>(index.internalPointer());
    Q_ASSERT(node);
    return node->profile;
}

QModelIndex ProfileModel::indexOf(Profile *p) const
{
    ProfileNode *n = find(p);
    return n ? index(n) : QModelIndex();
}

void ProfileModel::setDefaultProfile(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    ProfileNode *node = static_cast<ProfileNode *>(index.internalPointer());
    Q_ASSERT(node);
    if (node->profile)
        setDefaultNode(node);
}

bool ProfileModel::isDefaultProfile(const QModelIndex &index)
{
    return m_defaultNode == static_cast<ProfileNode *>(index.internalPointer());
}

ProfileConfigWidget *ProfileModel::widget(const QModelIndex &index)
{
    if (!index.isValid())
        return 0;
    ProfileNode *node = static_cast<ProfileNode *>(index.internalPointer());
    Q_ASSERT(node);
    return node->widget;
}

bool ProfileModel::isDirty() const
{
    foreach (ProfileNode *n, m_manualRoot->childNodes) {
        if (n->changed)
            return true;
    }
    return false;
}

bool ProfileModel::isDirty(Profile *p) const
{
    ProfileNode *n = find(p);
    return n ? !n->changed : false;
}

void ProfileModel::setDirty()
{
    ProfileConfigWidget *w = qobject_cast<ProfileConfigWidget *>(sender());
    foreach (ProfileNode *n, m_manualRoot->childNodes) {
        if (n->widget == w) {
            n->changed = true;
            emit dataChanged(index(n, 0), index(n, columnCount(QModelIndex())));
        }
    }
}

void ProfileModel::apply()
{
    // Remove unused profiles:
    QList<ProfileNode *> nodes = m_toRemoveList;
    foreach (ProfileNode *n, nodes) {
        Q_ASSERT(!n->parent);
        ProfileManager::instance()->deregisterProfile(n->profile);
    }
    Q_ASSERT(m_toRemoveList.isEmpty());

    // Update profiles:
    foreach (ProfileNode *n, m_manualRoot->childNodes) {
        Q_ASSERT(n);
        Q_ASSERT(n->profile);
        if (n->changed) {
            ProfileManager::instance()->blockSignals(true);
            if (!n->newName.isEmpty()) {
                n->profile->setDisplayName(n->newName);
                n->newName.clear();
            }
            if (n->widget)
                n->widget->apply();
            n->changed = false;

            ProfileManager::instance()->blockSignals(false);
            ProfileManager::instance()->notifyAboutUpdate(n->profile);
            emit dataChanged(index(n, 0), index(n, columnCount(QModelIndex())));
        }
    }

    // Add new (and already updated) profiles
    QStringList removedSts;
    nodes = m_toAddList;
    foreach (ProfileNode *n, nodes) {
        if (!ProfileManager::instance()->registerProfile(n->profile))
            removedSts << n->profile->displayName();
    }

    foreach (ProfileNode *n, m_toAddList)
        markForRemoval(n->profile);

    if (removedSts.count() == 1) {
        QMessageBox::warning(0,
                             tr("Duplicate profiles detected"),
                             tr("The following profile was already configured:<br>"
                                "&nbsp;%1<br>"
                                "It was not configured again.")
                             .arg(removedSts.at(0)));

    } else if (!removedSts.isEmpty()) {
        QMessageBox::warning(0,
                             tr("Duplicate profile detected"),
                             tr("The following profiles were already configured:<br>"
                                "&nbsp;%1<br>"
                                "They were not configured again.")
                             .arg(removedSts.join(QLatin1String(",<br>&nbsp;"))));
    }

    // Set default profile:
    if (m_defaultNode)
        ProfileManager::instance()->setDefaultProfile(m_defaultNode->profile);
}

void ProfileModel::markForRemoval(Profile *p)
{
    ProfileNode *node = find(p);
    if (!node)
        return;

    if (node == m_defaultNode) {
        ProfileNode *newDefault = 0;
        if (!m_autoRoot->childNodes.isEmpty())
            newDefault = m_autoRoot->childNodes.at(0);
        else if (!m_manualRoot->childNodes.isEmpty())
            newDefault = m_manualRoot->childNodes.at(0);
        setDefaultNode(newDefault);
    }

    beginRemoveRows(index(m_manualRoot), m_manualRoot->childNodes.indexOf(node), m_manualRoot->childNodes.indexOf(node));
    m_manualRoot->childNodes.removeOne(node);
    node->parent = 0;
    if (m_toAddList.contains(node)) {
        delete node->profile;
        node->profile = 0;
        m_toAddList.removeOne(node);
        delete node;
    } else {
        m_toRemoveList.append(node);
    }
    endRemoveRows();
}

void ProfileModel::markForAddition(Profile *p)
{
    int pos = m_manualRoot->childNodes.size();
    beginInsertRows(index(m_manualRoot), pos, pos);

    ProfileNode *node = createNode(m_manualRoot, p, true);
    m_toAddList.append(node);

    if (!m_defaultNode)
        setDefaultNode(node);

    endInsertRows();
}

QModelIndex ProfileModel::index(ProfileNode *node, int column) const
{
    if (node->parent == 0) // is root (or was marked for deletion)
        return QModelIndex();
    else if (node->parent == m_root)
        return index(m_root->childNodes.indexOf(node), column, QModelIndex());
    else
        return index(node->parent->childNodes.indexOf(node), column, index(node->parent));
}

ProfileNode *ProfileModel::find(Profile *p) const
{
    foreach (ProfileNode *n, m_autoRoot->childNodes) {
        if (n->profile == p)
            return n;
    }
    foreach (ProfileNode *n, m_manualRoot->childNodes) {
        if (n->profile == p)
            return n;
    }
    return 0;
}

ProfileNode *ProfileModel::createNode(ProfileNode *parent, Profile *p, bool changed)
{
    ProfileNode *node = new ProfileNode(parent, p, changed);
    if (node->widget) {
        m_parentLayout->addWidget(node->widget);
        connect(node->widget, SIGNAL(dirty()),
                this, SLOT(setDirty()));
    }
    return node;
}

void ProfileModel::setDefaultNode(ProfileNode *node)
{
    if (m_defaultNode) {
        QModelIndex idx = index(m_defaultNode);
        if (idx.isValid())
            emit dataChanged(idx, idx);
    }
    m_defaultNode = node;
    if (m_defaultNode) {
        QModelIndex idx = index(m_defaultNode);
        if (idx.isValid())
            emit dataChanged(idx, idx);
    }
}

void ProfileModel::addProfile(Profile *p)
{
    QList<ProfileNode *> nodes = m_toAddList;
    foreach (ProfileNode *n, nodes) {
        if (n->profile == p) {
            m_toAddList.removeOne(n);
            // do not delete n: Still used elsewhere!
            return;
        }
    }

    ProfileNode *parent = m_manualRoot;
    if (p->isAutoDetected())
        parent = m_autoRoot;
    int row = parent->childNodes.count();

    beginInsertRows(index(parent), row, row);
    createNode(parent, p, false);
    endInsertRows();

    emit profileStateChanged();
}

void ProfileModel::removeProfile(Profile *p)
{
    QList<ProfileNode *> nodes = m_toRemoveList;
    foreach (ProfileNode *n, nodes) {
        if (n->profile == p) {
            m_toRemoveList.removeOne(n);
            delete n;
            return;
        }
    }

    ProfileNode *parent = m_manualRoot;
    if (p->isAutoDetected())
        parent = m_autoRoot;
    int row = 0;
    ProfileNode *node = 0;
    foreach (ProfileNode *current, parent->childNodes) {
        if (current->profile == p) {
            node = current;
            break;
        }
        ++row;
    }

    beginRemoveRows(index(parent), row, row);
    parent->childNodes.removeAt(row);
    delete node;
    endRemoveRows();

    emit profileStateChanged();
}

void ProfileModel::updateProfile(Profile *p)
{
    ProfileNode *n = find(p);
    // This can happen if Qt Versions and Profiles are removed simultaneously.
    if (!n)
        return;
    if (n->widget)
        n->widget->discard();
    QModelIndex idx = index(n);
    emit dataChanged(idx, idx);
}

void ProfileModel::changeDefaultProfile()
{
    setDefaultNode(find(ProfileManager::instance()->defaultProfile()));
}

} // namespace Internal
} // namespace ProjectExplorer
