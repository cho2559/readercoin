// Copyright (c) 2016-2018 The Readercoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef READERCOIN_QT_TEST_RPCNESTEDTESTS_H
#define READERCOIN_QT_TEST_RPCNESTEDTESTS_H

#include <QObject>
#include <QTest>

#include <txdb.h>
#include <txmempool.h>

class RPCNestedTests : public QObject
{
    Q_OBJECT

    private Q_SLOTS:
    void rpcNestedTests();
};

#endif // READERCOIN_QT_TEST_RPCNESTEDTESTS_H
