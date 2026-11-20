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

#include <QObject>
#include <QTest>

#include "base/bittorrent/diskspaceadmission.h"
#include "base/global.h"

class TestDiskSpaceAdmission final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TestDiskSpaceAdmission)

public:
    TestDiskSpaceAdmission() = default;

private slots:
    void overcommittedTorrentsCanMakeProgress() const
    {
        using BitTorrent::DiskSpaceAdmission::Candidate;
        using BitTorrent::DiskSpaceAdmission::select;
        const QList<Candidate> torrents = {
            {u"A"_s, 600, 0, true},
            {u"B"_s, 600, 1, true},
            {u"C"_s, 250, 2, false}
        };
        const auto admitted = select(torrents, 1000, 100);
        QVERIFY(admitted.contains(u"A"_s));
        QVERIFY(!admitted.contains(u"B"_s));
        QVERIFY(admitted.contains(u"C"_s));
        QCOMPARE(admitted.size(), 2);
    }

    void stableAdmittanceBreaksTies() const
    {
        using BitTorrent::DiskSpaceAdmission::Candidate;
        using BitTorrent::DiskSpaceAdmission::select;
        const QList<Candidate> torrents = {
            {u"A"_s, 600, -1, false},
            {u"B"_s, 600, -1, true}
        };
        const auto admitted = select(torrents, 700, 100);
        QVERIFY(!admitted.contains(u"A"_s));
        QVERIFY(admitted.contains(u"B"_s));
        QCOMPARE(admitted, select(torrents, 700, 100));
    }

    void honorsQueuePriority() const
    {
        using BitTorrent::DiskSpaceAdmission::Candidate;
        using BitTorrent::DiskSpaceAdmission::select;
        const QList<Candidate> torrents = {
            {u"low"_s, 600, 10, true},
            {u"high"_s, 600, 0, false}
        };
        const auto admitted = select(torrents, 700, 100);
        QVERIFY(admitted.contains(u"high"_s));
        QVERIFY(!admitted.contains(u"low"_s));
    }

    void recoveryAndUnknownSpace() const
    {
        using BitTorrent::DiskSpaceAdmission::Candidate;
        using BitTorrent::DiskSpaceAdmission::select;
        const QList<Candidate> torrents = {{u"A"_s, 600, 0, true}, {u"B"_s, 600, 1, false}};
        QVERIFY(!select(torrents, 1000, 100).contains(u"B"_s));
        QVERIFY(select(torrents, 1400, 100).contains(u"B"_s));
        QVERIFY(select(torrents, -1, 100).contains(u"B"_s));
    }

    void protectsFloorAndSkipsOversizedTorrents() const
    {
        using BitTorrent::DiskSpaceAdmission::Candidate;
        using BitTorrent::DiskSpaceAdmission::select;
        QVERIFY(select({{u"A"_s, 0, 0, true}}, 50, 100).isEmpty());
        const QList<Candidate> torrents = {{u"huge"_s, 1500, 0, true}, {u"small"_s, 200, 1, false}};
        QVERIFY(!select(torrents, 1000, 100).contains(u"huge"_s));
        QVERIFY(select(torrents, 1000, 100).contains(u"small"_s));
    }
};

QTEST_APPLESS_MAIN(TestDiskSpaceAdmission)
#include "testdiskspaceadmission.moc"
