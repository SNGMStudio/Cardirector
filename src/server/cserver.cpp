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

#include "cpch.h"
#include "croom.h"
#include "cserver.h"
#include "cserveruser.h"
#include "ctcpserver.h"
#include "ctcpsocket.h"
#include "cprotocol.h"
#include "cjsonpacketparser.h"

class CServerPrivate
{
public:
    bool acceptMultipleClientsBehindOneIp;
    QHash<QHostAddress, CServerUser *> clientIp;
    CTcpServer *server;
    CAbstractPacketParser *parser;

    QHash<uint, CServerUser *> users;
    CRoom *lobby;
    QHash<uint, CRoom *> rooms;
};

CServer::CServer(QObject *parent)
    : QObject(parent)
    , p_ptr(new CServerPrivate)
{
    p_ptr->parser = new CJsonPacketParser;
    p_ptr->acceptMultipleClientsBehindOneIp = true;
    p_ptr->lobby = new CRoom(this);
    connect(p_ptr->lobby, &CRoom::userAdded, this, &CServer::updateRoomList);
    p_ptr->server = new CTcpServer(this);
    connect(p_ptr->server, &CTcpServer::newSocket, this, &CServer::handleNewConnection);
}

CServer::~CServer()
{
    delete p_ptr->parser;
    delete p_ptr;
}

void CServer::setPacketParser(CAbstractPacketParser *parser)
{
    if (p_ptr->users.isEmpty()) {
        delete p_ptr->parser;
        p_ptr->parser = parser;
    } else {
        qWarning("Packet parser can't be switched after the server starts.");
    }
}

CAbstractPacketParser *CServer::packetParser() const
{
    return p_ptr->parser;
}

bool CServer::listen(const QHostAddress &address, ushort port)
{
    return p_ptr->server->listen(address, port);
}

QHostAddress CServer::address() const
{
    return p_ptr->server->serverAddress();
}

ushort CServer::port() const
{
    return p_ptr->server->serverPort();
}

void CServer::setAcceptMultipleClientsBehindOneIp(bool enabled)
{
    p_ptr->acceptMultipleClientsBehindOneIp = enabled;
}

bool CServer::acceptMultipleClientsBehindOneIp() const
{
    return p_ptr->acceptMultipleClientsBehindOneIp;
}

CServerUser *CServer::findUser(uint id)
{
    return p_ptr->users.value(id);
}

QHash<uint, CServerUser *> CServer::users() const
{
    return p_ptr->users;
}

void CServer::createRoom(CServerUser *owner, const QString &name, uint capacity)
{
    CRoom *room = new CRoom(this);
    connect(room, &CRoom::abandoned, this, &CServer::onRoomAbandoned);
    room->setName(name);
    room->setCapacity(capacity);
    room->setOwner(owner);
    room->addUser(owner);
    p_ptr->rooms.insert(room->id(), room);
    emit roomCreated(room);
}

CRoom *CServer::findRoom(uint id) const
{
    return p_ptr->rooms.value(id);
}

QHash<uint, CRoom *> CServer::rooms() const
{
    return p_ptr->rooms;
}

CRoom *CServer::lobby() const
{
    return p_ptr->lobby;
}

void CServer::updateRoomList(CServerUser *user)
{
    QVariantList roomList;
    foreach (CRoom *room, p_ptr->rooms)
        roomList << room->config();
    user->notify(S_COMMAND_SET_ROOM_LIST, roomList);
}

void CServer::broadcastNotification(int command, const QVariant &data, CServerUser *except)
{
    foreach (CServerUser *user, p_ptr->users) {
        if (user != except)
            user->notify(command, data);
    }
}

void CServer::handleNewConnection(CTcpSocket *client)
{
    CServerUser *user = new CServerUser(client, this);

    if (!acceptMultipleClientsBehindOneIp()) {
        if (p_ptr->clientIp.contains(user->ip())) {
            CServerUser *prevUser = p_ptr->clientIp.value(user->ip());
            prevUser->kick();
            prevUser->deleteLater();
        }

        p_ptr->clientIp.insert(user->ip(), user);
    }

    connect(user, &CServerUser::disconnected, this, &CServer::onUserDisconnected);
    connect(user, &CServerUser::stateChanged, this, &CServer::onUserStateChanged);
}

void CServer::onRoomAbandoned()
{
    CRoom *room = qobject_cast<CRoom *>(sender());
    p_ptr->rooms.remove(room->id());
}

void CServer::onUserDisconnected()
{
    CServerUser *user = qobject_cast<CServerUser *>(sender());
    //@todo: check the state and enable reconnection
    //if (user && user->state() == CServerUser::LoggedOut)
    p_ptr->users.remove(user->id());
    user->deleteLater();
}

void CServer::onUserStateChanged()
{
    CServerUser *user = qobject_cast<CServerUser *>(sender());
    if (user->state() == CServerUser::Online) {
        if (!p_ptr->users.contains(user->id())) {
            p_ptr->users.insert(user->id(), user);
            p_ptr->lobby->addUser(user);
            emit userAdded(user);
        }
    }
}
