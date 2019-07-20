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

mapi::header
IRCD_MODULE
{
	"Matrix m.room.canonical_alias"
};

const m::room::id::buf
alias_room_id
{
	"alias", ircd::my_host()
};

const m::room
alias_room
{
	alias_room_id
};

void
_changed_canonical_alias(const m::event &event,
                         m::vm::eval &)
{
	const m::room::id &room_id
	{
		at<"room_id"_>(event)
	};

	const m::room room
	{
		room_id, event.event_id
	};

	const json::object &content
	{
		at<"content"_>(event)
	};

	const m::room::alias &alias
	{
		content.has("alias")?
			m::room::alias{unquote(content.get("alias"))}:
			m::room::alias{}
	};

	log::info
	{
		m::log, "Changed canonical alias of %s to %s by %s with %s",
		string_view{room_id},
		string_view{alias},
		json::get<"sender"_>(event),
		string_view{event.event_id},
	};

	if(alias)
	{
		if(m::room::aliases::cache::has(alias))
			return;

		m::room::aliases::cache::set(alias, room_id);
		return;
	}

	const auto event_idx
	{
		room.get(std::nothrow, "m.room.canonical_alias", "")
	};

	if(!event_idx)
		return;

	m::get(std::nothrow, event_idx, "content", []
	(const json::object &content)
	{
		const json::string &alias
		{
			content["alias"]
		};

		m::room::aliases::cache::del(alias);
	});
}

const m::hookfn<m::vm::eval &>
_changed_canonical_alias_hookfn
{
	_changed_canonical_alias,
	{
		{ "_site",    "vm.effect"               },
		{ "type",     "m.room.canonical_alias"  },
	}
};
