/*
 * router.h - ambrosia
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

#ifndef ROUTER_H
#define ROUTER_H

#include <QtCore>
#include <QtNetwork>
#include "qca.h"
#include "xmpp.h"

class Router : public QObject
{
	Q_OBJECT
public:
	Router();
	~Router();

	void setCertificate(const QCA::Cert &cert, const QCA::RSAKey &key);

	bool start(const QString &host);
	void stop();

	void write(const XMPP::Stanza &s);

	XMPP::Jid userSessionJid(const XMPP::Jid &possiblyBare);

signals:
	void readyRead(const XMPP::Stanza &);
	void stanzaWriteFailed();
	void userSessionGone(const XMPP::Jid &);

public:
	class Private;
private:
	class Session;
	Private *d;
};

#endif
