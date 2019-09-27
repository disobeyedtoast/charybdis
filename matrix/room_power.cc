// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2019 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

decltype(ircd::m::room::power::default_creator_level)
ircd::m::room::power::default_creator_level
{
	100
};

decltype(ircd::m::room::power::default_power_level)
ircd::m::room::power::default_power_level
{
	50
};

decltype(ircd::m::room::power::default_event_level)
ircd::m::room::power::default_event_level
{
	0
};

decltype(ircd::m::room::power::default_user_level)
ircd::m::room::power::default_user_level
{
	0
};

ircd::json::object
ircd::m::room::power::default_content(const mutable_buffer &buf,
                                      const m::user::id &creator)
{
	return compose_content(buf, [&creator]
	(const string_view &key, json::stack::object &object)
	{
		if(key != "users")
			return;

		assert(default_creator_level == 100);
		json::stack::member
		{
			object, creator, json::value(default_creator_level)
		};
	});
}

ircd::json::object
ircd::m::room::power::compose_content(const mutable_buffer &buf,
                                      const compose_closure &closure)
{
	json::stack out{buf};
	json::stack::object content{out};

	assert(default_power_level == 50);
	json::stack::member
	{
		content, "ban", json::value(default_power_level)
	};

	{
		json::stack::object events
		{
			content, "events"
		};

		closure("events", events);
	}

	assert(default_event_level == 0);
	json::stack::member
	{
		content, "events_default", json::value(default_event_level)
	};

	json::stack::member
	{
		content, "invite", json::value(default_power_level)
	};

	json::stack::member
	{
		content, "kick", json::value(default_power_level)
	};

	{
		json::stack::object notifications
		{
			content, "notifications"
		};

		json::stack::member
		{
			notifications, "room", json::value(default_power_level)
		};

		closure("notifications", notifications);
	}

	json::stack::member
	{
		content, "redact", json::value(default_power_level)
	};

	json::stack::member
	{
		content, "state_default", json::value(default_power_level)
	};

	{
		json::stack::object users
		{
			content, "users"
		};

		closure("users", users);
	}

	assert(default_user_level == 0);
	json::stack::member
	{
		content, "users_default", json::value(default_user_level)
	};

	content.~object();
	return json::object{out.completed()};
}

//
// room::power::power
//

ircd::m::room::power::power(const m::room &room)
:power
{
	room, room.get(std::nothrow, "m.room.power_levels", "")
}
{
}

ircd::m::room::power::power(const m::room &room,
                            const event::idx &power_event_idx)
:room
{
	room
}
,power_event_idx
{
	power_event_idx
}
{
}

ircd::m::room::power::power(const m::event &power_event,
                            const m::event &create_event)
:power
{
	power_event, m::user::id(unquote(json::get<"content"_>(create_event).get("creator")))
}
{
}

ircd::m::room::power::power(const m::event &power_event,
                            const m::user::id &room_creator_id)
:power
{
	json::get<"content"_>(power_event), room_creator_id
}
{
}

ircd::m::room::power::power(const json::object &power_event_content,
                            const m::user::id &room_creator_id)
:power_event_content
{
	power_event_content
}
,room_creator_id
{
	room_creator_id
}
{
}

/// "all who attain great power and riches make use of either force or fraud"
///
/// Returns bool for "allow" or "deny"
///
/// Provide the user invoking the power. The return value indicates whether
/// they have the power.
///
/// Provide the property/event_type. There are two usages here: 1. This is a
/// string corresponding to one of the spec top-level properties like "ban"
/// and "redact". In this case, the type and state_key parameters to this
/// function are not used. 2. This string is empty or "events" in which case
/// the type parameter is used to fetch the power threshold for that type.
/// For state events of a type, the state_key must be provided for inspection
/// here as well.
bool
ircd::m::room::power::operator()(const m::user::id &user_id,
                                 const string_view &prop,
                                 const string_view &type,
                                 const string_view &state_key)
const
{
	const auto &user_level
	{
		level_user(user_id)
	};

	const auto &required_level
	{
		empty(prop) || prop == "events"?
			level_event(type, state_key):
			level(prop)
	};

	return user_level >= required_level;
}

int64_t
ircd::m::room::power::level_user(const m::user::id &user_id)
const try
{
	int64_t ret
	{
		default_user_level
	};

	const auto closure{[&user_id, &ret]
	(const json::object &content)
	{
		const auto users_default
		{
			content.get<int64_t>("users_default", default_user_level)
		};

		const json::object &users
		{
			content.get("users")
		};

		ret = users.get<int64_t>(user_id, users_default);
	}};

	const bool has_power_levels_event
	{
		view(closure)
	};

	if(!has_power_levels_event)
	{
		if(room_creator_id && user_id == room_creator_id)
			ret = default_creator_level;

		if(room.room_id && creator(room, user_id))
			ret = default_creator_level;
	}

	return ret;
}
catch(const json::error &e)
{
	return default_user_level;
}

