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

#ifndef MANAGER_H
#define MANAGER_H

#include <coreplugin/mimedatabase.h>

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QMultiHash>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>
#include <QtCore/QFutureWatcher>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QStringList;
template <class> class QFutureInterface;
QT_END_NAMESPACE

namespace TextEditor {
namespace Internal {

class HighlightDefinition;

class Manager : public QObject
{
    Q_OBJECT
public:
    virtual ~Manager();
    static Manager *instance();

    QString definitionIdByName(const QString &name) const;
    QString definitionIdByMimeType(const QString &mimeType) const;
    bool isBuildingDefinition(const QString &id) const;
    const QSharedPointer<HighlightDefinition> &definition(const QString &id);

private slots:
    void registerMimeTypes();
    void registerMimeType(int index) const;

private:
    Manager();
    Q_DISABLE_COPY(Manager)

    void gatherDefinitionsMimeTypes(QFutureInterface<Core::MimeType> &future);
    void parseDefinitionMetadata(const QFileInfo &fileInfo,
                                 QString *comment,
                                 QStringList *mimeTypes,
                                 QStringList *patterns);

    struct PriorityCompare
    {
        bool operator()(const QString &a, const QString &b) const
        { return m_priorityById.value(a) < m_priorityById.value(b); }

        QHash<QString, int> m_priorityById;
    };
    PriorityCompare m_priorityComp;

    QFutureWatcher<Core::MimeType> m_watcher;

    QHash<QString, QString> m_idByName;
    QMultiHash<QString, QString> m_idByMimeType;
    QHash<QString, QSharedPointer<HighlightDefinition> > m_definitions;
    QSet<QString> m_isBuilding;
};

} // namespace Internal
} // namespace TextEditor

#endif // MANAGER_H
