/****************************************************************************
**
** Copyright (C) 1992-2005 Trolltech AS. All rights reserved.
**
** This file is part of the compatibility classes of the Qt Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef Q3SERVERSOCKET_H
#define Q3SERVERSOCKET_H

#ifndef QT_H
#include "QtCore/qobject.h"
#include "QtNetwork/qhostaddress.h"
#include "q3socketdevice.h" // ### remove or keep for users' convenience?
#endif // QT_H

class Q3ServerSocketPrivate;

class Q_COMPAT_EXPORT Q3ServerSocket : public QObject
{
    Q_OBJECT
public:
    Q3ServerSocket( Q_UINT16 port, int backlog = 1,
		   QObject *parent=0, const char *name=0 );
    Q3ServerSocket( const QHostAddress & address, Q_UINT16 port, int backlog = 1,
		   QObject *parent=0, const char *name=0 );
    Q3ServerSocket( QObject *parent=0, const char *name=0 );
    virtual ~Q3ServerSocket();

    bool ok() const;

    Q_UINT16 port() const ;

    int socket() const ;
    virtual void setSocket( int socket );

    QHostAddress address() const ;

    virtual void newConnection( int socket ) = 0;

protected:
    Q3SocketDevice *socketDevice();

private slots:
    void incomingConnection( int socket );

private:
    Q3ServerSocketPrivate *d;
    void init( const QHostAddress & address, Q_UINT16 port, int backlog );
};

#endif // Q3SERVERSOCKET_H
