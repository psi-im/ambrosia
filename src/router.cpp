/*
 * router.cpp - ambrosia
 * Copyright (C) 2005  Justin Karneges <justin@affinix.com>
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "router.h"

#include "bsocket.h"
#include "servsock.h"

using namespace XMPP;

enum Mode { Client, Server };
enum Direction { In, Out };

class Router::Private : public QObject
{
	Q_OBJECT
public:
	Router *parent;
	ServSock c2s, c2s_ssl, s2s;
	QString host, realm;
	QCA::Cert cert;
	QCA::RSAKey privkey;
	QList<Session*> list;
	Jid jhost;

	Private(Router *);
	~Private();

	bool init();
	void stop();
	Session *ensureOutbound(const QString &host);
	Session *session(ClientStream *s);
	Session *sessionForUser(const QString &user);
	Session *pendingInboundSession(const QString &id);
	Session *pendingOutboundSession(const QString &id, const QString &key);

	void read(const Stanza &s);
	void write(const Stanza &s);

public slots:
	void c2s_connectionReady(int s);
	void c2s_ssl_connectionReady(int s);
	void s2s_connectionReady(int s);
	void sess_done();
};

int id_num = 0;

//----------------------------------------------------------------------------
// Router::Session
//----------------------------------------------------------------------------
class Router::Session : public QObject
{
	Q_OBJECT
public:
	Private *r;

	AdvancedConnector *conn;
	ClientStream *stream;
	QCA::TLS *tls;
	Mode mode;
	Direction dir;
	bool active;
	bool verify;
	QString ver_id, sent_key;
	int id;
	QList<Stanza> pending_stanzas;

	// incoming
	Session(Private *_r, ByteStream *bs, Mode _mode, bool sslnow)
	{
		r = _r;
		id = id_num++;
		active = true;
		verify = false;
		mode = _mode;
		dir = In;

		conn = 0;
		tls = 0;
		if(!r->cert.isNull() && !r->privkey.isNull())
		{
			tls = new QCA::TLS;
			tls->setCertificate(r->cert, r->privkey);
		}

		stream = new ClientStream(r->host, r->realm, bs, tls, sslnow, mode == Server ? true : false);
		connect(stream, SIGNAL(connectionClosed()), SLOT(cs_connectionClosed()));
		connect(stream, SIGNAL(error(int)), SLOT(cs_error(int)));
		connect(stream, SIGNAL(readyRead()), SLOT(cs_readyRead()));
		connect(stream, SIGNAL(authenticated()), SLOT(cs_authenticated()));

		// server
		connect(stream, SIGNAL(dialbackRequest(const Jid &, const Jid &, const QString &)), SLOT(cs_dialbackRequest(const Jid &, const Jid &, const QString &)));
		connect(stream, SIGNAL(dialbackVerifyRequest(const Jid &, const Jid &, const QString &, const QString &)), SLOT(cs_dialbackVerifyRequest(const Jid &, const Jid &, const QString &, const QString &)));
	}

	// outgoing
	Session(Private *_r, const Jid &to, const QString &key)
	{
		r = _r;
		id = id_num++;
		active = false;
		verify = false;
		mode = Server;
		dir = Out;
		sent_key = key;

		tls = 0;
		conn = new AdvancedConnector;
		stream = new ClientStream(conn, 0);
		connect(stream, SIGNAL(connectionClosed()), SLOT(cs_connectionClosed()));
		connect(stream, SIGNAL(error(int)), SLOT(cs_error(int)));
		connect(stream, SIGNAL(readyRead()), SLOT(cs_readyRead()));
		connect(stream, SIGNAL(authenticated()), SLOT(cs_authenticated()));

		// server
		connect(stream, SIGNAL(dialbackResult(const Jid &, bool)), SLOT(cs_dialbackResult(const Jid &, bool)));

		printf("[%d]: New outbound session\n", id);
		stream->connectToServerAsServer(to, r->host, key);
	}

	// dialback verify
	Session(Private *_r, const Jid &to, const QString &_id, const QString &key)
	{
		r = _r;
		id = id_num++;
		active = false;
		verify = true;
		mode = Server;
		dir = Out;
		ver_id = _id;

		tls = 0;
		conn = new AdvancedConnector;
		stream = new ClientStream(conn, 0);
		connect(stream, SIGNAL(dialbackVerifyResult(const Jid &, bool)), SLOT(cs_dialbackVerifyResult(const Jid &, bool)));

		printf("[%d]: Verifying\n", id);
		stream->connectToServerAsServerVerify(to, r->host, _id, key);
	}

	~Session()
	{
		delete stream;
		delete tls;
		delete conn;
		printf("[%d]: deleted\n", id);
	}

	void accept()
	{
		printf("[%d]: New inbound session!\n", id);
		stream->accept();
	}

	void close()
	{
		emit done();
	}

	void write(const Stanza &s)
	{
		if(active)
			stream->write(s);
		else
			pending_stanzas.append(s);
	}

signals:
	void activated();
	void done();

public slots:
	void cs_connectionClosed()
	{
		printf("[%d]: Connection closed by peer\n", id);
		close();
	}

	void cs_error(int)
	{
		printf("[%d]: Error\n", id);
		close();
	}

	void cs_authenticated()
	{
		printf("[%d]: <<< Authenticated >>>\n", id);
	}

	void cs_dialbackRequest(const Jid &to, const Jid &from, const QString &key)
	{
		printf("[%d]: Dialback Request: to=[%s], from=[%s], key=[%s]\n", id, to.full().toLatin1().data(), from.full().toLatin1().data(), key.toLatin1().data());

		Jid us = r->host;
		if(!to.compare(us))
		{
			stream->dialbackRequestGrant(from, to, false);
			return;
		}

		// dialback to verify the host
		Session *sess = new Session(r, from, stream->id(), key);
		connect(sess, SIGNAL(done()), r, SLOT(sess_done()));
		r->list.append(sess);
	}

	void cs_dialbackResult(const Jid &from, bool ok)
	{
		printf("[%d]: Dialback Result: from=[%s], ok=[%s]\n", id, from.full().toLatin1().data(), ok ? "yes" : "no");

		if(ok)
		{
			active = true;
			emit activated();

			// send pending stanzas
			for(int n = 0; n < pending_stanzas.size(); ++n)
				stream->write(pending_stanzas[n]);
			pending_stanzas.clear();
		}
	}

	void cs_dialbackVerifyRequest(const Jid &to, const Jid &from, const QString &_id, const QString &key)
	{
		printf("[%d]: Dialback Verify Request: to=[%s], from=[%s], key=[%s]\n", id, to.full().toLatin1().data(), from.full().toLatin1().data(), key.toLatin1().data());

		if(!r->pendingOutboundSession(_id, key))
		{
			stream->dialbackVerifyRequestGrant(from, r->host, QString(), false);
			return;
		}

		stream->dialbackVerifyRequestGrant(from, r->host, _id, true);
	}

	void cs_dialbackVerifyResult(const Jid &from, bool ok)
	{
		printf("[%d]: Dialback Verify Result: from=[%s], ok=[%s]\n", id, from.full().toLatin1().data(), ok ? "yes" : "no");

		Session *sess = r->pendingInboundSession(ver_id);
		if(sess)
			sess->stream->dialbackRequestGrant(from, r->host, ok);
		close();
	}

	void cs_readyRead()
	{
		printf("[%d]: ReadyRead\n", id);
		while(stream->stanzaAvailable())
		{
			Stanza s = stream->read();

			if(mode == Client)
				s.setFrom(stream->jid());

			r->read(s);
		}
	}
};

//----------------------------------------------------------------------------
// Router::Private
//----------------------------------------------------------------------------
Router::Private::Private(Router *_parent)
{
	parent = _parent;
	connect(&c2s, SIGNAL(connectionReady(int)), SLOT(c2s_connectionReady(int)));
	connect(&c2s_ssl, SIGNAL(connectionReady(int)), SLOT(c2s_ssl_connectionReady(int)));
	connect(&s2s, SIGNAL(connectionReady(int)), SLOT(s2s_connectionReady(int)));
}

Router::Private::~Private()
{
	stop();
}

bool Router::Private::init()
{
	if(!c2s.listen(5222) || (!cert.isNull() && !c2s_ssl.listen(5223)) || !s2s.listen(5269)) {
		c2s.stop();
		c2s_ssl.stop();
		s2s.stop();
		return false;
	}
	return true;
}

void Router::Private::stop()
{
	qDeleteAll(list);
	c2s.stop();
	c2s_ssl.stop();
	s2s.stop();
}

Router::Session *Router::Private::ensureOutbound(const QString &host)
{
	Jid to = host;
	for(int n = 0; n < list.size(); ++n)
	{
		if(list[n]->mode == Server && list[n]->dir == Out && list[n]->stream->jid().compare(host))
			return list[n];
	}

	// fire up a connection
	Session *sess = new Session(this, to, "123");
	connect(sess, SIGNAL(done()), SLOT(sess_done()));
	list.append(sess);
	return sess;
}

Router::Session *Router::Private::session(ClientStream *s)
{
	for(int n = 0; n < list.size(); ++n)
	{
		if(list[n]->stream == s)
			return list[n];
	}
	return 0;
}

Router::Session *Router::Private::sessionForUser(const QString &user)
{
	for(int n = 0; n < list.size(); ++n)
	{
		if(list[n]->mode == Client && list[n]->stream->jid().node() == user)
			return list[n];
	}
	return 0;
}

Router::Session *Router::Private::pendingInboundSession(const QString &id)
{
	for(int n = 0; n < list.size(); ++n)
	{
		if(list[n]->mode == Server && list[n]->dir == In && list[n]->stream->id() == id)
			return list[n];
	}
	return 0;
}

Router::Session *Router::Private::pendingOutboundSession(const QString &id, const QString &key)
{
	for(int n = 0; n < list.size(); ++n)
	{
		if(list[n]->mode == Server && list[n]->dir == Out && !list[n]->active && list[n]->stream->id() == id && list[n]->sent_key == key)
			return list[n];
	}
	return 0;
}

void Router::Private::c2s_connectionReady(int s)
{
	BSocket *bs = new BSocket;
	bs->setSocket(s);
	Session *sess = new Session(this, bs, Client, false);
	list.append(sess);
	connect(sess, SIGNAL(done()), SLOT(sess_done()));
	sess->accept();
}

void Router::Private::c2s_ssl_connectionReady(int s)
{
	BSocket *bs = new BSocket;
	bs->setSocket(s);
	Session *sess = new Session(this, bs, Client, true);
	list.append(sess);
	connect(sess, SIGNAL(done()), SLOT(sess_done()));
	sess->accept();
}

void Router::Private::s2s_connectionReady(int s)
{
	BSocket *bs = new BSocket;
	bs->setSocket(s);
	Session *sess = new Session(this, bs, Server, false);
	list.append(sess);
	connect(sess, SIGNAL(done()), SLOT(sess_done()));
	sess->accept();
}

void Router::Private::sess_done()
{
	Session *sess = (Session *)sender();
	sess->deleteLater();
	int n = list.indexOf(sess);
	list.removeAt(n);
}

void Router::Private::read(const Stanza &s)
{
	// incoming always jabber:client
	Stanza sw = s;
	sw.setBaseNS("jabber:client");
	printf("Reading Stanza: [%s]\n", sw.toString().toLatin1().data());
	emit parent->readyRead(sw);
}

void Router::Private::write(const Stanza &s)
{
	printf("Writing Stanza: [%s]\n", s.toString().toLatin1().data());

	Jid outhost = Jid(s.to().domain());

	// local?
	if(jhost.compare(outhost))
	{
		Session *sess = sessionForUser(s.to().node());
		if(sess)
			sess->write(s);
		else
			printf("no session for user: [%s]\n", s.to().node().toLatin1().data());
	}
	else
	{
		Session *sess = ensureOutbound(outhost.full());
		Stanza sw = s;
		sw.setBaseNS("jabber:server");
		sess->write(sw);
	}
}

//----------------------------------------------------------------------------
// Router
//----------------------------------------------------------------------------
Router::Router()
{
	d = new Private(this);
}

Router::~Router()
{
	delete d;
}

void Router::setCertificate(const QCA::Cert &cert, const QCA::RSAKey &key)
{
	d->cert = cert;
	d->privkey = key;
}

bool Router::start(const QString &host)
{
	d->realm = QDns::getHostName();
	if(!host.isEmpty())
		d->host = host;
	else
		d->host = d->realm;
	d->jhost = host;
	return d->init();
}

void Router::stop()
{
	d->stop();
}

void Router::write(const XMPP::Stanza &s)
{
	d->write(s);
}

#include "router.moc"
