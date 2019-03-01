// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2019 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

#pragma once
#define HAVE_IRCD_NET_IPADDR_H

// Forward declarations for boost because it is not included here.
namespace boost::asio::ip
{
	struct address;
};

namespace ircd::net
{
	union ipaddr;

	bool operator!(const ipaddr &);
	bool operator<(const ipaddr &, const ipaddr &);
	bool operator==(const ipaddr &, const ipaddr &);
}

union ircd::net::ipaddr
{
	uint32_t v4;
	uint128_t v6 {0};
	std::array<uint8_t, 16> byte;

	explicit operator bool() const;
};

static_assert(std::alignment_of<ircd::net::ipaddr>() >= 16);

inline ircd::net::ipaddr::operator
bool() const
{
	return bool(v6);
}

inline bool
ircd::net::operator==(const ipaddr &a, const ipaddr &b)
{
	return a.v6 == b.v6;
}

inline bool
ircd::net::operator<(const ipaddr &a, const ipaddr &b)
{
	return a.v6 < b.v6;
}

inline bool
ircd::net::operator!(const ipaddr &a)
{
	return !a.v6;
}