// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

namespace ircd::m::sync
{
	static long _notification_count(const room &, const event::idx &a, const event::idx &b);
	static long _highlight_count(const room &, const user &u, const event::idx &a, const event::idx &b);
	static bool room_unread_notifications_polylog(data &);
	static bool room_unread_notifications_linear(data &);

	extern item room_unread_notifications;
}

ircd::mapi::header
IRCD_MODULE
{
	"Client Sync :Room Unread Notifications"
};

decltype(ircd::m::sync::room_unread_notifications)
ircd::m::sync::room_unread_notifications
{
	"rooms.unread_notifications",
	room_unread_notifications_polylog,
	room_unread_notifications_linear,
};

bool
ircd::m::sync::room_unread_notifications_linear(data &data)
{
	if(!data.event_idx)
		return false;

	if(!data.room)
		return false;

	const bool is_user_room
	{
		data.room->room_id == data.user_room.room_id
	};

	assert(data.event);
	const auto &type
	{
		json::get<"type"_>(*data.event)
	};

	const bool is_self_read
	{
		is_user_room &&
		type == "ircd.read"
	};

	const bool is_push_note
	{
		is_user_room &&
		startswith(type, user::notifications::type_prefix)
	};

	const user::notifications::opts note_opts
	{
		is_push_note?
			user::notifications::unmake_type(type):
			user::notifications::opts{}
	};

	const m::room &room
	{
		is_self_read?
			room::id{at<"state_key"_>(*data.event)}:
		is_push_note && note_opts.room_id?
			note_opts.room_id:
			data.room->room_id
	};

	char membuf[32];
	const string_view &membership
	{
		room.room_id != data.room->room_id?
			m::membership(membuf, room, data.user):
			data.membership
	};

	if(!membership)
		return false;

	// skips state events only until a non-state event is seen.
	if(likely(!is_user_room))
		if(defined(json::get<"state_key"_>(*data.event)))
			return false;

	// skips old events the server has backfilled in the background.
	if(likely(!is_user_room))
		if(room::events::viewport_size >= 0)
			if(json::get<"depth"_>(*data.event) + room::events::viewport_size < data.room_depth)
				return false;

	m::event::id::buf last_read;
	if(likely(!is_self_read))
		if(!m::receipt::get(last_read, room.room_id, data.user))
			return false;

	const auto start_idx
	{
		last_read?
			index(last_read):
		!is_self_read?
			index(room):
			0UL
	};

	json::stack::object rooms
	{
		*data.out, "rooms"
	};

	json::stack::object membership_
	{
		*data.out, membership
	};

	json::stack::object room_
	{
		*data.out, room.room_id
	};

	json::stack::object unread_notifications
	{
		*data.out, "unread_notifications"
	};

	const event::idx upper_bound
	{
		std::max(data.range.second, data.event_idx + 1)
	};

	const auto notification_count
	{
		start_idx && !is_self_read?
			_notification_count(room, start_idx, upper_bound):
			0L
	};

	json::stack::member
	{
		*data.out, "notification_count", json::value
		{
			notification_count
		}
	};

	json::stack::member
	{
		*data.out, "highlight_count", json::value
		{
			notification_count?
				_highlight_count(room, data.user, start_idx, upper_bound):
				0L
		}
	};

	return true;
}

bool
ircd::m::sync::room_unread_notifications_polylog(data &data)
{
	if(!data.membership)
		return false;

	assert(data.room);
	const auto &room
	{
		*data.room
	};

	m::event::id::buf last_read_buf;
	const auto last_read
	{
		m::receipt::get(last_read_buf, room.room_id, data.user)
	};

	const auto start_idx
	{
		last_read?
			index(last_read):
		data.membership == "join"?
			room::index(room):
			0UL
	};

	if(!apropos(data, start_idx))
		return false;

	const auto notification_count
	{
		_notification_count(room, start_idx, data.range.second)
	};

	json::stack::member
	{
		*data.out, "notification_count", json::value
		{
			notification_count
		}
	};

	json::stack::member
	{
		*data.out, "highlight_count", json::value
		{
			notification_count?
				_highlight_count(room, data.user, start_idx, data.range.second):
				0L
		}
	};

	return true;
}

long
ircd::m::sync::_notification_count(const room &room,
                                   const event::idx &a,
                                   const event::idx &b)
{
	const event::idx_range range
	{
		std::minmax(a, b)
	};

	return room::events::count(room, range);
}

long
ircd::m::sync::_highlight_count(const room &room,
                                const user &user,
                                const event::idx &a,
                                const event::idx &b)
{
	const event::idx_range range
	{
		std::minmax(a, b)
	};

	const user::notifications notifications
	{
		user
	};

	user::notifications::opts opts;
	opts.room_id = room.room_id;
	opts.only = "highlight";
	opts.from = range.second;
	opts.to = range.first;
	const auto ret
	{
		notifications.count(opts)
	};

	return ret;
}
