/*
 * bconsole.cpp - ByteStream wrapper for stdin/stdout
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

#include"bconsole.h"

#include<qsocketnotifier.h>
#include<unistd.h>
#include<fcntl.h>

//----------------------------------------------------------------------------
// BConsole
//----------------------------------------------------------------------------
class BConsole::Private
{
public:
	Private() {}

	QSocketNotifier *r, *w;
	bool closing;
	bool closed;
};

BConsole::BConsole(QObject *parent)
:ByteStream(parent)
{
	d = new Private;
	d->closing = false;
	d->closed = false;

	// set stdin/stdout to non-block
	fcntl(0, F_SETFL, O_NONBLOCK);
	fcntl(1, F_SETFL, O_NONBLOCK);

	d->r = new QSocketNotifier(0, QSocketNotifier::Read);
	connect(d->r, SIGNAL(activated(int)), SLOT(sn_read()));
	d->w = 0;
}

BConsole::~BConsole()
{
	delete d->w;
	delete d->r;
	delete d;
}

bool BConsole::isOpen() const
{
	return (!d->closing && !d->closed);
}

void BConsole::close()
{
	if(d->closing || d->closed)
		return;

	if(bytesToWrite() > 0)
		d->closing = true;
	else {
		::fclose(stdout);
		d->closed = true;
	}
}

void BConsole::sn_read()
{
	QByteArray a(1024, 0);
	int r = ::read(0, a.data(), a.size());
	if(r < 0) {
		error(ErrRead);
	}
	else if(r == 0) {
		connectionClosed();
	}
	else {
		a.resize(r);
		appendRead(a);
		readyRead();
	}
}

void BConsole::sn_write()
{
	d->w->deleteLater();
	d->w = 0;

	if(bytesToWrite() > 0)
		tryWrite();
	if(bytesToWrite() == 0 && d->closing) {
		d->closing = false;
		d->closed = true;
		delayedCloseFinished();
	}
}

int BConsole::tryWrite()
{
	// try a section of the write buffer
	QByteArray a = takeWrite(1024, false);

	// write it
	int r = ::write(1, a.data(), a.size());
	if(r < 0) {
		error(ErrWrite);
		return -1;
	}

	d->w = new QSocketNotifier(1, QSocketNotifier::Write);
	connect(d->w, SIGNAL(activated(int)), SLOT(sn_write()));

	takeWrite(r);
	bytesWritten(r);
	return r;
}
