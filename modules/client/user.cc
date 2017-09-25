/*
 * Copyright (C) 2017 Charybdis Development Team
 * Copyright (C) 2017 Jason Volk <jason@zemos.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice is present in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

using namespace ircd;

resource user_resource
{
	"_matrix/client/r0/user",
	"User resource",
	{
		resource::DIRECTORY
	}
};

resource::response
get_filter(client &client, const resource::request &request)
try
{
	const m::user::id user_id
	{
		token(request.head.path, '/', 4)
	};

	const auto filter_id
	{
		token(request.head.path, '/', 6)
	};

	const m::event::where::equal query
	{
		{ "type",        "ircd.filter" },
		{ "sender",       user_id      },
		{ "state_key",    filter_id    }
	};

	const auto result{[&client]
	(const m::event &event)
	{
		const json::object &filter
		{
			json::at<m::name::content>(event)
		};

		resource::response
		{
			client, filter
		};

		return true;
	}};

	const m::room::events filters_room_events
	{
		m::filter::filters
	};

	if(!filters_room_events.any(query, result))
		throw m::NOT_FOUND("No matching filter with that ID");

	// Response already made
	return {};
}
catch(...)
{
	throw;
}

resource::method get
{
	user_resource, "GET", get_filter,
	{
		resource::method::REQUIRES_AUTH
	}
};

// (5.2) Uploads a new filter definition to the homeserver. Returns a filter ID that
// may be used in future requests to restrict which events are returned to the client.
resource::response
post_filter(client &client, const resource::request::object<const m::filter> &request)
try
{
	const auto &user_id
	{
		// (5.2) Required. The id of the user uploading the filter. The access
		// token must be authorized to make requests for this user id.
		token(request.head.path, '/', 4)
	};

	const auto &event_fields
	{
		// (5.2) List of event fields to include. If this list is absent then all fields are
		// included. The entries may include '.' charaters to indicate sub-fields. So
		// ['content.body'] will include the 'body' field of the 'content' object. A literal '.'
		// character in a field name may be escaped using a '\'. A server may include more
		// fields than were requested.
		json::get<m::name::event_fields>(request)
	};

	const auto &event_format
	{
		// (5.2) The format to use for events. 'client' will return the events in a format suitable
		// for clients. 'federation' will return the raw event as receieved over federation.
		// The default is 'client'. One of: ["client", "federation"]
		json::get<m::name::event_format>(request)
	};

	const auto &account_data
	{
		// (5.2) The user account data that isn't associated with rooms to include.
		json::val<m::name::account_data>(request)
	};

	const auto &room
	{
		// (5.2) Filters to be applied to room data.
		json::val<m::name::room>(request)
	};

	const auto &state
	{
		json::val<m::name::state>(room)
	};

	std::cout << json::get<m::name::limit>(state) << std::endl;

	const auto &presence
	{
		// (5.2) The presence updates to include.
		json::val<m::name::presence>(request)
	};

	const auto filter_id
	{
		"abc123"
	};

	json::iov event;
	json::iov::push members[]
	{
		{ event, json::member { "type",      "ircd.filter"    }},
		{ event, json::member { "state_key",  filter_id       }},
		{ event, json::member { "sender",     user_id         }},
		{ event, json::member { "content",    request.body    }}
	};

	m::filter::filters.send(event);
	return resource::response
	{
		client, http::CREATED,
		{
			{ "filter_id", filter_id }
		}
	};
}
catch(...)
{
	throw;
}

resource::method post
{
	user_resource, "POST", post_filter,
	{
		post.REQUIRES_AUTH
	}
};

mapi::header IRCD_MODULE
{
	"registers the resource 'client/user' to handle requests"
};
