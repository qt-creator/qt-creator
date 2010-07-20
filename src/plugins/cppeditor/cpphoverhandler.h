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

#ifndef CPPHOVERHANDLER_H
#define CPPHOVERHANDLER_H

#include <cplusplus/CppDocument.h>
#include <utils/htmldocextractor.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QPoint;
QT_END_NAMESPACE

namespace CPlusPlus {
class LookupItem;
class LookupContext;
}

namespace Core {
class IEditor;
}

namespace CppTools {
class CppModelManagerInterface;
}

namespace TextEditor {
class ITextEditor;
class BaseTextEditor;
}

namespace CppEditor {
namespace Internal {

class CppHoverHandler : public QObject
{
    Q_OBJECT
public:
    CppHoverHandler(QObject *parent = 0);

public slots:
    void showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos);
    void updateContextHelpId(TextEditor::ITextEditor *editor, int pos);

private slots:
    void editorOpened(Core::IEditor *editor);

private:
    struct HelpCandidate
    {
        enum Category {
            ClassOrNamespace,
            Enum,
            Typedef,
            Var,
            Macro,
            Brief,
            Function
        };

        HelpCandidate(const QString &helpId, const QString &markId, Category category) :
            m_helpId(helpId), m_markId(markId), m_category(category)
        {}
        QString m_helpId;
        QString m_markId;
        Category m_category;
    };

    void resetMatchings();
    void identifyMatch(TextEditor::ITextEditor *editor, int pos);
    bool matchDiagnosticMessage(const CPlusPlus::Document::Ptr &document, unsigned line);
    bool matchIncludeFile(const CPlusPlus::Document::Ptr &document, unsigned line);
    bool matchMacroInUse(const CPlusPlus::Document::Ptr &document, unsigned pos);
    void handleLookupItemMatch(const CPlusPlus::LookupItem &lookupItem,
                               const CPlusPlus::LookupContext &lookupContext,
                               const bool assignTooltip);

    void evaluateHelpCandidates();
    bool helpIdExists(const QString &helpId) const;
    QString getDocContents() const;
    QString getDocContents(const HelpCandidate &helpCandidate) const;

    void generateDiagramTooltip(const bool integrateDocs);
    void generateNormalTooltip(const bool integrateDocs);
    void addF1ToTooltip();

    static TextEditor::BaseTextEditor *baseTextEditor(TextEditor::ITextEditor *editor);

    CppTools::CppModelManagerInterface *m_modelManager;
    int m_matchingHelpCandidate;
    QList<HelpCandidate> m_helpCandidates;
    QString m_toolTip;
    QList<QStringList> m_classHierarchy;
    Utils::HtmlDocExtractor m_htmlDocExtractor;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPHOVERHANDLER_H
