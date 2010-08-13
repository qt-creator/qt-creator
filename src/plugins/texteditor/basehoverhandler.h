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

#ifndef BASEHOVERHANDLER_H
#define BASEHOVERHANDLER_H

#include "texteditor_global.h"

#include <utils/htmldocextractor.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QPoint;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace TextEditor {

class ITextEditor;
class BaseTextEditor;

class TEXTEDITOR_EXPORT BaseHoverHandler : public QObject
{
    Q_OBJECT
public:
    BaseHoverHandler(QObject *parent = 0);

    struct HelpCandidate
    {
        enum Category {
            ClassOrNamespace,
            Enum,
            Typedef,
            Macro,
            Brief,
            Function,
            QML,
            Unknown
        };

        HelpCandidate(const QString &helpId, Category category) :
            m_helpId(helpId), m_docMark(helpId), m_category(category)
        {}
        HelpCandidate(const QString &helpId, const QString &docMark, Category category) :
            m_helpId(helpId), m_docMark(docMark), m_category(category)
        {}
        QString m_helpId;
        QString m_docMark;
        Category m_category;
    };

private slots:
    void editorOpened(Core::IEditor *editor);

public slots:
    void showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos);
    void updateContextHelpId(TextEditor::ITextEditor *editor, int pos);

protected:
    void setToolTip(const QString &tooltip);
    const QString &toolTip() const;
    void appendToolTip(const QString &extension);
    void addF1ToToolTip();

    void addHelpCandidate(const HelpCandidate &helpCandidate);
    void setHelpCandidate(const HelpCandidate &helpCandidate, int index);
    const QList<HelpCandidate> &helpCandidates() const;
    const HelpCandidate &helpCandidate(int index) const;

    void setMatchingHelpCandidate(int index);
    int matchingHelpCandidate() const;

    bool helpIdExists(const QString &helpId) const;

    QString getDocContents(const bool extended);
    QString getDocContents(const HelpCandidate &help, const bool extended);

    static BaseTextEditor *baseTextEditor(ITextEditor *editor);
    static bool extendToolTips(ITextEditor *editor);

private:
    void reset();
    void process(ITextEditor *editor, int pos);

    virtual bool acceptEditor(Core::IEditor *editor) = 0;
    virtual void identifyMatch(ITextEditor *editor, int pos) = 0;

    virtual void resetExtras();
    virtual void evaluateHelpCandidates();
    virtual void decorateToolTip(ITextEditor *editor);
    virtual void operateTooltip(ITextEditor *editor, const QPoint &point);

    int m_matchingHelpCandidate;
    QList<HelpCandidate> m_helpCandidates;
    QString m_toolTip;
    Utils::HtmlDocExtractor m_htmlDocExtractor;
};

} // namespace TextEditor

#endif // BASEHOVERHANDLER_H
