/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#pragma once

#include "perfprofiler_global.h"
#include <utils/qtcassert.h>

#include <vector>
#include <map>

namespace PerfProfiler {
namespace Internal {

struct NoPayload
{
    void adjust(qint64) {}

    void countObservedRelease() {}
    void countGuessedRelease() {}

    void countObservedAllocation() {}
    void countLostRequest() {}
};

template<typename Payload>
class ResourceBlock {
public:
    ResourceBlock(qint64 size = -1, Payload &&payload = Payload())
        : m_size(size), m_payload(std::move(payload))
    {}

    void setSize(quint64 size) { m_size = size; }
    qint64 size() const { return m_size; }

    Payload &payload() { return m_payload; }

private:
    qint64 m_size;
    Payload m_payload;
};

template<typename Payload, quint64 InvalidId>
class PendingRequestsContainer
{
public:
    class Block : public ResourceBlock<Payload>
    {
    public:
        Block(qint64 size = -1, Payload &&payload = Payload(), quint64 releaseId = InvalidId)
            : ResourceBlock<Payload>(size, std::move(payload)), m_releaseId(releaseId)
        {}

        Block(Block &&other) = default;
        Block &operator=(Block &&other) = default;

        quint64 releaseId() const
        {
            return m_releaseId;
        }

        void addRecursiveAllocation(quint64 id, qint64 size)
        {
            insert(m_recursiveAllocations, id, size);
        }

        void addRecursiveRelease(quint64 id, qint64 size)
        {
            insert(m_recursiveReleases, id, size);
        }

        bool hasRecursiveAllocation(quint64 id) const
        {
            return contains(m_recursiveAllocations, id);
        }

        bool hasRecursiveRelease(quint64 id) const
        {
            return contains(m_recursiveReleases, id);
        }

        void mergeRecursives(const Block &other)
        {
            for (auto it = other.m_recursiveAllocations.begin(),
                 end = other.m_recursiveAllocations.end();
                 it != end; ++it) {
                addRecursiveAllocation(it->first, it->second);
            }

            for (auto it = other.m_recursiveReleases.begin(),
                 end = other.m_recursiveReleases.end();
                 it != end; ++it) {
                addRecursiveRelease(it->first, it->second);
            }
        }

    private:
        static void insert(std::map<quint64, qint64> &addressSpace, quint64 id, qint64 size)
        {
            auto it = addressSpace.upper_bound(id);
            if (it != addressSpace.end()) {
                if (it->first < id + size) {
                    size = qMax(it->first + it->second, id + size) - id;
                    it = addressSpace.erase(it);
                }
            }
            if (it != addressSpace.begin()) {
                --it;
                if (id < it->first + it->second) {
                    it->second = qMax(it->first + it->second, id + size) - it->first;
                    return;
                }
                ++it;
            }
            addressSpace.emplace_hint(it, id, size);
        }

        static bool contains(const std::map<quint64, qint64> &addressSpace, quint64 id)
        {
            auto it = addressSpace.upper_bound(id);
            if (it == addressSpace.begin())
                return false;
            --it;
            return (id < it->first + it->second);
        }

        quint64 m_releaseId;
        std::map<quint64, qint64> m_recursiveAllocations;
        std::map<quint64, qint64> m_recursiveReleases;
    };

    using Container = std::vector<Block>;

    void reserve(int num)
    {
        m_pendingRequests.reserve(num);
    }

    void push(qint64 amount, Payload &&payload, quint64 releaseId)
    {
        m_pendingRequests.emplace_back(amount, std::move(payload), releaseId);
    }

    Block &back()
    {
        return m_pendingRequests.back();
    }

    void popBack()
    {
        const Block last = std::move(m_pendingRequests.back());
        m_pendingRequests.pop_back();
        if (!m_pendingRequests.empty())
            m_pendingRequests.back().mergeRecursives(last);
    }

    void clear()
    {
        m_pendingRequests.clear();
    }

    bool isEmpty() const
    {
        return m_pendingRequests.empty();
    }

    void allocate(quint64 id, qint64 size)
    {
        if (!m_pendingRequests.empty())
            m_pendingRequests.back().addRecursiveAllocation(id, size);
    }

    void release(quint64 id, qint64 size)
    {
        if (!m_pendingRequests.empty())
            m_pendingRequests.back().addRecursiveRelease(id, size);
    }

