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
#define HAVE_IRCD_M_FED_H

/// Federation Interface
namespace ircd::m::fed
{
	net::hostport matrix_service(net::hostport);

	id::event::buf fetch_head(const id::room &room_id, const string_view &remote, const id::user &);
	id::event::buf fetch_head(const id::room &room_id, const string_view &remote);

	string_view fetch_well_known(const mutable_buffer &out, const string_view &origin);
	string_view well_known(const mutable_buffer &out, const string_view &origin);
}

#include "request.h"
#include "version.h"
#include "key.h"
#include "query.h"
#include "user.h"
#include "user_keys.h"
#include "make_join.h"
#include "send_join.h"
#include "invite.h"
#include "invite2.h"
#include "event.h"
#include "event_auth.h"
#include "query_auth.h"
#include "state.h"
#include "backfill.h"
#include "frontfill.h"
#include "public_rooms.h"
#include "send.h"
#include "groups.h"

inline ircd::net::hostport
ircd::m::fed::matrix_service(net::hostport remote)
{
	net::service(remote) = net::service(remote)?: m::canon_service;
	return remote;
}
