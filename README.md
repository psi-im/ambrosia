# Ambrosia XMPP Server

Language: English | [Bulgarian](http://weknownyourdreamz.com/ambrosia.html)

## What is it?

Ambrosia is a proof of concept XMPP server. It does not have many features, and is not intended for use in a production environment. It was mainly created as an exercise to show that an XMPP server in C++ is possible by using [Iris](https://github.com/psi-im/iris), the same library that powers the [Psi client](http://psi-im.org).

## What do I need to be able to use it?

Ambrosia depends on OpenSSL 0.9.6+ and Cyrus SASL 2. Interestingly, even though Iris is Qt-based, Ambrosia does not externally depend on Qt, because the necessary Qt sources are in the Ambrosia package.

## What features are supported?

* Client connections, using SASL or iq:auth methods
* Server to server connectivity
* Message delivery
* Roster, Subscriptions, Presence
* VCards
* SSL/TLS and SASL-based encryption for clients

## What problems are there?

* No offline stored events (messages or subscription packets)
* Unsubscribing is not supported properly
* Probably tons of bugs and memory leaks

## How do I use it?

Build it with the usual ./configure, make, make install procedure. To run it, type "./ambrosia hostname", where hostname is the domain to service. Be sure that you can accept connections on ports 5222, 5223, and 5269. To use SASL auth, you probably have to run as root. To use non-sasl auth, be sure to edit the included plaintext userdb file.

## How was it made?

Version 0.1 was made in 3 days. Version 0.2 (the current version) was made in 3 more days. Given that the current featureset makes the server almost usable, this should say quite a lot about the power of Iris and Qt.

The basic explanation is this: First, I grabbed a snapshot of Qt 4, stripped it down to just the core/network classes, and set it up in such a way that it could be bundled (I’ve put this in the neatstuff/byoq (Bring Your Own Qt) CVS module). Next, QCA v1 and Iris were ported over to Qt 4. Next, missing Iris features were added (essentially mapping the public API to a lot of the server stuff, the code was pretty much already there). Finally, a basic server was written. The Ambrosia-specific code is actually quite small, around 1000 lines.

## What is the development plan?

There is none. This has so far just been a two time development spree, as if I do any open source programming it should probably be on my existing libraries. I just felt like doing something different for a weekend. If you want to pick up this project, let me know.
