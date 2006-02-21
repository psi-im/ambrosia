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

#define SERVER_VERSION "0.1"

#define NS_IMSESSION "urn:ietf:params:xml:ns:xmpp-session"
#define NS_ROSTER    "jabber:iq:roster"
#define NS_VCARD     "vcard-temp"
#define NS_VERSION   "jabber:iq:version"

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

static QDomElement subelement(const QDomElement &e, const QString &ns, const QString &name)
{
	QDomNodeList nl = e.elementsByTagNameNS(ns, name);
	if(nl.count() > 0)
		return nl.item(0).toElement();
	return QDomElement();
}

static QString subtext(const QDomElement &e)
{
	for(QDomNode n = e.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if(n.isText())
			return n.toText().data();
	}
	return QString();
}

class RosterChange
{
public:
	Jid jid;
	QString name;
	bool remove;
	QStringList groups;

	bool fromXml(const QDomElement &e)
	{
		jid = Jid();
		name = QString();
		remove = false;
		groups = QStringList();

		if(e.namespaceURI() != NS_ROSTER || e.localName() != "item")
			return false;

		if(!e.hasAttribute("jid"))
			return false;
		jid = e.attribute("jid");
		if(e.hasAttribute("name"))
			name = e.attribute("name");
		remove = e.hasAttribute("subscription") && e.attribute("subscription") == "remove";

		QDomNodeList gnl = e.elementsByTagNameNS(NS_ROSTER, "group");
		for(int n = 0; n < gnl.count(); ++n)
			groups += subtext(gnl.item(n).toElement());

		return true;
	}
};

class RosterChangeList : public QList<RosterChange>
{
public:
	bool fromXml(const QDomElement &in)
	{
		clear();
		if(in.namespaceURI() != NS_ROSTER || in.localName() != "query")
			return false;

		QDomNodeList nl = in.elementsByTagNameNS(NS_ROSTER, "item");
		for(int n = 0; n < nl.count(); ++n)
		{
			QDomElement e = nl.item(n).toElement();
			RosterChange rc;
			if(rc.fromXml(e))
				append(rc);
		}
		return true;
	}
};

class RosterItem
{
public:
	Jid jid;
	QString name;
	QString sub;
	bool ask;
	QStringList groups;

	QDomElement toXml(QDomDocument *doc) const
	{
		QDomElement e = doc->createElementNS(NS_ROSTER, "item");
		e.setAttribute("jid", jid.full());
		if(!name.isEmpty())
			e.setAttribute("name", name);
		e.setAttribute("subscription", sub);
		if(ask)
			e.setAttribute("ask", "subscribe");
		for(int n = 0; n < groups.count(); ++n)
		{
			QDomElement ge = doc->createElementNS(NS_ROSTER, "group");
			QDomText text = doc->createTextNode(groups[n]);
			ge.appendChild(text);
			e.appendChild(ge);
		}
		return e;
	}

	bool fromXml(const QDomElement &e)
	{
		jid = Jid();
		name = QString();
		sub = QString();
		ask = false;
		groups = QStringList();

		if(e.namespaceURI() != NS_ROSTER || e.localName() != "item")
			return false;

		if(!e.hasAttribute("jid"))
			return false;
		jid = e.attribute("jid");
		if(e.hasAttribute("name"))
			name = e.attribute("name");
		if(e.hasAttribute("subscription"))
		{
			sub = e.attribute("subscription");
			if(sub != "none" && sub != "from" && sub != "to" && sub != "both")
				return false;
		}
		ask = e.hasAttribute("ask") && e.attribute("ask") == "subscribe";

		QDomNodeList gnl = e.elementsByTagNameNS(NS_ROSTER, "group");
		for(int n = 0; n < gnl.count(); ++n)
			groups += subtext(gnl.item(n).toElement());

		return true;
	}
};

class Roster : public QList<RosterItem>
{
public:
	Roster applyChanges(const RosterChangeList &changeList)
	{
		Roster out;
		for(int k = 0; k < changeList.count(); ++k)
		{
			const RosterChange &rc = changeList[k];

			int index = -1;
			for(int n = 0; n < count(); ++n)
			{
				if(at(n).jid.compare(rc.jid))
				{
					index = n;
					break;
				}
			}

			if(rc.remove)
			{
				if(index != -1)
				{
					RosterItem ri = at(index);
					ri.sub = "remove";
					out += ri;
					removeAt(index);
				}
			}
			else
			{
				// new item?
				if(index == -1)
				{
					RosterItem new_item;
					new_item.jid = rc.jid;
					new_item.sub = "none";
					new_item.ask = false;
					index = count();
					append(new_item);
				}

				RosterItem &ri = operator[](index);
				ri.name = rc.name;
				ri.groups = rc.groups;
				out += ri;
			}
		}
		return out;
	}

