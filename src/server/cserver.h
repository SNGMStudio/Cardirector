/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of Cardirector.

    This game engine is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the LICENSE file for more details.

    Mogara
*********************************************************************/

#ifndef CSERVER_H
#define CSERVER_H

#include "cglobal.h"

#include <QObject>
#include <QHostAddress>

class CAbstractPacketParser;
class CRoom;
class CServerUser;
class CTcpServer;
class CTcpSocket;

class CServerPrivate;
class MCD_EXPORT CServer : public QObject
{
    Q_OBJECT

public:
    CServer(QObject *parent = 0);
    ~CServer();

    bool listen(const QHostAddress &address = QHostAddress::Any, ushort port = 0);
    QHostAddress address() const;
    ushort port() const;

    void setAcceptMultipleClientsBehindOneIp(bool enabled);
    bool acceptMultipleClientsBehindOneIp() const;

    CServerUser *findUser(uint id);
    QHash<uint, CServerUser *> users() const;

    void createRoom(CServerUser *owner, const QVariant &config);
    CRoom *findRoom(uint id) const;
    QHash<uint, CRoom *> rooms() const;
    CRoom *lobby() const;

    void updateRoomList(CServerUser *user);

    void setPacketParser(CAbstractPacketParser *parser);
    CAbstractPacketParser *packetParser() const;

    void broadcastNotification(int command, const QVariant &data = QVariant(), CServerUser *except = NULL);

signals:
    void roomCreated(CRoom *room);
    void userAdded(CServerUser *user);

protected:
    void handleNewConnection(CTcpSocket *client);

    void onRoomAbandoned();
    void onUserDisconnected();
    void onUserStateChanged();

private:
    C_DISABLE_COPY(CServer)
    C_DECLARE_PRIVATE(CServer)
    CServerPrivate *p_ptr;
};

#endif
