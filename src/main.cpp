/*
 * main.cpp - ambrosia
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

#include "qca-tls.h"
#include "qca-sasl.h"

#define NS_IMSESSION "urn:ietf:params:xml:ns:xmpp-session"
#define NS_ROSTER    "jabber:iq:roster"
#define NS_VCARD     "vcard-temp"

using namespace XMPP;

static QString subns(const Stanza &s)
{
	QDomElement e = s.element();
	for(QDomNode n = e.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if(n.isElement())
			return n.toElement().namespaceURI();
	}
	return QString();
}

//static QDomElement subelement(const QDomElement &e, const QString &ns, const QString &name)
//{
//	return e.elementsByTagNameNS(ns, name).item(0).toElement();
//}

class RosterItem
{
public:
	Jid jid;
	QString name;
	QString sub;
	bool ask;
	QStringList groups;
};

class RosterChanges
{
public:
	QDomElement toXml(QDomDocument *) const
	{
		return QDomElement();
	}
};

class Roster : public QList<RosterItem>
{
public:
	QDomElement toXml(QDomDocument *) const
	{
		return QDomElement();
	}

	bool fromXml(const QDomElement &)
	{
		return false;
	}
};

class User
{
public:
	Roster roster;
};

/*static QString hex(QChar c)
{
	QString str;
	str.sprintf("%02x", (uchar)c.toLatin1());
	return str;
}

static QString normalize(const QString &in)
{
	QString out;
	for(int n = 0; n < in.length(); ++n)
	{
		if(in[n].isLetterOrNumber())
			out += in[n];
		else
			out += hex(in[n]);
	}
	return out;
}

static Roster loadRoster(const QString &user)
{
	Roster r;
	QFile f(normalize(user) + ".xml");
	if(!f.open(QIODevice::ReadOnly))
		return r;

	QDomDocument doc;
	doc.setContent(&f);
	r.fromXml(doc.documentElement());
	return r;
}

static void saveRoster(const QString &user, const Roster &r)
{
	QFile f(normalize(user) + ".xml");
	f.open(QIODevice::WriteOnly | QIODevice::Truncate);

	QDomDocument doc;
	doc.appendChild(r.toXml(&doc));
	QByteArray buf = doc.toString().toUtf8();
	f.write(buf.data(), buf.size());
}*/

class App : public QObject
{
	Q_OBJECT
public:
	Router r;
	QString host;
	bool c2s_ssl;

	App(const QString &_host, const QCA::Cert &cert, const QCA::RSAKey &key)
	{
		host = _host;
		if(!cert.isNull())
		{
			c2s_ssl = true;
			r.setCertificate(cert, key);
		}
		else
			c2s_ssl = false;
		connect(&r, SIGNAL(readyRead(const Stanza &)), SLOT(router_readyRead(const Stanza &)));
	}

