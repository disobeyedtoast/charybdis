// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

using namespace ircd;

resource join_resource
{
	"/_matrix/client/r0/join/", resource::opts
	{
		"join",
		resource::DIRECTORY,
	}
};

mapi::header IRCD_MODULE
{
	"registers the resource 'client/join'"
};

resource::response
post_join(client &client, const resource::request &request)
{
	if(request.parv.size() < 1)
		throw http::error
		{
			http::MULTIPLE_CHOICES, "/join room_id required"
		};

	m::room::id::buf room_id
	{
		url::decode(request.parv[0], room_id)
	};

	m::join(room_id, request.user_id);

	return resource::response
	{
		client, json::members
		{
			{ "room_id", room_id }
		}
	};
}

resource::method method_post
{
	join_resource, "POST", post_join,
	{
		method_post.REQUIRES_AUTH
	}
};