	QDomElement toXml(QDomDocument *doc) const
	{
		QDomElement root = doc->createElementNS(NS_ROSTER, "roster");
		for(int n = 0; n < count(); ++n)
			root.appendChild(at(n).toXml(doc));
		return root;
	}

	QDomElement toQueryXml(QDomDocument *doc) const
	{
		QDomElement root = doc->createElementNS(NS_ROSTER, "query");
		for(int n = 0; n < count(); ++n)
			root.appendChild(at(n).toXml(doc));
		return root;
	}

	bool fromXml(const QDomElement &in)
	{
		clear();
		if(in.namespaceURI() != NS_ROSTER || in.localName() != "roster")
			return false;

		printf("---- reading items\n");
		QDomNodeList nl = in.elementsByTagNameNS(NS_ROSTER, "item");
		printf("---- nlcount: %d\n", nl.count());
		for(int n = 0; n < nl.count(); ++n)
		{
			QDomElement e = nl.item(n).toElement();
			RosterItem ri;
			if(ri.fromXml(e))
				append(ri);
		}
		return true;
	}
};

class User
{
public:
	Roster roster;
	QDomElement vcard;
};

static QString hex(QChar c)
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

static User loadUser(const QString &username)
{
	User user;
	QFile f("data/" + normalize(username) + ".xml");
	printf("---- trying to read: [%s]\n", qPrintable(f.fileName()));
	if(!f.open(QIODevice::ReadOnly))
		return user;

	QDomDocument doc;
	doc.setContent(&f, true);
	QDomElement u = doc.documentElement();
	printf("[%s]\n", qPrintable(u.tagName()));
	QDomNodeList nl = u.elementsByTagNameNS(NS_ROSTER, "roster");
	if(nl.count() > 0)
		user.roster.fromXml(nl.item(0).toElement());
	nl = u.elementsByTagNameNS("vcard-temp", "vCard");
	printf("nl count: %d\n", nl.count());
	if(nl.count() > 0)
		user.vcard = nl.item(0).toElement();
	if(user.vcard.isNull())
		printf("no vcard\n");
	return user;
}

static void saveUser(const QString &username, const User &user)
{
	QDir dir("data");
	if(!dir.exists())
	{
		dir.setPath(".");
		if(!dir.mkdir("data"))
			return;
	}
	QFile f("data/" + normalize(username) + ".xml");
	printf("---- trying to write: [%s]\n", qPrintable(f.fileName()));
	f.open(QIODevice::WriteOnly | QIODevice::Truncate);

	QDomDocument doc;
	QDomElement u = doc.createElement("user");
	u.setAttribute("name", username);
	u.appendChild(user.roster.toXml(&doc));
	if(!user.vcard.isNull())
		u.appendChild(user.vcard);
	doc.appendChild(u);
	QByteArray buf = doc.toString().toUtf8();
	f.write(buf.data(), buf.size());
}

/*class PresenceItem
{
public:
	Jid jid;
	Stanza stanza;
	QList<Jid> watchers;
};*/

class PresenceManager
{
public:
	class Item
	{
	public:
		Jid user;
		Stanza stanza;
		QList<Jid> dontsend;
	};

	class DPItem
	{
	public:
		Jid user;
		Jid to;
		Stanza stanza;
	};

	QString host;
	Router &router;
	QList<Item> list;
	QList<DPItem> dplist;

	PresenceManager(Router *_router) : router(*_router)
	{
	}

	int findItem(const Jid &user)
	{
		for(int n = 0; n < list.count(); ++n)
		{
			if(list[n].user.compare(user))
			{
				return n;
			}
		}
		return -1;
	}

	int findDPItem(const Jid &user, const Jid &to)
	{
		for(int n = 0; n < dplist.count(); ++n)
		{
			if(dplist[n].user.compare(user) && dplist[n].to.compare(to))
			{
				return n;
			}
		}
		return -1;
	}

