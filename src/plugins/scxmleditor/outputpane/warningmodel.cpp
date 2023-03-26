// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scxmleditorconstants.h"
#include "scxmleditortr.h"
#include "warningmodel.h"

#include <utils/algorithm.h>

#include <QTimer>

using namespace ScxmlEditor::OutputPane;

WarningModel::WarningModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_countChecker = new QTimer(this);
    m_countChecker->setInterval(500);
    m_countChecker->setSingleShot(true);
    connect(m_countChecker.data(), &QTimer::timeout, this, [this] {
        if (m_warnings.count() != m_oldCount) {
            m_oldCount = m_warnings.count();
            emit countChanged(m_oldCount);
        }
    });
}

WarningModel::~WarningModel()
{
    delete m_countChecker;

    // Clear warnings
    clear(false);
}

void WarningModel::clear(bool sendSignal)
{
    emit modelAboutToBeClear();

    for (Warning *w : std::as_const(m_warnings))
        w->disconnect(this);

    beginResetModel();
    qDeleteAll(m_warnings);
    m_warnings.clear();
    endResetModel();

    if (m_countChecker)
        m_countChecker->start();

    if (sendSignal) {
        emit warningsChanged();
        emit modelCleared();
    }
}

int WarningModel::count(Warning::Severity type) const
{
    return Utils::count(m_warnings, [type](Warning *w) {
        return w->severity() == type;
    });
}

int WarningModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_warnings.count();
}

int WarningModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 4;
}

QVariant WarningModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return Tr::tr("Severity");
        case 1:
            return Tr::tr("Type");
        case 2:
            return Tr::tr("Reason");
        case 3:
            return Tr::tr("Description");
        default:
            break;
        }
    }

    return QVariant();
}

QString WarningModel::severityName(Warning::Severity severity) const
{
    switch (severity) {
    case Warning::ErrorType:
        return Tr::tr("Error");
    case Warning::WarningType:
        return Tr::tr("Warning");
    case Warning::InfoType:
        return Tr::tr("Info");
    default:
        return Tr::tr("Unknown");
    }
}

QIcon WarningModel::severityIcon(Warning::Severity severity) const
{
    switch (severity) {
    case Warning::ErrorType: {
        static const QIcon errorIcon(":/scxmleditor/images/error.png");
        return errorIcon;
    }
    case Warning::WarningType: {
        static const QIcon warningIcon(":/scxmleditor/images/warning.png");
        return warningIcon;
    }
    case Warning::InfoType: {
        static const QIcon infoIcon(":/scxmleditor/images/warning_low.png");
        return infoIcon;
    }
    default:
        return QIcon();
    }
}

QVariant WarningModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();

    if (index.isValid() && row >= 0 && row < m_warnings.count()) {
        Warning *it = m_warnings[row];

        switch (role) {
        case WarningModel::FilterRole:
            return QString::fromLatin1(it->isActive() ? Constants::C_WARNINGMODEL_FILTER_ACTIVE
                                                      : Constants::C_WARNINGMODEL_FILTER_NOT);
        case Qt::DecorationRole: {
            if (col == 0)
                return severityIcon(it->severity());
            break;
        }
        case Qt::ToolTipRole: {
            return Tr::tr("Severity:\t%1\nType:     \t%2\nReason: \t%3\nDescription:\t%4")
                .arg(severityName(it->severity()))
                .arg(it->typeName())
                .arg(it->reason())
                .arg(it->description());
        }
        case Qt::DisplayRole: {
            switch (col) {
            case 0:
                return severityName(it->severity());
            case 1:
                return it->typeName();
            case 2:
                return it->reason();
            case 3:
                return it->description();
            default:
                break;
            }
        }
        default:
            break;
        }
    }
    return QVariant();
}

void WarningModel::setShowWarnings(int type, bool show)
{
    //beginResetModel();
    m_warningVisibilities[type] = show;
    for (int i = 0; i < m_warnings.count(); ++i)
        m_warnings[i]->setActive(m_warningVisibilities.value(m_warnings[i]->severity(), true));
    //endResetModel();
}

Warning *WarningModel::createWarning(Warning::Severity severity, const QString &type, const QString &reason, const QString &description)
{
    beginInsertRows(QModelIndex(), m_warnings.count(), m_warnings.count());
    auto warning = new Warning(severity, type, reason, description, m_warningVisibilities.value(severity, true));
    connect(warning, &Warning::destroyed, this, &WarningModel::warningDestroyed);
    connect(warning, &Warning::dataChanged, this, [=] {
        emit warningsChanged();
        QModelIndex ind = createIndex(m_warnings.indexOf(warning), 0);
        emit dataChanged(ind, ind);
    });

    m_warnings << warning;
    endInsertRows();

    emit warningsChanged();

    m_countChecker->start();
    return warning;
}

Warning *WarningModel::getWarning(int row)
{
    if (row >= 0 && row < m_warnings.count())
        return m_warnings[row];

    return nullptr;
}

Warning *WarningModel::getWarning(const QModelIndex &ind)
{
    if (ind.isValid())
        return getWarning(ind.row());

    return nullptr;
}

void WarningModel::warningDestroyed(QObject *w)
{
    // Intentional static_cast.
    // The Warning is being destroyed, so qobject_cast doesn't work anymore.
    const int ind = m_warnings.indexOf(static_cast<Warning *>(w));
    if (ind >= 0) {
        beginRemoveRows(QModelIndex(), ind, ind);
        m_warnings.removeAt(ind);
        endRemoveRows();
    }

    m_countChecker->start();
    emit warningsChanged();
}
