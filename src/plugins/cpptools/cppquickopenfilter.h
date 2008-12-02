/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef CPPQUICKOPENFILTER_H
#define CPPQUICKOPENFILTER_H

#include "cppmodelmanager.h"
#include <cplusplus/CppDocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <quickopen/iquickopenfilter.h>
#include <QtGui/QIcon>
#include <QFile>
#include <QMetaType>

namespace CppTools {
namespace Internal {

struct ModelItemInfo
{
    enum ItemType { Enum, Class, Method };

    ModelItemInfo()
    { }

    ModelItemInfo(const QString &symbolName,
                  const QString &symbolType,
                  ItemType type,
                  const QString &fileName,
                  int line,
                  const QIcon &icon)
        : symbolName(symbolName),
          symbolType(symbolType),
          type(type),
          fileName(fileName),
          line(line),
          icon(icon)
    { }

    QString symbolName;
    QString symbolType;
    ItemType type;
    QString fileName;
    int line;
    QIcon icon;
};

class CppQuickOpenFilter : public QuickOpen::IQuickOpenFilter
{
    Q_OBJECT
public:
    CppQuickOpenFilter(CppModelManager *manager, Core::EditorManager *editorManager);
    ~CppQuickOpenFilter();

    QString trName() const { return tr("Classes and Methods"); }
    QString name() const { return "Classes and Methods"; }
    Priority priority() const { return Medium; }
    QList<QuickOpen::FilterEntry> matchesFor(const QString &entry);
    void accept(QuickOpen::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);

private slots:
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void onAboutToRemoveFiles(const QStringList &files);

private:
    CppModelManager *m_manager;
    Core::EditorManager *m_editorManager;

    struct Info {
        Info(): dirty(true) {}
        Info(CPlusPlus::Document::Ptr doc): doc(doc), dirty(true) {}

        CPlusPlus::Document::Ptr doc;
        QList<ModelItemInfo> items;
        bool dirty;
    };

    QMap<QString, Info> m_searchList;
    QList<ModelItemInfo> m_previousResults;
    bool m_forceNewSearchList;
    QString m_previousEntry;
};

} // namespace Internal
} // namespace CppTools

Q_DECLARE_METATYPE(CppTools::Internal::ModelItemInfo)

#endif // CPPQUICKOPENFILTER_H