	bool haveDontSend(const Item &i, const Jid &to)
	{
		for(int n = 0; n < i.dontsend.count(); ++n)
		{
			if(i.dontsend[n].compare(to))
				return true;
		}
		return false;
	}

	bool forThisHost(const Jid &to)
	{
		Jid a = to.domain();
		Jid b = host;
		return a.compare(b);
	}

	void writeFromHost(const Stanza &out)
	{
		if(forThisHost(out.to()))
			incomingFromOutside(out);
		else
			router.write(out);
	}

	void incomingFromClient(const Stanza &in, const Jid &u)
	{
		// assumptions:
		// in.to:   optional
		// in.from: required
		// in.type: optional

		bool sub = false;
		if(in.type() == "subscribe"
			|| in.type() == "subscribed"
			|| in.type() == "unsubscribe"
			|| in.type() == "unsubscribed"
		)
		{
			sub = true;
		}

		// subscriptions require to/from
		if(sub && in.to().isEmpty())
		{
			// skip
			return;
		}

		if(sub)
			incomingFromClientSub(in, u);
		else
			incomingFromClientNormal(in, u);
	}

	void incomingFromOutside(const Stanza &in)
	{
		// assumptions:
		// in.to:   required
		// in.from: required
		// in.type: optional

		bool sub = false;
		if(in.type() == "subscribe"
			|| in.type() == "subscribed"
			|| in.type() == "unsubscribe"
			|| in.type() == "unsubscribed"
		)
		{
			sub = true;
		}

		// subscriptions require to/from
		if(sub && in.to().isEmpty())
		{
			// skip
			return;
		}

		if(sub)
			incomingFromOutsideSub(in);
		else
			incomingFromOutsideNormal(in);
	}

	void incomingFromClientSub(const Stanza &in, const Jid &u)
	{
		QString username = u.node();

		if(in.type() == "subscribe")
		{
			User user = loadUser(username);
			int index = -1;
			for(int n = 0; n < user.roster.count(); ++n)
			{
				RosterItem &r = user.roster[n];
				if(r.jid.compare(in.to()))
				{
					index = n;
					break;
				}
			}

			bool is_new = false;
			if(index == -1)
			{
				is_new = true;
				RosterItem new_item;
				new_item.jid = in.to();
				new_item.sub = "none";
				new_item.ask = false;
				index = user.roster.count();
				user.roster += new_item;
			}

			RosterItem &ri = user.roster[index];

			Roster changedItems;

			if(is_new || (!ri.ask && ri.sub != "to" && ri.sub != "both"))
			{
				ri.ask = true;
				changedItems += ri;
			}

			saveUser(username, user);

			if(!changedItems.isEmpty())
			{
				// broadcast
				Stanza out(XMPP::Stanza::IQ, in.from(), "set");
				out.setFrom(in.from());
				out.appendChild(changedItems.toQueryXml(&out.doc()));
				router.write(out);
			}

			// forward the presence
			Stanza tmp = in;
			tmp.setFrom(tmp.from().withResource(""));
			writeFromHost(tmp);
		}
		else if(in.type() == "subscribed")
		{
			User user = loadUser(username);
			int index = -1;
			for(int n = 0; n < user.roster.count(); ++n)
			{
				RosterItem &r = user.roster[n];
				if(r.jid.compare(in.to()))
				{
					index = n;
					break;
				}
			}

			if(index == -1)
				return;

			Roster changedItems;
			if(user.roster[index].sub == "none" || user.roster[index].sub == "to")
			{
				if(user.roster[index].sub == "none")
					user.roster[index].sub = "from";
				else
					user.roster[index].sub = "both";
				changedItems += user.roster[index];
			}

			saveUser(username, user);

			if(!changedItems.isEmpty())
			{
				// forward
				Stanza tmp = in;
				tmp.setFrom(tmp.from().withResource(""));
				writeFromHost(tmp);

				// broadcast
				Stanza out(XMPP::Stanza::IQ, in.from(), "set");
				out.setFrom(in.from());
				out.appendChild(changedItems.toQueryXml(&out.doc()));
				router.write(out);
			}
		}
		else if(in.type() == "unsubscribe")
		{
		}
		else if(in.type() == "unsubscribed")
		{
		}
	}

