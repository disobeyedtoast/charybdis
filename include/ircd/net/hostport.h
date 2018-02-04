// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

#pragma once
#define HAVE_IRCD_NET_HOSTPORT_H

namespace ircd::net
{
	struct hostport;

	const uint16_t &port(const hostport &);
	const string_view &host(const hostport &);
	const string_view &service(const hostport &);

	uint16_t &port(hostport &);
	string_view &host(hostport &);

	string_view string(const mutable_buffer &out, const hostport &);
}

namespace ircd
{
	using net::hostport;
	using net::host;
}

/// This structure holds a hostname and service or port usually fresh from
/// user input intended for resolution.
///
/// The host can be passed to the constructor as a hostname or hostname:port
/// string.
///
/// The service field is a string which will trigger an SRV query during
/// resolution and/or be used to figure out the port number. If it is omitted
/// then the numerical port should be specified directly. If it exists then
/// the numerical port will only be used as a fallback, and an SRV response
/// with an alternative port may even override both the host and given
/// numerical port here.
///
struct ircd::net::hostport
{
	string_view host {"0.0.0.0"};
	string_view service {"matrix"};
	uint16_t port {8448};

	bool operator!() const;

	hostport(const string_view &host, const string_view &service);
	hostport(const string_view &host, const uint16_t &port);
	hostport(const string_view &amalgam);
	hostport() = default;

	friend std::ostream &operator<<(std::ostream &, const hostport &);
};

inline
ircd::net::hostport::hostport(const string_view &host,
                              const string_view &service)
:host{host}
,service{service}
{}

inline
ircd::net::hostport::hostport(const string_view &host,
                              const uint16_t &port)
:host{host}
,service{}
,port{port}
{}

inline
ircd::net::hostport::hostport(const string_view &amalgam)
:host
{
	rsplit(amalgam, ':').first
}
,service{}
,port
{
	amalgam != host? lex_cast<uint16_t>(rsplit(amalgam, ':').second) : uint16_t(0)
}
{}

inline bool
ircd::net::hostport::operator!()
const
{
	static const hostport defaults{};
	return net::host(*this) == net::host(defaults);
}

inline ircd::string_view &
ircd::net::host(hostport &hp)
{
	return hp.host;
}

inline uint16_t &
ircd::net::port(hostport &hp)
{
	return hp.port;
}

inline const ircd::string_view &
ircd::net::service(const hostport &hp)
{
	return hp.service;
}

inline const ircd::string_view &
ircd::net::host(const hostport &hp)
{
	return hp.host;
}

inline const uint16_t &
ircd::net::port(const hostport &hp)
{
	return hp.port;
}
