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

#ifndef QMLJSDELTA_H
#define QMLJSDELTA_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljs_global.h>

namespace QmlJS {

class QMLJS_EXPORT Delta
{
public:
    typedef int DebugId;
    typedef QHash<AST::UiObjectMember*, QList<DebugId> > DebugIdMap;
    DebugIdMap operator()(const QmlJS::Document::Ptr &doc1, const QmlJS::Document::Ptr &doc2, const DebugIdMap &debugIds);

    QSet<AST::UiObjectMember *> newObjects;

    QmlJS::Document::Ptr document() const;
    QmlJS::Document::Ptr previousDocument() const;

private:
    void insert(AST::UiObjectMember *member, AST::UiObjectMember *parentMember,
                const QList<DebugId> &debugReferences, const Document::Ptr &doc);
    void update(AST::UiObjectMember* oldObject, const QmlJS::Document::Ptr& oldDoc,
                AST::UiObjectMember* newObject, const QmlJS::Document::Ptr& newDoc,
                const QList<DebugId>& debugReferences);
    void remove(const QList<DebugId> &debugReferences);
    void reparent(const QList <DebugId> &member, const QList<DebugId> &newParent);

protected:
    virtual void updateScriptBinding(DebugId objectReference,
                             AST::UiObjectMember *parentObject,
                             AST::UiScriptBinding *scriptBinding,
                             const QString &propertyName,
                             const QString &scriptCode);
    virtual void updateMethodBody(DebugId objectReference,
                            AST::UiObjectMember *parentObject,
                            AST::UiScriptBinding *scriptBinding,
                            const QString &methodName,
                            const QString &methodBody);
    virtual void resetBindingForObject(int debugId, const QString &propertyName);
    virtual void removeObject(int debugId);
    virtual void reparentObject(int debugId, int newParent);
    virtual void createObject(const QString &qmlText, DebugId ref,
                              const QStringList &importList, const QString &filename);
    virtual void notifyUnsyncronizableElementChange(AST::UiObjectMember *parent);

private:
    QmlJS::Document::Ptr m_currentDoc;
    QmlJS::Document::Ptr m_previousDoc;
};

} // namespace QmlJS

#endif // QMLJSDELTA_H

