// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

#include "rooms.h"

using namespace ircd;

m::resource::response
post__join(client &client,
           const m::resource::request &request,
           const m::room::id &room_id)
{
	const json::string &third_party_signed
	{
		request["third_party_signed"]
	};

	const json::string &server_name
	{
		request["server_name"]
	};

	const m::room room
	{
		room_id
	};

	m::join(room, request.user_id);

	return m::resource::response
	{
		client, json::members
		{
			{ "room_id", room_id }
		}
	};
}
