#include <QtCore>
#include <QtNetwork>
#include <QtXml>

#include <stdlib.h>
#include <time.h>

#include "qca.h"
#include "qca-tls.h"
#include "qca-sasl.h"

#include "bsocket.h"
#include "servsock.h"
#include "xmpp.h"

#include <unistd.h>

char pemdata_cert[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDbjCCAtegAwIBAgIBADANBgkqhkiG9w0BAQQFADCBhzELMAkGA1UEBhMCVVMx\n"
	"EzARBgNVBAgTCkNhbGlmb3JuaWExDzANBgNVBAcTBklydmluZTEYMBYGA1UEChMP\n"
	"RXhhbXBsZSBDb21wYW55MRQwEgYDVQQDEwtleGFtcGxlLmNvbTEiMCAGCSqGSIb3\n"
	"DQEJARYTZXhhbXBsZUBleGFtcGxlLmNvbTAeFw0wMzA3MjQwNzMwMDBaFw0wMzA4\n"
	"MjMwNzMwMDBaMIGHMQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTEP\n"
	"MA0GA1UEBxMGSXJ2aW5lMRgwFgYDVQQKEw9FeGFtcGxlIENvbXBhbnkxFDASBgNV\n"
	"BAMTC2V4YW1wbGUuY29tMSIwIAYJKoZIhvcNAQkBFhNleGFtcGxlQGV4YW1wbGUu\n"
	"Y29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCobzCF268K2sRp473gvBTT\n"
	"4AgSL1kjeF8N57vxS1P8zWrWMXNs4LuH0NRZmKTajeboy0br8xw+smIy3AbaKAwW\n"
	"WZToesxebu3m9VeA8dqWyOaUMjoxAcgVYesgVaMpjRe7fcWdJnX1wJoVVPuIcO8m\n"
	"a+AAPByfTORbzpSTmXAQAwIDAQABo4HnMIHkMB0GA1UdDgQWBBTvFierzLmmYMq0\n"
	"cB/+5rK1bNR56zCBtAYDVR0jBIGsMIGpgBTvFierzLmmYMq0cB/+5rK1bNR566GB\n"
	"jaSBijCBhzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlmb3JuaWExDzANBgNV\n"
	"BAcTBklydmluZTEYMBYGA1UEChMPRXhhbXBsZSBDb21wYW55MRQwEgYDVQQDEwtl\n"
	"eGFtcGxlLmNvbTEiMCAGCSqGSIb3DQEJARYTZXhhbXBsZUBleGFtcGxlLmNvbYIB\n"
	"ADAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBAUAA4GBAGqGhXf7xNOnYNtFO7gz\n"
	"K6RdZGHFI5q1DAEz4hhNBC9uElh32XGX4wN7giz3zLC8v9icL/W4ff/K5NDfv3Gf\n"
	"gQe/+Wo9Be3H3ul6uwPPFnx4+PIOF2a5TW99H9smyxWdNjnFtcUte4al3RszcMWG\n"
	"x3iqsWosGtj6F+ridmKoqKLu\n"
	"-----END CERTIFICATE-----\n";

char pemdata_privkey[] =
	"-----BEGIN RSA PRIVATE KEY-----\n"
	"MIICXAIBAAKBgQCobzCF268K2sRp473gvBTT4AgSL1kjeF8N57vxS1P8zWrWMXNs\n"
	"4LuH0NRZmKTajeboy0br8xw+smIy3AbaKAwWWZToesxebu3m9VeA8dqWyOaUMjox\n"
	"AcgVYesgVaMpjRe7fcWdJnX1wJoVVPuIcO8ma+AAPByfTORbzpSTmXAQAwIDAQAB\n"
	"AoGAP83u+aYghuIcaWhmM03MLf69z/WztKYSi/fu0BcS977w67bL3MC9CVPoPRB/\n"
	"0nLSt/jZIuRzHKUCYfXLerSU7v0oXDTy6GPzWMh/oXIrpF0tYNbwWF7LSq2O2gGZ\n"
	"XtA9MSmUNNJaKzQQeXjqdVFOY8A0Pho+k2KByBiCi+ChkcECQQDRUuyX0+PKJtA2\n"
	"M36BOTFpy61BAv+JRlXUnHuevOfQWl6NR6YGygqCyH1sWtP1sa9S4wWys3DFH+5A\n"
	"DkuAqk7zAkEAzf4eUH2hp5CIMsXH+WpIzKj09oY1it2CAKjVq4rUELf8iXvmGoFl\n"
	"000spua4MjHNUYm7LR0QaKesKrMyGZUesQJAL8aLdYPJI+SD9Tr/jqLtIkZ4frQe\n"
	"eshw4pvsoyheiHF3zyshO791crAr4EVCx3sMlxB1xnmqLXPCPyCEHxO//QJBAIBY\n"
	"IYkjDZJ6ofGIe1UyXJNvfdkPu9J+ut4wU5jjEcgs6mK62J6RGuFxhy2iOQfFMdjo\n"
	"yL+OCUg7mDCun7uCxrECQAtSvnLOFMjO5qExRjFtwi+b1rcSekd3Osk/izyRFSzg\n"
	"Or+AL56/EKfiogNnFipgaXIbb/xj785Cob6v96XoW1I=\n"
	"-----END RSA PRIVATE KEY-----\n";

QCA::Cert *cert;
QCA::RSAKey *privkey;

using namespace XMPP;

int id_num = 0;

QStringList servers;
QList<ClientStream*> servers_p;

class Session : public QObject
{
	Q_OBJECT
public:
	QString serverHost;
	Jid other_person;
	QString other_msg;

	Session(const QString &host, const QString &defRealm, ByteStream *bs, bool sslnow, bool s2s = false) : QObject(0)
	{
		id = id_num++;
		serverHost = host;

		tls = new QCA::TLS;
		tls->setCertificate(*cert, *privkey);

		cs = new ClientStream(host, defRealm, bs, tls, sslnow, s2s);
		connect(cs, SIGNAL(connectionClosed()), SLOT(cs_connectionClosed()));
		connect(cs, SIGNAL(error(int)), SLOT(cs_error(int)));
		connect(cs, SIGNAL(readyRead()), SLOT(cs_readyRead()));
		connect(cs, SIGNAL(authenticated()), SLOT(cs_authenticated()));
		connect(cs, SIGNAL(dialbackRequest(const Jid &, const Jid &, const QString &)), SLOT(cs_dialbackRequest(const Jid &, const Jid &, const QString &)));
		connect(cs, SIGNAL(dialbackVerifyRequest(const Jid &, const Jid &, const QString &, const QString &)), SLOT(cs_dialbackVerifyRequest(const Jid &, const Jid &, const QString &, const QString &)));
	}

	~Session()
	{
		delete cs;
		delete tls;
		printf("[%d]: deleted\n", id);
	}

	void start()
	{
		printf("[%d]: New session!\n", id);
		cs->accept();
	}

signals:
	void done();

private slots:
	void cs_connectionClosed()
	{
		printf("[%d]: Connection closed by peer\n", id);
		done();
	}

	void cs_error(int)
	{
		printf("[%d]: Error\n", id);
		done();
	}

	void cs_authenticated()
	{
		printf("<<< Authenticated >>>\n");
	}

	void cs_dialbackRequest(const Jid &to, const Jid &from, const QString &key)
	{
		printf("Dialback Request: to=[%s], from=[%s], key=[%s]\n", to.full().toLatin1().data(), from.full().toLatin1().data(), key.toLatin1().data());
		// TODO: make sure the 'to' is us

		AdvancedConnector *conn = new AdvancedConnector;
		ClientStream *s = new ClientStream(conn, 0);
		connect(s, SIGNAL(dialbackVerifyResult(const Jid &, bool)), SLOT(cs_dialbackVerifyResult(const Jid &, bool)));
		//s->connectToServerAsServerVerify("andbit.net", "infiniti.homelesshackers.org", cs->id(), key);
		s->connectToServerAsServerVerify(from, "infiniti.homelesshackers.org", cs->id(), key);
	}

	void cs_dialbackResult(const Jid &from, bool ok)
	{
		ClientStream *s = (ClientStream *)sender();

		printf("Dialback Result: from=[%s], ok=[%s]\n", from.full().toLatin1().data(), ok ? "yes" : "no");

		if(ok) {
			servers += from.full();
			servers_p += s;
			sendMsg(s, other_person, other_msg);
		}
	}

	void sendMsg(ClientStream *s, const Jid &to, const QString &text)
	{
		XMPP::Jid parrotJid;
		parrotJid.set(serverHost, "parrot");
		XMPP::Jid parrotFull = parrotJid.withResource("Olympus");

		// send a message from the parrot
		Stanza msg = s->createStanza(XMPP::Stanza::Message, to, "chat");
		msg.setFrom(parrotFull);

		QDomElement bo = msg.createElement("jabber:server", "body");
		bo.appendChild(msg.doc().createTextNode(text));
		msg.appendChild(bo);
		s->write(msg);
	}

	void cs_dialbackVerifyRequest(const Jid &to, const Jid &from, const QString &id, const QString &key)
	{
		ClientStream *s = (ClientStream *)sender();

		printf("Dialback Verify Request: to=[%s], from=[%s], key=[%s]\n", to.full().toLatin1().data(), from.full().toLatin1().data(), key.toLatin1().data());
		if(key != "123") { // TODO: also check id
			//s->dialbackVerifyRequestGrant("andbit.net", "infiniti.homelesshackers.org", QString(), false);
			s->dialbackVerifyRequestGrant(from, "infiniti.homelesshackers.org", QString(), false);
			return;
		}

		//s->dialbackVerifyRequestGrant("andbit.net", "infiniti.homelesshackers.org", id, true);
		s->dialbackVerifyRequestGrant(from, "infiniti.homelesshackers.org", id, true);
	}

	void cs_dialbackVerifyResult(const Jid &from, bool ok)
	{
		ClientStream *s = (ClientStream *)sender();
		s->deleteLater();

		printf("Dialback Verify Result: from=[%s], ok=[%s]\n", from.full().toLatin1().data(), ok ? "yes" : "no");
		if(ok) {
			//cs->dialbackRequestGrant("andbit.net", "infiniti.homelesshackers.org", true);
			cs->dialbackRequestGrant(from, "infiniti.homelesshackers.org", true);
		}
	}

	void cs_readyRead()
	{
		printf("cs_readyRead\n");
		while(cs->stanzaAvailable())
		{
			XMPP::Jid parrotJid;
			parrotJid.set(serverHost, "parrot");
			XMPP::Jid parrotFull = parrotJid.withResource("Olympus");

			Stanza s = cs->read();
			printf("Got Stanza: [%s]\n", s.toString().toLatin1().data());
			if(s.kind() == XMPP::Stanza::IQ)
			{
				Stanza result = cs->createStanza(XMPP::Stanza::IQ, XMPP::Jid(), "result", s.id());

				if(s.type() == "get")
				{
					QDomElement q;
					QDomNodeList nl = s.element().elementsByTagNameNS("jabber:iq:roster", "query");
					if(nl.count() > 0)
						q = nl.item(0).toElement();

					if(!q.isNull())
					{
						printf("Roster request\n");
						QDomElement e = result.createElement("jabber:iq:roster", "query");
						QDomElement i = result.createElement("jabber:iq:roster", "item");
						i.setAttribute("jid", parrotJid.full());
						i.setAttribute("name", "Delta Parrot");
						i.setAttribute("subscription", "both");
						e.appendChild(i);
						result.appendChild(e);
					}
					else
					{
						QDomNodeList nl = s.element().elementsByTagNameNS("vcard-temp", "vCard");
						if(nl.count() > 0)
							q = nl.item(0).toElement();
						if(!q.isNull())
						{
							QDomElement v = result.createElement("vcard-temp", "vCard");
							v.setAttribute("prodid", "-//HandGen//NONSGML vGen v1.0//EN");
							v.setAttribute("version", "2.0");
							QDomElement nick = result.createElement("vcard-temp", "NICKNAME");
							nick.appendChild(result.doc().createTextNode(cs->jid().node()));
							v.appendChild(nick);
							result.appendChild(v);
						}
					}
				}

				cs->write(result);
			}
			else if(s.kind() == XMPP::Stanza::Message)
			{
				QDomNodeList nl = s.element().elementsByTagName("body");
				QDomElement b;
				if(nl.count() > 0)
					b = nl.item(0).toElement();

				if(s.from().domain() != "infiniti.homelesshackers.org") {
					QString server = s.from().domain();
					if(servers.contains(server)) {
						int n = servers.indexOf(server);
						printf("[%s] is server %d\n", server.toLatin1().data(), n);
						sendMsg(servers_p[n], s.from(), b.text());
					}
					else {
						other_person = s.from();
						other_msg = b.text();
						AdvancedConnector *conn = new AdvancedConnector;
						ClientStream *s = new ClientStream(conn, 0);
						connect(s, SIGNAL(dialbackResult(const Jid &, bool)), SLOT(cs_dialbackResult(const Jid &, bool)));
						s->connectToServerAsServer(server, "infiniti.homelesshackers.org", "123");
					}
					return;
				}

				Stanza msg = cs->createStanza(XMPP::Stanza::Message, s.from(), "chat");
				msg.setFrom(parrotFull);

				QDomElement bo = msg.createElement("jabber:client", "body");
				bo.appendChild(s.doc().createTextNode(b.text()));
				msg.appendChild(bo);
				cs->write(msg);
			}
			else if(s.kind() == XMPP::Stanza::Presence)
			{
				// show online
				Stanza pres = cs->createStanza(XMPP::Stanza::Presence, cs->jid());
				pres.setFrom(parrotFull);
				QDomElement pri = pres.createElement("jabber:client", "priority");
				pri.appendChild(pres.doc().createTextNode("5"));
				pres.appendChild(pri);
				cs->write(pres);
			}
		}
	}

private:
	int id;
	ClientStream *cs;
	QCA::TLS *tls;
};

#include "q3serversocket.h"

class ServerTest : public QObject
{
	Q_OBJECT
public:
	//enum { Idle, Handshaking, Active, Closing };

	ServSock c2s, c2s_ssl, s2s;
	QString host, realm;
	QList<Session*> list;

	ServerTest(const QString &_host) : host(_host)
	{
		cert = new QCA::Cert;
		privkey = new QCA::RSAKey;

		cert->fromPEM(pemdata_cert);
		privkey->fromPEM(pemdata_privkey);

		connect(&c2s, SIGNAL(connectionReady(int)), SLOT(c2s_connectionReady(int)));
		connect(&c2s_ssl, SIGNAL(connectionReady(int)), SLOT(c2s_ssl_connectionReady(int)));
		connect(&s2s, SIGNAL(connectionReady(int)), SLOT(s2s_connectionReady(int)));

		//list.setAutoDelete(true);
	}

	~ServerTest()
	{
		qDeleteAll(list);
	}

	void start()
	{
		char buf[256];
		int r = gethostname(buf, sizeof(buf)-1);
		if(r == -1) {
			printf("Error getting hostname!\n");
			QTimer::singleShot(0, this, SIGNAL(quit()));
			return;
		}
		QString myhostname = buf;

		realm = myhostname;
		if(host.isEmpty())
			host = myhostname;

		if(cert->isNull() || privkey->isNull()) {
			printf("Error loading cert and/or private key!\n");
			QTimer::singleShot(0, this, SIGNAL(quit()));
			return;
		}

		if(!c2s.listen(5222) || !c2s_ssl.listen(5223) || !s2s.listen(5269)) {
			printf("Error binding to port 5222/5223/5269!\n");
			QTimer::singleShot(0, this, SIGNAL(quit()));
			return;
		}
		printf("Listening on %s:[5222,5223,5269] ...\n", host.toLatin1().data());
	}

signals:
	void quit();

private slots:
	void c2s_connectionReady(int s)
	{
		BSocket *bs = new BSocket;
		bs->setSocket(s);
		Session *sess = new Session(host, realm, bs, false);
		list.append(sess);
		connect(sess, SIGNAL(done()), SLOT(sess_done()));
		sess->start();
	}

	void c2s_ssl_connectionReady(int s)
	{
		BSocket *bs = new BSocket;
		bs->setSocket(s);
		Session *sess = new Session(host, realm, bs, true);
		list.append(sess);
		connect(sess, SIGNAL(done()), SLOT(sess_done()));
		sess->start();
	}

	void s2s_connectionReady(int s)
	{
		BSocket *bs = new BSocket;
		bs->setSocket(s);
		Session *sess = new Session(host, realm, bs, false, true);
		list.append(sess);
		connect(sess, SIGNAL(done()), SLOT(sess_done()));
		sess->start();
	}

	void sess_done()
	{
		Session *sess = (Session *)sender();
		delete sess;
		list.removeAll(sess);
	}
};

#include "server.moc"

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	if(argc < 2)
	{
		printf("Usage: ambrosia [hostname]\n\n");
		return 0;
	}

	QCA::init();
	QCA::insertProvider(createProviderTLS());
	QCA::insertProvider(createProviderSASL());

	QString host = argc > 1 ? QString(argv[1]) : QString();

	srand(time(NULL));

	ServerTest *s = new ServerTest(host);
	QObject::connect(s, SIGNAL(quit()), &app, SLOT(quit()));
	s->start();
	app.exec();
	delete s;

	// clean up
	QCA::unloadAllPlugins();

	return 0;
}