	void incomingFromOutsideSub(const Stanza &in)
	{
		Jid u = router.userSessionJid(in.to());
		if(u.isEmpty())
			return;

		QString username = u.node();

		if(in.type() == "subscribe")
		{
			User user = loadUser(username);
			int index = -1;
			for(int n = 0; n < user.roster.count(); ++n)
			{
				RosterItem &r = user.roster[n];
				if(r.jid.compare(in.from()))
				{
					index = n;
					break;
				}
			}

			if(index == -1)
			{
				// forward
				router.write(in);
				return;
			}

			// forward
			if(user.roster[index].sub == "none" || user.roster[index].sub == "to")
				router.write(in);
		}
		else if(in.type() == "subscribed")
		{
			User user = loadUser(username);
			int index = -1;
			for(int n = 0; n < user.roster.count(); ++n)
			{
				RosterItem &r = user.roster[n];
				if(r.jid.compare(in.from()))
				{
					index = n;
					break;
				}
			}

			if(index == -1)
				return;

			Roster changedItems;
			if(user.roster[index].sub == "none" || user.roster[index].sub == "from")
			{
				if(user.roster[index].sub == "none")
					user.roster[index].sub = "to";
				else
					user.roster[index].sub = "both";
				user.roster[index].ask = false;
				changedItems += user.roster[index];
			}

			saveUser(username, user);

			if(!changedItems.isEmpty())
			{
				// forward
				router.write(in);

				// broadcast
				Stanza out(XMPP::Stanza::IQ, u, "set");
				out.setFrom(u);
				out.appendChild(changedItems.toQueryXml(&out.doc()));
				router.write(out);

				// probe & presence push
				int x = findItem(u);
				if(x != -1)
				{
					Item &i = list[x];

					out = Stanza(Stanza::Presence, user.roster[index].jid, "probe");
					out.setFrom(u);
					writeFromHost(out);

					//i.watchers += u.roster[index].jid;

					out = i.stanza;
					out.setTo(user.roster[index].jid);
					out.setFrom(i.user);
					writeFromHost(out);
				}
			}
		}
		else if(in.type() == "unsubscribe")
		{
		}
		else if(in.type() == "unsubscribed")
		{
		}
	}

	void incomingFromClientNormal(const Stanza &in, const Jid &u)
	{
		// clients shouldn't send presence probes or errors
		if(in.type() == "probe" || in.type() == "error")
		{
			// skip
			return;
		}

		bool direct = !in.to().isEmpty();
		bool avail = true;
		if(in.type() == "unavailable")
			avail = false;

		if(direct)
		{
			int index = findDPItem(u, in.to());

			// don't have one for this combo yet?  make it
			if(index == -1)
			{
				DPItem di;
				di.user = u;
				di.to = in.to();
				index = dplist.count();
				dplist += di;
			}

			DPItem &di = dplist[index];
			di.stanza = in;

			Stanza out = in;
			writeFromHost(out);
		}
		else
		{
			int index = findItem(u);

			// initial ?
			bool initial = false;
			if(index == -1)
			{
				initial = true;
				Item i;
				i.user = u;
				index = list.count();
				list += i;
			}

			Item &i = list[index];
			i.stanza = in;

			// initial can't be unavailable
			if(initial && !avail)
			{
				// skip
				return;
			}

			User user = loadUser(u.node());

			// TODO: send to own available resources (we don't support these yet)

			if(initial)
			{
				// send presence probes to roster items of to/both
				//   from = u.fulljid
				//   to   = roster jid
				for(int n = 0; n < user.roster.count(); ++n)
				{
					RosterItem &r = user.roster[n];
					if(r.sub == "to" || r.sub == "both")
					{
						Stanza out(Stanza::Presence, r.jid, "probe");
						out.setFrom(u);
						writeFromHost(out);
					}
				}
			}

			// broadcast presence to roster items of from/both
			//   from = u.fulljid
			//   to   = roster.jid
			for(int n = 0; n < user.roster.count(); ++n)
			{
				RosterItem &r = user.roster[n];
				if(r.sub == "from" || r.sub == "both")
				{
					// skip
					if(!initial && haveDontSend(i, r.jid))
						continue;

					// TODO: if this roster item is in the dplist, remove from the dplist?

					Stanza out = in;
					out.setFrom(u);
					out.setTo(r.jid);
					writeFromHost(out);
				}
			}

			// send unavailable to all jids that the user has sent available directed presence to
			if(!initial && !avail)
			{
				for(int n = 0; n < dplist.count(); ++n)
				{
					DPItem &di = dplist[n];
					if(di.user.compare(u))
					{
						Stanza out = in;
						out.setFrom(u);
						out.setTo(di.to);
						writeFromHost(out);
					}
				}
			}

			// if going unavailable, remove all state
			if(!avail)
			{
				list.removeAt(index);
				for(int n = 0; n < dplist.count(); ++n)
				{
					DPItem &di = dplist[n];
					if(di.user.compare(u))
					{
						dplist.removeAt(n);
						--n; // adjust position
					}
				}
			}

			// send to all contacts on this server where
			//   the user's roster contains target of from/both
			//   the target's roster contains user of to/both
			// NOTE: we can cheat and not do the above, based on the way we loopback things
		}
	}

