/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2026  qBittorrent contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library), and
 * distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version of the file(s).
 */

#pragma once

#include <algorithm>
#include <limits>

#include <QList>
#include <QSet>
#include <QString>
#include <QtGlobal>

namespace BitTorrent::DiskSpaceAdmission
{
    struct Candidate
    {
        QString id;
        qint64 pendingBytes = 0;
        int queuePosition = -1;
        bool previouslyAdmitted = false;
    };

    // Estimates which torrents can complete without consuming the free-space floor.
    // This does not reserve physical filesystem blocks.
    inline QSet<QString> select(const QList<Candidate> &candidates, const qint64 freeBytes, const qint64 minFreeBytes)
    {
        QSet<QString> admitted;
        if ((freeBytes < 0) || (minFreeBytes <= 0))
        {
            for (const Candidate &candidate : candidates)
                admitted.insert(candidate.id);
            return admitted;
        }

        if (freeBytes < minFreeBytes)
            return admitted;

        qint64 budget = freeBytes - minFreeBytes;
        QList<Candidate> ordered = candidates;
        std::ranges::sort(ordered, [](const Candidate &lhs, const Candidate &rhs)
        {
            const auto priority = [](const Candidate &candidate)
            {
                return (candidate.queuePosition >= 0) ? candidate.queuePosition : std::numeric_limits<int>::max();
            };
            if (priority(lhs) != priority(rhs))
                return priority(lhs) < priority(rhs);
            if (lhs.previouslyAdmitted != rhs.previouslyAdmitted)
                return lhs.previouslyAdmitted;
            return lhs.id < rhs.id;
        });

        for (const Candidate &candidate : ordered)
        {
            if ((candidate.pendingBytes >= 0) && (candidate.pendingBytes <= budget))
            {
                admitted.insert(candidate.id);
                budget -= candidate.pendingBytes;
            }
        }
        return admitted;
    }
}
