#ifndef TESTCLIENTNETWORK_H
#define TESTCLIENTNETWORK_H

#include <QObject>
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "network/servernetwork.h"
#include "network/clientnetwork.h"

class testClientNetwork : public QObject
{
    Q_OBJECT
public:
    explicit testClientNetwork(QObject *parent = nullptr);
    ~testClientNetwork();


private slots:
    void verifyServerWorking();
    void LoginCorrectly();
    void wrongServerDetails();
    void incorrectSocket();
    void cleanupTestCase();


signals:

private:
    void testFunc();


    QString passwordServer;
    quint16 port;
    QHostAddress ip;
    QString playerName;
    ServerNetwork testServerNet1;

    ServerNetwork *testServer;

    QSignalSpy *spyServer;
    QSignalSpy *spyServerPlayerJoined;
    QSignalSpy *spyServerError;

};

#endif // TESTCLIENTNETWORK_H