	void start()
	{
		if(!r.start(host))
		{
			printf("Error binding to port 5222/5223/5269!\n");
			QTimer::singleShot(0, this, SIGNAL(quit()));
			return;
		}
		if(c2s_ssl)
			printf("Listening on %s:[5222,5223,5269] ...\n", host.toLatin1().data());
		else
			printf("Listening on %s:[5222,5269] ...\n", host.toLatin1().data());
	}

signals:
	void quit();

private slots:
	void router_readyRead(const Stanza &in)
	{
		Jid jhost = host;
		Jid inhost = in.from().domain();
		bool local = jhost.compare(inhost);
		QString user;
		if(local)
			user = in.from().node();

		if(in.kind() == Stanza::IQ)
		{
			if(in.type() != "get" && in.type() != "set")
				return;

			if(local)
			{
				if(subns(in) == NS_IMSESSION)
				{
					Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
					r.write(out);
				}
				else if(subns(in) == NS_ROSTER)
				{
					if(in.type() == "get")
					{
						//Roster roster = loadRoster(user);
						Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
						out.setFrom(in.from());
						//out.appendChild(roster.toXml(&out.doc()));
						r.write(out);
					}
					else if(in.type() == "set")
					{
						/*Roster roster = loadRoster(user);
						RosterChanges changes = roster.applyChanges(subelement(in.element(), NS_ROSTER, "query"));
						saveRoster(user, roster);

						// ack
						Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
						out.setFrom(in.from());
						r.write(out);

						// broadcast
						out = Stanza(XMPP::Stanza::IQ, in.from(), "set");
						out.setFrom(in.from());
						out.appendChild(changes.toXml(&out.doc()));
						r.write(out);*/

						// TODO: if deleting a contact, send unsubscribed to target
						// TODO: broadcast changes to all logged in resources
					}
					return;
				}
			}

			if(subns(in) == NS_VCARD)
			{
				// return static vcard
				Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
				out.setFrom(in.to());
				QDomElement v = out.createElement("vcard-temp", "vCard");
				v.setAttribute("prodid", "-//HandGen//NONSGML vGen v1.0//EN");
				v.setAttribute("version", "2.0");
				QDomElement nick = out.createElement("vcard-temp", "NICKNAME");
				nick.appendChild(out.doc().createTextNode(in.to().node()));
				v.appendChild(nick);
				out.appendChild(v);
				r.write(out);
			}
			else
			{
				// TODO: return service unavailable
				Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
				r.write(out);
			}
			return;
		}
		else if(in.kind() == Stanza::Presence)
		{
			// directed presence?
			bool dp = !in.to().isEmpty();

			if(in.type() == "unavailable")
			{
				// TODO: send unavailable to all watchers
			}
			else if(in.type() == "subscribe")
			{
				if(local)
				{
					// TODO: if not subscribed to target, then:
					//   - add to roster if not already there
					//   - send to destination
					//   - ask=subscribe
				}
				else
				{
					// TODO: if not subscribed to target, then:
					//   - send to destination
				}
			}
			else if(in.type() == "subscribed")
			{
				if(local)
				{
					// TODO:
				}
				else
				{
				}
			}
			else if(in.type() == "unsubscribe")
			{
			}
			else if(in.type() == "unsubscribed")
			{
			}
			else if(in.type() == "probe")
			{
			}
			else if(in.type() == "error")
			{
			}
			else
			{
				if(dp)
				{
					if(local)
					{
						// TODO: send to destination, mark in dp list for this contact
					}
					else
					{
						// TODO: send to destination
					}
				}
				else
				{
					if(local)
					{
						// TODO: deliver to local watchers
						// TODO: send to self resources
						// TODO: if this is the first time, probe for other presence
					}
					else
					{
						// TODO: can't have broadcasted presence from non-local
					}
				}
			}
		}
		else if(in.kind() == Stanza::Message)
		{
			r.write(in);
		}
	}
};

#include "main.moc"

static QCA::Cert readCertFile(const QString &fname)
{
	QCA::Cert cert;
	QFile f(fname);
	if(f.open(QIODevice::ReadOnly))
	{
		QByteArray buf = f.readAll();
		cert.fromPEM(QString::fromLatin1(buf));
	}
	return cert;
}

static QCA::RSAKey readKeyFile(const QString &fname)
{
	QCA::RSAKey key;
	QFile f(fname);
	if(f.open(QIODevice::ReadOnly))
	{
		QByteArray buf = f.readAll();
		key.fromPEM(QString::fromLatin1(buf));
	}
	return key;
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	if(argc < 2)
	{
		printf("Usage: ambrosia [hostname] (cert.pem) (privkey.pem)\n\n");
		return 0;
	}

	QCA::init();
	QCA::insertProvider(createProviderTLS());
	QCA::insertProvider(createProviderSASL());

	{
		QCA::Cert cert;
		QCA::RSAKey key;
		if(argc >= 4)
		{
			cert = readCertFile(argv[2]);
			key = readKeyFile(argv[3]);
			if(cert.isNull() || key.isNull())
			{
				printf("Error reading cert/key files\n\n");
				return 0;
			}
		}

		QString host(argv[1]);

		srand(time(NULL));

		App *a = new App(host, cert, key);
		QObject::connect(a, SIGNAL(quit()), &app, SLOT(quit()));
		a->start();
		app.exec();
		delete a;
	}

	// clean up
	QCA::unloadAllPlugins();

	return 0;
}
