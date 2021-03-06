/*
 * jid.cpp - class for verifying and manipulating Jabber IDs
 * Copyright (C) 2003  Justin Karneges
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "xmpp.h"

#include <stringprep.h>

using namespace XMPP;

Jid::Jid()
{
	valid = false;
}

Jid::~Jid()
{
}

Jid::Jid(const QString &s)
{
	set(s);
}

Jid::Jid(const char *s)
{
	set(QString(s));
}

Jid & Jid::operator=(const QString &s)
{
	set(s);
	return *this;
}

Jid & Jid::operator=(const char *s)
{
	set(QString(s));
	return *this;
}

void Jid::reset()
{
	f = QString();
	b = QString();
	d = QString();
	n = QString();
	r = QString();
	valid = false;
}

void Jid::update()
{
	// build 'bare' and 'full' jids
	if(n.isEmpty())
		b = d;
	else
		b = n + '@' + d;
	if(r.isEmpty())
		f = b;
	else
		f = b + '/' + r;
	if(f.isEmpty())
		valid = false;
}

void Jid::set(const QString &s)
{
	QString rest, domain, node, resource;
	QString norm_domain, norm_node, norm_resource;
	int x = s.indexOf('/');
	if(x != -1) {
		rest = s.mid(0, x);
		resource = s.mid(x+1);
	}
	else {
		rest = s;
		resource = QString();
	}
	if(!validResource(resource, &norm_resource)) {
		reset();
		return;
	}

	x = rest.indexOf('@');
	if(x != -1) {
		node = rest.mid(0, x);
		domain = rest.mid(x+1);
	}
	else {
		node = QString();
		domain = rest;
	}
	if(!validDomain(domain, &norm_domain) || !validNode(node, &norm_node)) {
		reset();
		return;
	}

	valid = true;
	d = norm_domain;
	n = norm_node;
	r = norm_resource;
	update();
}

void Jid::set(const QString &domain, const QString &node, const QString &resource)
{
	QString norm_domain, norm_node, norm_resource;
	if(!validDomain(domain, &norm_domain) || !validNode(node, &norm_node) || !validResource(resource, &norm_resource)) {
		reset();
		return;
	}
	valid = true;
	d = norm_domain;
	n = norm_node;
	r = norm_resource;
	update();
}

void Jid::setDomain(const QString &s)
{
	if(!valid)
		return;
	QString norm;
	if(!validDomain(s, &norm)) {
		reset();
		return;
	}
	d = norm;
	update();
}

void Jid::setNode(const QString &s)
{
	if(!valid)
		return;
	QString norm;
	if(!validNode(s, &norm)) {
		reset();
		return;
	}
	n = norm;
	update();
}

void Jid::setResource(const QString &s)
{
	if(!valid)
		return;
	QString norm;
	if(!validResource(s, &norm)) {
		reset();
		return;
	}
	r = norm;
	update();
}

Jid Jid::withNode(const QString &s) const
{
	Jid j = *this;
	j.setNode(s);
	return j;
}

Jid Jid::withResource(const QString &s) const
{
	Jid j = *this;
	j.setResource(s);
	return j;
}

bool Jid::isValid() const
{
	return valid;
}

bool Jid::isEmpty() const
{
	return f.isEmpty();
}

bool Jid::compare(const Jid &a, bool compareRes) const
{
	// only compare valid jids
	if(!valid || !a.valid)
		return false;

	if(compareRes ? (f != a.f) : (b != a.b))
		return false;

	return true;
}

bool Jid::validDomain(const QString &s, QString *norm)
{
	QByteArray cs = s.toUtf8();
	cs.resize(1024);
	if(stringprep(cs.data(), 1024, (Stringprep_profile_flags)0, stringprep_nameprep) != 0)
		return false;
	if(norm)
		*norm = QString::fromUtf8(cs);
	return true;
}

bool Jid::validNode(const QString &s, QString *norm)
{
	QByteArray cs = s.toUtf8();
	cs.resize(1024);
	if(stringprep(cs.data(), 1024, (Stringprep_profile_flags)0, stringprep_xmpp_nodeprep) != 0)
		return false;
	if(norm)
		*norm = QString::fromUtf8(cs);
	return true;
}

bool Jid::validResource(const QString &s, QString *norm)
{
	QByteArray cs = s.toUtf8();
	cs.resize(1024);
	if(stringprep(cs.data(), 1024, (Stringprep_profile_flags)0, stringprep_xmpp_resourceprep) != 0)
		return false;
	if(norm)
		*norm = QString::fromUtf8(cs);
	return true;
}