	void incomingFromOutsideNormal(const Stanza &in)
	{
		Jid u = router.userSessionJid(in.to());

		// better be going to one of our own users
		if(u.isEmpty())
		{
			// skip
			return;
		}

		if(in.type() == "error")
		{
			int index = findItem(u);

			// we haven't sent presence?
			if(index == -1)
			{
				// skip
				return;
			}

			Item &i = list[index];

			// FIXME: deal only with bare addresses, or?
			if(!haveDontSend(i, in.from()))
				i.dontsend += in.from();

			// pass to the client
			router.write(in);
		}
		else if(in.type() == "probe")
		{
			// is there a directed presence?
			int index = findDPItem(u, in.from());
			if(index != -1)
			{
				DPItem &di = dplist[index];
				Stanza out = di.stanza;
				out.setFrom(u);
				out.setTo(di.to);
				router.write(out);
				return;
			}

			// look up regular presence
			index = findItem(u);

			User user = loadUser(u.node());

			// is the sender from the roster?
			for(int n = 0; n < user.roster.count(); ++n)
			{
				RosterItem &r = user.roster[n];
				if(r.jid.compare(in.from(), false)) // bare-jid compare
				{
					if(r.sub == "from" || r.sub == "both")
					{
						Stanza out;
						if(index != -1)
						{
							out = list[index].stanza;
							out.setFrom(u);
							out.setTo(in.from());
						}
						else
						{
							out = Stanza(XMPP::Stanza::Presence, in.from(), "unavailable");
							out.setFrom(u);
						}
						router.write(out);
					}
					else
					{
						// TODO: auth error
					}
				}
				else
				{
					// TODO: some error
				}
			}
		}
		else
		{
			router.write(in);
		}
	}
};

class App : public QObject
{
	Q_OBJECT
public:
	Router r;
	QString host;
	bool c2s_ssl;

	//QList<PresenceItem> pres;
	PresenceManager presman;

	/*int indexOfPres(const Jid &j)
	{
		for(int n = 0; n < pres.count(); ++n)
		{
			//printf(" -> [%s]\n", qPrintable(pres[n].jid.full()));
			if(pres[n].jid.compare(j))
				return n;
		}
		return -1;
	}*/