    bool hasRelease(quint64 id) const
    {
        return m_pendingRequests.empty() ? false
                                         : m_pendingRequests.back().hasRecursiveRelease(id);
    }

    bool hasAllocation(quint64 id) const
    {
        return m_pendingRequests.empty() ? false
                                         : m_pendingRequests.back().hasRecursiveAllocation(id);
    }

private:
    Container m_pendingRequests;
};

template<typename Payload = NoPayload, quint64 InvalidId = 0>
class PerfResourceCounter
{
public:
    using Block = ResourceBlock<Payload>;
    using Container = std::map<quint64, Block>;

    PerfResourceCounter(Container *blocks = nullptr) : m_blocks(blocks)
    {
        // Make sure we don't need to reallocate all the time.
        m_pendingRequests.reserve(10);
    }

    PerfResourceCounter(const PerfResourceCounter &other) = delete;
    PerfResourceCounter &operator=(const PerfResourceCounter &other) = delete;

    PerfResourceCounter(PerfResourceCounter &&other) = default;
    PerfResourceCounter &operator=(PerfResourceCounter &&other) = default;

    void request(qint64 amount, Payload &&payload = Payload())
    {
        QTC_ASSERT(amount >= 0, return);
        m_pendingRequests.push(amount, std::move(payload), InvalidId);
    }

    void request(qint64 amount, quint64 releaseId, Payload &&payload = Payload())
    {
        QTC_ASSERT(amount >= 0, return);
        m_pendingRequests.push(amount, std::move(payload), releaseId);
    }

    void obtain(quint64 id, Payload &&payload = Payload())
    {
        if (m_pendingRequests.isEmpty()) {
            if (id != InvalidId)
                insertLostBlock(id, std::move(payload));
        } else {
            if (id != InvalidId)
                doObtain(id, std::move(m_pendingRequests.back()), std::move(payload));
            m_pendingRequests.popBack();
        }
    }

    void move(quint64 id, Payload &&payload = Payload())
    {
        if (m_pendingRequests.isEmpty()) {
            if (id != InvalidId) {
                ++m_numGuessedReleases;
                payload.countGuessedRelease();
                insertLostBlock(id, std::move(payload));
            }
        } else {
            typename PendingRequestsContainer<Payload, InvalidId>::Block &block
                    = m_pendingRequests.back();

            if (id != InvalidId) {
                doRelease(block.releaseId(), payload);
                doObtain(id, std::move(block), std::move(payload));
            }
            m_pendingRequests.popBack();
        }
    }

    void release(quint64 id, Payload &&payload = Payload())
    {
        if (id == InvalidId)
            return;

        doRelease(id, payload);
    }

    qint64 currentTotal() const
    {
        return m_observedAllocated + m_guessedAllocated - m_observedReleased - m_guessedReleased;
    }

    qint64 currentNumObservations() const
    {
        return m_numObservedAllocations + m_numObservedReleases;
    }

    qint64 currentNumGuesses() const
    {
        return m_numGuessedAllocations + m_numGuessedReleases;
    }

    qint64 minTotal() const
    {
        return m_minTotal;
    }

    qint64 maxTotal() const
    {
        return m_maxTotal;
    }

    void clear()
    {
        m_pendingRequests.clear();
        m_observedAllocated = m_guessedAllocated = 0;
        m_observedReleased = m_guessedReleased = 0;
        m_numObservedAllocations = m_numGuessedAllocations = 0;
        m_numObservedReleases = m_numGuessedReleases = 0;
        m_minTotal = m_maxTotal = 0;
    }

    bool isEmpty() const
    {
        return m_pendingRequests.isEmpty()
                && m_observedAllocated == 0 && m_guessedAllocated == 0
                && m_observedReleased == 0 && m_guessedReleased == 0
                && m_numObservedAllocations == 0 && m_numGuessedAllocations == 0
                && m_numObservedReleases == 0 && m_numGuessedReleases == 0
                && m_minTotal == 0 && m_maxTotal == 0;
    }

private:
    void doObtain(quint64 id, Block &&block, Payload &&payload)
    {
        ++m_numObservedAllocations;
        m_observedAllocated += block.size();
        block.payload().adjust(block.size());
        block.payload().countObservedAllocation();

        auto it = m_blocks->upper_bound(id);
        if (it != m_blocks->begin())
            --it;

        makeSpace(it, id, id + block.size(), payload);
        m_blocks->emplace_hint(it, id, std::move(block));

        m_maxTotal = qMax(m_maxTotal, currentTotal());
    }

