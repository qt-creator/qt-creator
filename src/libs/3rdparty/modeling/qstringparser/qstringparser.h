/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QSTRINGPARSER_H
#define QSTRINGPARSER_H

#include <QString>
#include <QList>


class QStringParser
{
private:

    class Parser
    {
    private:

        class Node
        {
        public:
            virtual ~Node() { }
            virtual bool accept(Parser &visitor, int *index) = 0;
        };

        template<typename V>
        class RefNode :
        public Node
        {
        public:
            RefNode(V &v) : m_v(v) { }
            bool accept(Parser &visitor, int *index) { return visitor.visit(this, index); }
            V &ref() const { return m_v; }
        private:
            V &m_v;
        };

        template<class U, typename V>
        class SetterNode :
        public Node
        {
        public:
            SetterNode(U &u, void (U::*setter)(V)) : m_object(u), m_setter(setter) { }
            bool accept(Parser &visitor, int *index) { return visitor.visit(this, index); }
            U &object() const { return m_object; }
            void (U::*setter() const)(V) { return m_setter; }
        private:
            U &m_object;
            void (U::*m_setter)(V);
        };

    public:

        Parser(const QString &source, const QString &pattern);
        ~Parser();

    public:

        template<typename V>
        Parser &arg(V &v)
        {
            m_nodes.append(new RefNode<V>(v));
            return *this;
        }

        template<class U, typename V>
        Parser &arg(U &u, void (U::*setter)(V))
        {
            m_nodes.append(new SetterNode<U, V>(u, setter));
            return *this;
        }

        bool failed();

    private:

        bool scan(int *i, int *index);
        bool scan(double *d, int *index);

        template<typename V>
        bool visit(RefNode<V> *node, int *index)
        {
            V v = 0;
            if (!scan(&v, index)) {
                return false;
            }
            node->ref() = v;
            return true;
        }

        template<class U, typename V>
        bool visit(SetterNode<U, V> *node, int *index)
        {
            V v = 0;
            if (!scan(&v, index)) {
                return false;
            }
            (node->object().*(node->setter()))(v);
            return true;
        }

        template<class U, typename V>
        bool visit(SetterNode<U, const V &> *node, int *index)
        {
            V v = 0;
            if (!scan(&v, index)) {
                return false;
            }
            (node->object().*(node->setter()))(v);
            return true;
        }

        void evaluate();

    private:

        const QString m_source;
        const QString m_pattern;
        bool m_evaluated;
        bool m_evaluationFailed;
        QList<Node *> m_nodes;
    };

public:

    explicit QStringParser(const QString &source);
    ~QStringParser();

public:

    Parser parse(const QString &pattern);

private:

    const QString m_source;
};

#endif // QSTRINGPARSER_H