	App(const QString &_host, const QCA::Cert &cert, const QCA::RSAKey &key) : presman(&r)
	{
		host = _host;
		presman.host = host;
		if(!cert.isNull())
		{
			c2s_ssl = true;
			r.setCertificate(cert, key);
		}
		else
			c2s_ssl = false;
		connect(&r, SIGNAL(readyRead(const XMPP::Stanza &)), SLOT(router_readyRead(const XMPP::Stanza &)));
		connect(&r, SIGNAL(userSessionGone(const XMPP::Jid &)), SLOT(router_userSessionGone(const XMPP::Jid &)));
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
	void router_userSessionGone(const XMPP::Jid &jid)
	{
		printf("user session gone: [%s]\n", qPrintable(jid.full()));
		Stanza fake_in(XMPP::Stanza::Presence, Jid(), "unavailable");
		fake_in.setFrom(jid);
		QDomElement e = fake_in.createElement("jabber:client", "status");
		e.appendChild(fake_in.doc().createTextNode("Disconnected"));
		fake_in.appendChild(e);
		router_readyRead(fake_in);
	}

	void router_readyRead(const XMPP::Stanza &in)
	{
		Jid jhost = host;
		Jid inhost = in.from().domain();
		bool local = jhost.compare(inhost);
		QString user;
		if(local)
			user = in.from().node();

		if(in.kind() == Stanza::IQ)
		{
			if(local)
			{
				if(subns(in) == NS_IMSESSION)
				{
					if(in.type() != "get" && in.type() != "set")
						return;

					Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
					r.write(out);
				}
				else if(subns(in) == NS_ROSTER)
				{
					if(in.type() != "get" && in.type() != "set")
						return;

					if(in.type() == "get")
					{
						User u = loadUser(user);
						Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
						out.setFrom(in.from());
						out.appendChild(u.roster.toQueryXml(&out.doc()));
						r.write(out);
					}
					else if(in.type() == "set")
					{
						User u = loadUser(user);
						RosterChangeList changes;
						if(!changes.fromXml(subelement(in.element(), NS_ROSTER, "query")))
							return;

						Roster changedItems = u.roster.applyChanges(changes);
						saveUser(user, u);

						// ack
						Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
						out.setFrom(in.from());
						r.write(out);

						// broadcast
						out = Stanza(XMPP::Stanza::IQ, in.from(), "set");
						out.setFrom(in.from());
						out.appendChild(changedItems.toQueryXml(&out.doc()));
						r.write(out);

						// TODO: if deleting a contact, send unsubscribed to target
						// TODO: broadcast changes to all logged in resources
					}
					return;
				}
			}

			if(subns(in) == NS_VCARD)
			{
				if(in.type() != "get" && in.type() != "set")
					return;

				QDomElement sub = subelement(in.element(), NS_VCARD, "vCard");
				if(!sub.isNull())
				{
					if(in.type() == "get")
					{
						QString user = in.to().node();

						User u = loadUser(user);

						Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
						out.setFrom(in.to());
						if(!u.vcard.isNull())
							out.appendChild(u.vcard);

						// return static vcard
						/*QDomElement v = out.createElement("vcard-temp", "vCard");
						v.setAttribute("prodid", "-//HandGen//NONSGML vGen v1.0//EN");
						v.setAttribute("version", "2.0");
						QDomElement nick = out.createElement("vcard-temp", "NICKNAME");
						nick.appendChild(out.doc().createTextNode(in.to().node()));
						v.appendChild(nick);
						out.appendChild(v);*/

						r.write(out);
					}
					else if(in.type() == "set")
					{
						if(local)
						{
							User u = loadUser(user);
							u.vcard = sub;
							saveUser(user, u);

							Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
							//out.setFrom(in.from());
							r.write(out);
						}
						else
						{
							// TODO: error
						}
					}
				}
				else
				{
					// TODO: error
				}
			}
			else if(in.to().compare(jhost) && subns(in) == NS_VERSION)
			{
				if(in.type() != "get")
					return;

				QDomElement sub = subelement(in.element(), NS_VERSION, "query");
				if(!sub.isNull())
				{
					Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
					out.setFrom(in.to());
					QDomElement q = out.createElement(NS_VERSION, "query");
					QDomElement name = out.createElement(NS_VERSION, "name");
					name.appendChild(out.doc().createTextNode("Ambrosia"));
					q.appendChild(name);
					QDomElement version = out.createElement(NS_VERSION, "version");
					version.appendChild(out.doc().createTextNode(SERVER_VERSION));
					q.appendChild(version);
					//QDomElement os = out.createElement(NS_VERSION, "os");
					//os.appendChild(out.doc().createTextNode("unknown"));
					//q.appendChild(os);
					out.appendChild(q);
					r.write(out);
				}
			}
			else
			{
				// TODO: return service unavailable
				//Stanza out(XMPP::Stanza::IQ, in.from(), "result", in.id());
				//r.write(out);

				r.write(in);
			}
			return;
		}
		else if(in.kind() == Stanza::Presence)
		{
			if(local)
			{
				Jid u = r.userSessionJid(in.from());
				presman.incomingFromClient(in, u);
			}
			else
				presman.incomingFromOutside(in);

			// directed presence?
			/*bool dp = !in.to().isEmpty();

			if(in.type() == "unavailable")
			{
				if(local)
				{
					printf("unavailable packet! [%s]\n", qPrintable(in.from().full()));

					// TODO: send unavailable to all watchers
					// TODO: don't forget to send to directed presence too!

					int index = indexOfPres(in.from());
					if(index == -1)
					{
						printf("no such presence\n");
						return;
					}

					PresenceItem &i = pres[index];

					// send to all watchers
					for(int n = 0; n < i.watchers.count(); ++n)
					{
						printf("  delivering unavailable presence\n");
						Stanza out = in;
						out.setTo(i.watchers[n]);
						out.setFrom(i.jid);
						r.write(out);
					}
					pres.removeAt(index);
				}
				else
				{
					// forward
					r.write(in);
				}
			}
			else if(in.type() == "subscribe")
			{
				if(in.to().isEmpty())
					return;

				if(local)
				{
					// TODO: if not subscribed to target, then:
					//   - add to roster if not already there
					//   - send to destination
					//   - ask=subscribe

					User u = loadUser(user);
					int index = -1;
					for(int n = 0; n < u.roster.count(); ++n)
					{
						if(u.roster[n].jid.compare(in.to()))
						{
							index = n;
							break;
						}
					}

					bool is_new = false;
					if(index == -1)
					{
						is_new = true;
						RosterItem new_item;
						new_item.jid = in.to();
						new_item.sub = "none";
						new_item.ask = false;
						index = u.roster.count();
						u.roster += new_item;
					}

					RosterItem &ri = u.roster[index];

					Roster changedItems;

					if(is_new || (!ri.ask && ri.sub != "to" && ri.sub != "both"))
					{
						ri.ask = true;
						changedItems += ri;
					}

					saveUser(user, u);

					if(!changedItems.isEmpty())
					{
						// broadcast
						Stanza out(XMPP::Stanza::IQ, in.from(), "set");
						out.setFrom(in.from());
						out.appendChild(changedItems.toQueryXml(&out.doc()));
						r.write(out);
					}

					// forward the presence
					Stanza tmp = in;
					tmp.setFrom(tmp.from().withResource(""));
					r.write(tmp);
				}
				else
				{
					// TODO: if not subscribed to target, then:
					//   - send to destination

					Jid userTo = r.userSessionJid(in.to());
					if(userTo.isEmpty())
						return;

					QString user = userTo.node();

					User u = loadUser(user);
					int index = -1;
					for(int n = 0; n < u.roster.count(); ++n)
					{
						if(u.roster[n].jid.compare(in.from()))
						{
							index = n;
							break;
						}
					}

					if(index == -1)
					{
						// forward
						r.write(in);
						return;
					}

					// forward
					if(u.roster[index].sub == "none" || u.roster[index].sub == "to")
						r.write(in);
				}
			}
			else if(in.type() == "subscribed")
			{
				if(in.to().isEmpty())
					return;

				if(local)
				{
					// TODO:

					User u = loadUser(user);
					int index = -1;
					for(int n = 0; n < u.roster.count(); ++n)
					{
						if(u.roster[n].jid.compare(in.to()))
						{
							index = n;
							break;
						}
					}

					if(index == -1)
						return;

					Roster changedItems;
					if(u.roster[index].sub == "none" || u.roster[index].sub == "to")
					{
						if(u.roster[index].sub == "none")
							u.roster[index].sub = "from";
						else
							u.roster[index].sub = "both";
						changedItems += u.roster[index];
					}

					saveUser(user, u);

					if(!changedItems.isEmpty())
					{
						// forward
						Stanza tmp = in;
						tmp.setFrom(tmp.from().withResource(""));
						r.write(tmp);

						// broadcast
						Stanza out(XMPP::Stanza::IQ, in.from(), "set");
						out.setFrom(in.from());
						out.appendChild(changedItems.toQueryXml(&out.doc()));
						r.write(out);
					}
				}
				else
				{
					Jid userTo = r.userSessionJid(in.to());
					if(userTo.isEmpty())
						return;

					QString user = userTo.node();

					User u = loadUser(user);
					int index = -1;
					for(int n = 0; n < u.roster.count(); ++n)
					{
						if(u.roster[n].jid.compare(in.from()))
						{
							index = n;
							break;
						}
					}

					if(index == -1)
						return;

					Roster changedItems;
					if(u.roster[index].sub == "none" || u.roster[index].sub == "from")
					{
						if(u.roster[index].sub == "none")
							u.roster[index].sub = "to";
						else
							u.roster[index].sub = "both";
						u.roster[index].ask = false;
						changedItems += u.roster[index];
					}

					saveUser(user, u);

					if(!changedItems.isEmpty())
					{
						// forward
						r.write(in);

						// broadcast
						Stanza out(XMPP::Stanza::IQ, userTo, "set");
						out.setFrom(userTo);
						out.appendChild(changedItems.toQueryXml(&out.doc()));
						r.write(out);

						// presence probe
						int x = indexOfPres(userTo);
						if(x == -1)
							return;

						PresenceItem &i = pres[x];

						out = Stanza(Stanza::Presence, u.roster[index].jid, "probe");
						out.setFrom(userTo);
						r.write(out);

						i.watchers += u.roster[index].jid;

						out = i.stanza;
						out.setTo(u.roster[index].jid);
						out.setFrom(i.jid);
						r.write(out);
					}
				}
			}
			else if(in.type() == "unsubscribe")
			{
				if(in.to().isEmpty())
					return;

				if(local)
				{
					//User u = loadUser(user);
					//for(int n = 0; n < u.roster.count(); ++n)
					//{
					//}
				}
				else
				{
				}
			}
			else if(in.type() == "unsubscribed")
			{
				if(in.to().isEmpty())
					return;

				if(local)
				{
					//User u = loadUser(user);
					//for(int n = 0; n < u.roster.count(); ++n)
					//{
					//}
				}
				else
				{
				}
			}
			else if(in.type() == "probe")
			{
				if(in.to().isEmpty())
					return;

				Jid userTo = r.userSessionJid(in.to());
				if(userTo.isEmpty())
					return;

				QString user = userTo.node();

				User u = loadUser(user);

				// are they allowed to be added as a watcher?
				int index = -1;
				for(int n = 0; n < u.roster.count(); ++n)
				{
					if(u.roster[n].jid.compare(in.from(), false))
					{
						index = n;
						break;
					}
				}

				if(index == -1)
				{
					// TODO: bounce
					return;
				}

				if(u.roster[index].sub != "from" && u.roster[index].sub != "both")
				{
					// TODO: bounce
					return;
				}

				int x = indexOfPres(userTo);
				if(x == -1)
				{
					// TODO: not online
					return;
				}

				Stanza out = pres[x].stanza;
				out.setTo(in.from());
				out.setFrom(pres[x].jid);
				r.write(out);

				// add as a watcher, and send
				//for(int n = 0; n < pres.count(); ++n)
				//{
				//	if(pres[n].jid.compare(in.to(), false))
				//	{
				//		pres[n].watchers += in.from();

				//		Stanza out = pres[n].stanza;
				//		out.setTo(in.from());
				//		out.setFrom(pres[n].jid);
				//		r.write(out);
				//	}
				//}
			}
			else if(in.type() == "error")
			{
				// TODO
				r.write(in);
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

						//Jid userTo = r.userSessionJid(in.to());
						//if(userTo.isEmpty())
						//	return;
						//Jid from = in.from();

						//for(int n = 0; n < pres.count(); ++n)
						//{
						//	PresenceItem &pi = pres[n];
						//	for(int k = 0; k < pi.watchers.count(); ++k)
						//	{
						//		if(pi.watchers[k].compare(from, false))
						//			pi.watchers[k] = from;
						//	}
						//}

						r.write(in);
					}
				}
				else
				{
					if(local)
					{
						// TODO: deliver to local watchers
						// TODO: send to self resources
						// TODO: if this is the first time, probe for other presence

						User u = loadUser(user);

						int index = indexOfPres(in.from());
						if(index == -1)
						{
							PresenceItem i;
							i.jid = in.from();
							index = pres.count();
							pres += i;
						}

						PresenceItem &i = pres[index];
						i.stanza = in;

						// send to all watchers
						for(int n = 0; n < i.watchers.count(); ++n)
						{
							Stanza out = i.stanza;
							out.setTo(i.watchers[n]);
							out.setFrom(i.jid);
							r.write(out);
						}

						// probe for all contacts
						for(int n = 0; n < u.roster.count(); ++n)
						{
							if(u.roster[n].sub == "to" || u.roster[n].sub == "both")
							{
								Stanza out(Stanza::Presence, u.roster[n].jid, "probe");
								out.setFrom(in.from());
								r.write(out);
							}

							if(u.roster[n].sub == "from" || u.roster[n].sub == "both")
							{
								bool found = false;
								for(int k = 0; k < i.watchers.count(); ++k)
								{
									if(i.watchers[k].compare(u.roster[n].jid, false))
									{
										found = true;
										break;
									}
								}

								if(!found)
								{
									i.watchers += u.roster[n].jid;

									Stanza out = i.stanza;
									out.setTo(u.roster[n].jid);
									out.setFrom(i.jid);
									r.write(out);
								}
							}
						}
					}
					else
					{
						// TODO: can't have broadcasted presence from non-local
					}
				}
			}*/
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