    void makeSpace(typename Container::iterator &it, quint64 blockBegin, quint64 blockEnd,
                   Payload &payload)
    {
        const auto end = m_blocks->end();
        if (it == end)
            return;

        if (it->first <= blockBegin) {
            if (it->first + it->second.size() > blockBegin) {
                Payload &itPayload = it->second.payload();
                if (m_pendingRequests.hasAllocation(it->first)) {
                    const qint64 newSize = blockBegin - it->first;
                    if (newSize > 0) {
                        itPayload.adjust(newSize - it->second.size());
                        m_observedAllocated -= it->second.size() - newSize;
                        it->second.setSize(newSize);
                        ++it;
                    } else {
                        itPayload.adjust(-(it->second.size()));
                        m_observedAllocated -= it->second.size();
                        it = m_blocks->erase(it);
                    }
                } else {
                    itPayload.adjust(-(it->second.size()));
                    m_guessedReleased += it->second.size();
                    it = m_blocks->erase(it);
                    payload.countGuessedRelease();
                }
            } else {
                ++it;
            }
        }

        while (it != end && blockEnd > it->first) {
            Payload &itPayload = it->second.payload();
            if (m_pendingRequests.hasAllocation(it->first)) {
                const qint64 diff = it->first + it->second.size() - blockEnd;
                if (diff > 0) {
                    itPayload.adjust(diff - it->second.size());
                    m_observedAllocated -= it->second.size() - diff;
                    Block newBlock(diff, std::move(itPayload));
                    it = m_blocks->erase(it);
                    it = ++(m_blocks->emplace_hint(it, blockEnd, std::move(newBlock)));
                } else {
                    itPayload.adjust(-(it->second.size()));
                    m_observedAllocated -= it->second.size();
                    it = m_blocks->erase(it);
                }
            } else {
                itPayload.adjust(-(it->second.size()));
                m_guessedReleased += it->second.size();
                it = m_blocks->erase(it);
                payload.countGuessedRelease();
            }
        }
    }

    void insertLostBlock(quint64 id, Payload &&payload)
    {
        ++m_numGuessedAllocations;
        m_guessedAllocated += 1;
        payload.adjust(1);
        payload.countLostRequest();

        auto it = m_blocks->upper_bound(id);
        if (it != m_blocks->begin())
            --it;

        makeSpace(it, id, id + 1, payload);
        m_blocks->emplace_hint(it, id, Block(1, std::move(payload)));
    }

    void doRelease(quint64 id, Payload &payload)
    {
        auto it = m_blocks->lower_bound(id);
        if (it != m_blocks->end() && it->first == id) {
            m_pendingRequests.release(it->first, it->second.size());
            m_observedReleased += it->second.size();
            it->second.payload().adjust(-(it->second.size()));
            m_blocks->erase(it);
            payload.countObservedRelease();
            ++m_numObservedReleases;
        } else if (it != m_blocks->begin()) {
            --it;
            if (it->first + it->second.size() > id) {
                m_pendingRequests.release(it->first, it->second.size());
                m_guessedReleased += it->second.size();
                it->second.payload().adjust(-(it->second.size()));
                m_blocks->erase(it);
                payload.countGuessedRelease();
                ++m_numGuessedReleases;
            }
        } else if (!m_pendingRequests.hasRelease(id)) {
            payload.countGuessedRelease();
            ++m_numGuessedReleases;
        }

        m_minTotal = qMin(m_minTotal, currentTotal());
    }

    Container *m_blocks;

    PendingRequestsContainer<Payload, InvalidId> m_pendingRequests;

    qint64 m_observedAllocated = 0;
    qint64 m_guessedAllocated = 0;

    qint64 m_numObservedAllocations = 0;
    qint64 m_numGuessedAllocations = 0;

    qint64 m_observedReleased = 0;
    qint64 m_guessedReleased = 0;

    qint64 m_numObservedReleases = 0;
    qint64 m_numGuessedReleases = 0;

    qint64 m_minTotal = 0;
    qint64 m_maxTotal = 0;
};

} // namespace Internal
} // namespace PerfProfiler