int64_t
ircd::m::room::power::level_event(const string_view &type)
const try
{
	int64_t ret
	{
		default_event_level
	};

	const auto closure{[&type, &ret]
	(const json::object &content)
	{
		const auto &events_default
		{
			content.get<int64_t>("events_default", default_event_level)
		};

		const json::object &events
		{
			content.get("events")
		};

		ret = events.get<int64_t>(type, events_default);
	}};

	const bool has_power_levels_event
	{
		view(closure)
	};

	return ret;
}
catch(const json::error &e)
{
	return default_event_level;
}

int64_t
ircd::m::room::power::level_event(const string_view &type,
                                  const string_view &state_key)
const try
{
	if(!defined(state_key))
		return level_event(type);

	int64_t ret
	{
		default_power_level
	};

	const auto closure{[&type, &ret]
	(const json::object &content)
	{
		const auto &state_default
		{
			content.get<int64_t>("state_default", default_power_level)
		};

		const json::object &events
		{
			content.get("events")
		};

		ret = events.get<int64_t>(type, state_default);
	}};

	const bool has_power_levels_event
	{
		view(closure)
	};

	return ret;
}
catch(const json::error &e)
{
	return default_power_level;
}

int64_t
ircd::m::room::power::level(const string_view &prop)
const try
{
	int64_t ret
	{
		default_power_level
	};

	view([&prop, &ret]
	(const json::object &content)
	{
		ret = content.at<int64_t>(prop);
	});

	return ret;
}
catch(const json::error &e)
{
	return default_power_level;
}

size_t
ircd::m::room::power::count_levels()
const
{
	size_t ret{0};
	for_each([&ret]
	(const string_view &, const int64_t &)
	{
		++ret;
	});

	return ret;
}

size_t
ircd::m::room::power::count_collections()
const
{
	size_t ret{0};
	view([&ret]
	(const json::object &content)
	{
		for(const auto &member : content)
			ret += json::type(member.second) == json::OBJECT;
	});

	return ret;
}

size_t
ircd::m::room::power::count(const string_view &prop)
const
{
	size_t ret{0};
	for_each(prop, [&ret]
	(const string_view &, const int64_t &)
	{
		++ret;
	});

	return ret;
}

bool
ircd::m::room::power::has_event(const string_view &type)
const try
{
	bool ret{false};
	view([&type, &ret]
	(const json::object &content)
	{
		const json::object &events
		{
			content.at("events")
		};

		const string_view &level
		{
			unquote(events.at(type))
		};

		ret = json::type(level) == json::NUMBER;
	});

	return ret;
}
catch(const json::error &)
{
	return false;
}

bool
ircd::m::room::power::has_user(const m::user::id &user_id)
const try
{
	bool ret{false};
	view([&user_id, &ret]
	(const json::object &content)
	{
		const json::object &users
		{
			content.at("users")
		};

		const string_view &level
		{
			unquote(users.at(user_id))
		};

		ret = json::type(level) == json::NUMBER;
	});

	return ret;
}
catch(const json::error &)
{
	return false;
}

bool
ircd::m::room::power::has_collection(const string_view &prop)
const
{
	bool ret{false};
	view([&prop, &ret]
	(const json::object &content)
	{
		const auto &value{content.get(prop)};
		if(value && json::type(value) == json::OBJECT)
			ret = true;
	});

	return ret;
}

bool
ircd::m::room::power::has_level(const string_view &prop)
const
{
	bool ret{false};
	view([&prop, &ret]
	(const json::object &content)
	{
		const auto &value(unquote(content.get(prop)));
		if(value && json::type(value) == json::NUMBER)
			ret = true;
	});

	return ret;
}

void
ircd::m::room::power::for_each(const closure &closure)
const
{
	for_each(string_view{}, closure);
}

bool
ircd::m::room::power::for_each(const closure_bool &closure)
const
{
	return for_each(string_view{}, closure);
}

void
ircd::m::room::power::for_each(const string_view &prop,
                               const closure &closure)
const
{
	for_each(prop, closure_bool{[&closure]
	(const string_view &key, const int64_t &level)
	{
		closure(key, level);
		return true;
	}});
}

bool
ircd::m::room::power::for_each(const string_view &prop,
                               const closure_bool &closure)
const
{
	bool ret{true};
	view([&prop, &closure, &ret]
	(const json::object &content)
	{
		const json::object &collection
		{
			// This little cmov gimmick sets collection to be the outer object
			// itself if no property was given, allowing us to reuse this func
			// for all iterations of key -> level mappings.
			prop? json::object{content.get(prop)} : content
		};

		const string_view _collection{collection};
		if(prop && (!_collection || json::type(_collection) != json::OBJECT))
			return;

		for(auto it(begin(collection)); it != end(collection) && ret; ++it)
		{
			const auto &member(*it);
			if(json::type(unquote(member.second)) != json::NUMBER)
				continue;

			const auto &key
			{
				unquote(member.first)
			};

			const auto &val
			{
				lex_cast<int64_t>(member.second)
			};

			ret = closure(key, val);
		}
	});

	return ret;
}

bool
ircd::m::room::power::view(const std::function<void (const json::object &)> &closure)
const
{
	if(power_event_idx)
		if(m::get(std::nothrow, power_event_idx, "content", closure))
			return true;

	closure(power_event_content);
	return !empty(power_event_content);
}
