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

#ifndef Q3SOCKETDEVICE_H
#define Q3SOCKETDEVICE_H

#ifndef QT_H
#include "QtCore/qiodevice.h"
#include "QtNetwork/qhostaddress.h" // int->QHostAddress conversion
#endif // QT_H

#ifndef QT_NO_NETWORK
typedef qint64 Offset;
typedef quint16 Q_UINT16;
typedef long Q_LONG;
typedef unsigned long Q_ULONG;
#define IO_ReadOnly QIODevice::ReadOnly
#define IO_WriteOnly QIODevice::WriteOnly
#define IO_ReadWrite QIODevice::ReadWrite
#define IO_Append QIODevice::Append
#define IO_Truncate QIODevice::Truncate
#define IO_Translate QIODevice::Text
#define IO_ModeMask 0x00ff
typedef int Status;
Status iodev_status(QIODevice *that);
void iodev_resetStatus(QIODevice *that);
bool is_ipv4(const QHostAddress &h);
bool is_ipv6(const QHostAddress &h);

class Q3SocketDevicePrivate;

class Q_COMPAT_EXPORT Q3SocketDevice: public QIODevice
{
public:
    enum Type { Stream, Datagram };
    enum Protocol { IPv4, IPv6, Unknown };

    Q3SocketDevice( Type type = Stream );
    Q3SocketDevice( Type type, Protocol protocol, int dummy );
    Q3SocketDevice( int socket, Type type );
    virtual ~Q3SocketDevice();

    bool	 isValid() const;
    Type	 type() const;
    Protocol	 protocol() const;

    int		 socket() const;
    virtual void setSocket( int socket, Type type );

    bool	 open( int mode );
    void	 close();
    bool	 flush();

    // Implementation of QIODevice abstract virtual functions
    Offset	 size() const;
    Offset	 at() const;
    bool	 at( Offset );
    bool	 atEnd() const;

    bool	 blocking() const;
    virtual void setBlocking( bool );

    bool	 addressReusable() const;
    virtual void setAddressReusable( bool );

    int		 receiveBufferSize() const;
    virtual void setReceiveBufferSize( uint );
    int		 sendBufferSize() const;
    virtual void setSendBufferSize( uint );

    virtual bool connect( const QHostAddress &, Q_UINT16 );

    virtual bool bind( const QHostAddress &, Q_UINT16 );
    virtual bool listen( int backlog );
    virtual int	 accept();

    qint64	 bytesAvailable() const;
    Q_LONG	 waitForMore( int msecs, bool *timeout=0 ) const;
    virtual Q_LONG  writeBlock( const char *data, Q_ULONG len,
			    const QHostAddress & host, Q_UINT16 port );
    inline Q_LONG writeBlock(const char *data, Q_ULONG len)
        { return qint64(write(data, qint64(len))); }
    inline qint64 readBlock(char *data, Q_ULONG maxlen)
        { return qint64(read(data, qint64(maxlen))); }

    Q_UINT16	 port() const;
    Q_UINT16	 peerPort() const;
    QHostAddress address() const;
    QHostAddress peerAddress() const;

    enum Error {
	NoError,
	AlreadyBound,
	Inaccessible,
	NoResources,
	InternalError,
	Bug = InternalError, // ### remove in 4.0?
	Impossible,
	NoFiles,
	ConnectionRefused,
	NetworkFailure,
	UnknownError
    };
    Error	 error() const;

    inline bool isSequential() const { return true; }

protected:
    void setError( Error err );
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private:
    int fd;
    Type t;
    Q_UINT16 p;
    QHostAddress a;
    Q_UINT16 pp;
    QHostAddress pa;
    Q3SocketDevice::Error e;
    Q3SocketDevicePrivate * d;

    enum Option { Broadcast, ReceiveBuffer, ReuseAddress, SendBuffer };

    int		 option( Option ) const;
    virtual void setOption( Option, int );

    void	 fetchConnectionParameters();
#if defined(Q_OS_WIN32)
    void	 fetchPeerConnectionParameters();
#endif

    static void  init();
    int		 createNewSocket();
    Protocol	 getProtocol() const;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    Q3SocketDevice( const Q3SocketDevice & );
    Q3SocketDevice &operator=( const Q3SocketDevice & );
#endif
};

#endif // QT_NO_NETWORK

#endif // Q3SOCKETDEVICE_H
