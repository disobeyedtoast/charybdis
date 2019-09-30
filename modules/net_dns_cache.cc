// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2019 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

#include "net_dns.h"

decltype(ircd::net::dns::cache::error_ttl)
ircd::net::dns::cache::error_ttl
{
	{ "name",     "ircd.net.dns.cache.error_ttl" },
	{ "default",  1200L                          },
};

decltype(ircd::net::dns::cache::nxdomain_ttl)
ircd::net::dns::cache::nxdomain_ttl
{
	{ "name",     "ircd.net.dns.cache.nxdomain_ttl" },
	{ "default",  86400L                            },
};

decltype(ircd::net::dns::cache::min_ttl)
ircd::net::dns::cache::min_ttl
{
	{ "name",     "ircd.net.dns.cache.min_ttl" },
	{ "default",  28800L                       },
};

decltype(ircd::net::dns::cache::room_id)
ircd::net::dns::cache::room_id
{
	"dns", my_host()
};

decltype(ircd::net::dns::cache::hook)
ircd::net::dns::cache::hook
{
    handle,
    {
        { "_site",       "vm.effect"          },
        { "room_id",     string_view{room_id} },
    }
};

decltype(ircd::net::dns::cache::waiting)
ircd::net::dns::cache::waiting;

decltype(ircd::net::dns::cache::mutex)
ircd::net::dns::cache::mutex;

decltype(ircd::net::dns::cache::dock)
ircd::net::dns::cache::dock;

void
ircd::net::dns::cache::init()
{
}

void
ircd::net::dns::cache::fini()
{
	if(!waiting.empty())
		log::warning
		{
			log, "Waiting for %zu unfinished cache operations.",
			waiting.size(),
		};

	dock.wait([]
	{
		return waiting.empty();
	});
}

bool
IRCD_MODULE_EXPORT
ircd::net::dns::cache::put(const hostport &hp,
                           const opts &opts,
                           const uint &code,
                           const string_view &msg)
{
	char type_buf[64];
	const string_view type
	{
		make_type(type_buf, opts.qtype)
	};

	char state_key_buf[rfc1035::NAME_BUFSIZE * 2];
	const string_view &state_key
	{
		opts.qtype == 33?
			make_SRV_key(state_key_buf, host(hp), opts):
			host(hp)
	};

	return put(type, state_key, code, msg);
}

bool
IRCD_MODULE_EXPORT
ircd::net::dns::cache::put(const hostport &hp,
                           const opts &opts,
                           const records &rrs)
{
	const auto &type_code
	{
		!rrs.empty()? rrs.at(0)->type : opts.qtype
	};

	char type_buf[48];
	const string_view type
	{
		make_type(type_buf, type_code)
	};

	char state_key_buf[rfc1035::NAME_BUFSIZE * 2];
	const string_view &state_key
	{
		opts.qtype == 33?
			make_SRV_key(state_key_buf, host(hp), opts):
			host(hp)
	};

	return put(type, state_key, rrs);
}

bool
ircd::net::dns::cache::put(const string_view &type,
                           const string_view &state_key,
                           const uint &code,
                           const string_view &msg)
try
{
	char content_buf[1024];
	json::stack out{content_buf};
	json::stack::object content{out};
	json::stack::array array
	{
		content, ""
	};

	json::stack::object rr0
	{
		array
	};

	json::stack::member
	{
		rr0, "errcode", lex_cast(code)
	};

	json::stack::member
	{
		rr0, "error", msg
	};

	json::stack::member
	{
		rr0, "ttl", json::value
		{
			code == 3?
				long(seconds(nxdomain_ttl).count()):
				long(seconds(error_ttl).count())
		}
	};

	rr0.~object();
	array.~array();
	content.~object();
	send(room_id, m::me, type, state_key, json::object(out.completed()));
	return true;
}
catch(const http::error &e)
{
	const ctx::exception_handler eh;
	log::error
	{
		log, "cache put (%s, %s) code:%u (%s) :%s %s",
		type,
		state_key,
		code,
		msg,
		e.what(),
		e.content,
	};

	const json::value error_value
	{
		json::object{e.content}
	};

	const json::value error_records{&error_value, 1};
	const json::strung error{error_records};
	call_waiters(type, state_key, error);
	return false;
}
catch(const std::exception &e)
{
	const ctx::exception_handler eh;
	log::error
	{
		log, "cache put (%s, %s) code:%u (%s) :%s",
		type,
		state_key,
		code,
		msg,
		e.what()
	};

	const json::members error_object
	{
		{ "error", e.what() },
	};

	const json::value error_value{error_object};
	const json::value error_records{&error_value, 1};
	const json::strung error{error_records};
	call_waiters(type, state_key, error);
	return false;
}

bool
ircd::net::dns::cache::put(const string_view &type,
                           const string_view &state_key,
                           const records &rrs)
try
{
	const unique_buffer<mutable_buffer> buf
	{
		8_KiB
	};

	json::stack out{buf};
	json::stack::object content{out};
	json::stack::array array
	{
		content, ""
	};

	if(rrs.empty())
	{
		// Add one object to the array with nothing except a ttl indicating no
		// records (and no error) so we can cache that for the ttl. We use the
		// nxdomain ttl for this value.
		json::stack::object rr0{array};
		json::stack::member
		{
			rr0, "ttl", json::value
			{
				long(seconds(nxdomain_ttl).count())
			}
		};
	}
	else for(const auto &record : rrs)
	{
		switch(record->type)
		{
			case 1: // A
			{
				json::stack::object object{array};
				dynamic_cast<const rfc1035::record::A *>(record)->append(object);
				continue;
			}

			case 5: // CNAME
			{
				json::stack::object object{array};
				dynamic_cast<const rfc1035::record::CNAME *>(record)->append(object);
				continue;
			}

			case 28: // AAAA
			{
				json::stack::object object{array};
				dynamic_cast<const rfc1035::record::AAAA *>(record)->append(object);
				continue;
			}

			case 33: // SRV
			{
				json::stack::object object{array};
				dynamic_cast<const rfc1035::record::SRV *>(record)->append(object);
				continue;
			}
		}
	}

	// When the array has a zero value count we didn't know how to cache
	// any of these records; don't send anything to the cache room.
	if(!array.vc)
		return false;

	array.~array();
	content.~object();
	send(room_id, m::me, type, state_key, json::object{out.completed()});
	return true;
}
catch(const http::error &e)
{
	const ctx::exception_handler eh;
	log::error
	{
		log, "cache put (%s, %s) rrs:%zu :%s %s",
		type,
		state_key,
		rrs.size(),
		e.what(),
		e.content,
	};

	const json::value error_value
	{
		json::object{e.content}
	};

	const json::value error_records{&error_value, 1};
	const json::strung error{error_records};
	call_waiters(type, state_key, error);
	return false;
}
catch(const std::exception &e)
{
	const ctx::exception_handler eh;
	log::error
	{
		log, "cache put (%s, %s) rrs:%zu :%s",
		type,
		state_key,
		rrs.size(),
		e.what(),
	};

	const json::members error_object
	{
		{ "error", e.what() },
	};

	const json::value error_value{error_object};
	const json::value error_records{&error_value, 1};
	const json::strung error{error_records};
	call_waiters(type, state_key, error);
	return false;
}

bool
IRCD_MODULE_EXPORT
ircd::net::dns::cache::get(const hostport &hp,
                           const opts &opts,
                           const callback &closure)
{
	char type_buf[48];
	const string_view type
	{
		make_type(type_buf, opts.qtype)
	};

	char state_key_buf[rfc1035::NAME_BUFSIZE * 2];
	const string_view &state_key
	{
		opts.qtype == 33?
			make_SRV_key(state_key_buf, host(hp), opts):
			host(hp)
	};

	const m::room::state state
	{
		room_id
	};

	const m::event::idx &event_idx
	{
		state.get(std::nothrow, type, state_key)
	};

	if(!event_idx)
		return false;

	time_t origin_server_ts;
	if(!m::get<time_t>(event_idx, "origin_server_ts", origin_server_ts))
		return false;

	bool ret{false};
	const time_t ts{origin_server_ts / 1000L};
	m::get(std::nothrow, event_idx, "content", [&hp, &closure, &ret, &ts]
	(const json::object &content)
	{
		const json::array &rrs
		{
			content.get("")
		};

		// If all records are expired then skip; otherwise since this closure
		// expects a single array we reveal both expired and valid records.
		ret = !std::all_of(begin(rrs), end(rrs), [&ts]
		(const json::object &rr)
		{
			return expired(rr, ts);
		});

		if(ret && closure)
			closure(hp, rrs);
	});

	return ret;
}

bool
IRCD_MODULE_EXPORT
ircd::net::dns::cache::for_each(const hostport &hp,
                                const opts &opts,
                                const closure &closure)
{
	char type_buf[48];
	const string_view type
	{
		make_type(type_buf, opts.qtype)
	};

	char state_key_buf[rfc1035::NAME_BUFSIZE * 2];
	const string_view &state_key
	{
		opts.qtype == 33?
			make_SRV_key(state_key_buf, host(hp), opts):
			host(hp)
	};

	const m::room::state state
	{
		room_id
	};

	const m::event::idx &event_idx
	{
		state.get(std::nothrow, type, state_key)
	};

	if(!event_idx)
		return false;

	time_t origin_server_ts;
	if(!m::get<time_t>(event_idx, "origin_server_ts", origin_server_ts))
		return false;

	bool ret{true};
	const time_t ts{origin_server_ts / 1000L};
	m::get(std::nothrow, event_idx, "content", [&state_key, &closure, &ret, &ts]
	(const json::object &content)
	{
		for(const json::object &rr : json::array(content.get("")))
		{
			if(expired(rr, ts))
				continue;

			if(!(ret = closure(state_key, rr)))
				break;
		}
	});

	return ret;
}

bool
IRCD_MODULE_EXPORT
ircd::net::dns::cache::for_each(const string_view &type,
                                const closure &closure)
{
	char type_buf[48];
	const string_view full_type
	{
		make_type(type_buf, type)
	};

	const m::room::state state
	{
		room_id
	};

	return state.for_each(full_type, [&closure]
	(const string_view &, const string_view &state_key, const m::event::idx &event_idx)
	{
		time_t origin_server_ts;
		if(!m::get<time_t>(event_idx, "origin_server_ts", origin_server_ts))
			return true;

		bool ret{true};
		const time_t ts{origin_server_ts / 1000L};
		m::get(std::nothrow, event_idx, "content", [&state_key, &closure, &ret, &ts]
		(const json::object &content)
		{
			for(const json::object &rr : json::array(content.get("")))
			{
				if(expired(rr, ts))
					continue;

				if(!(ret = closure(state_key, rr)))
					break;
			}
		});

		return ret;
	});
}

void
ircd::net::dns::cache::handle(const m::event &event,
                              m::vm::eval &eval)
try
{
	const string_view &type
	{
		json::get<"type"_>(event)
	};

	if(!startswith(type, "ircd.dns.rrs."))
		return;

	const string_view &state_key
	{
		json::get<"state_key"_>(event)
	};

	const json::array &rrs
	{
		json::get<"content"_>(event).get("")
	};

	call_waiters(type, state_key, rrs);
}
catch(const std::exception &e)
{
	log::critical
	{
		log, "handle_cached() :%s", e.what()
	};
}

/// Note complications due to reentrance and other factors:
/// - This function is invoked from several different places on both the
/// timeout and receive contexts, in addition to any evaluator context.
/// - This function calls back to users making DNS queries, and they may
/// conduct another query in their callback frame -- mid-loop in this
/// function.
size_t
ircd::net::dns::cache::call_waiters(const string_view &type,
                                    const string_view &state_key,
                                    const json::array &rrs)
{
	const ctx::uninterruptible::nothrow ui;
	size_t ret(0), last; do
	{
		const std::lock_guard lock
		{
			mutex
		};

		auto it(begin(waiting));
		for(last = ret; it != end(waiting); ++it)
			if(call_waiter(type, state_key, rrs, *it))
			{
				it = waiting.erase(it);
				++ret;
				break;
			}
	}
	while(last < ret);

	if(ret)
		dock.notify_all();

	return ret;
}

bool
ircd::net::dns::cache::call_waiter(const string_view &type,
                                   const string_view &state_key,
                                   const json::array &rrs,
                                   waiter &waiter)
try
{
	if(state_key != waiter.key)
		return false;

	if(lstrip(type, "ircd.dns.rrs.") != rfc1035::rqtype.at(waiter.opts.qtype))
		return false;

	const hostport &target
	{
		waiter.opts.qtype == 33?
			unmake_SRV_key(waiter.key):
			waiter.key,

		waiter.port
	};

	assert(waiter.callback);
	waiter.callback(target, rrs);
	return true;
}
catch(const std::exception &e)
{
	log::critical
	{
		log, "callback:%p %s,%s :%s",
		(const void *)&waiter,
		type,
		state_key,
		e.what(),
	};

	return true;
}

//
// cache::waiter
//

bool
ircd::net::dns::cache::operator==(const waiter &a, const waiter &b)
{
	return a.opts.qtype == b.opts.qtype &&
	       a.key && b.key &&
	       a.key == b.key;
}

bool
ircd::net::dns::cache::operator!=(const waiter &a, const waiter &b)
{
	return !operator==(a, b);
}

//
// cache::waiter::waiter
//

ircd::net::dns::cache::waiter::waiter(const hostport &hp,
                                      const dns::opts &opts,
                                      dns::callback &&callback)
:callback
{
	std::move(callback)
}
,opts
{
	opts
}
,port
{
	net::port(hp)
}
,key
{
	opts.qtype == 33?
		make_SRV_key(keybuf, hp, opts):
		strlcpy(keybuf, host(hp))
}
{
	this->opts.srv = {};
	this->opts.proto = {};
	assert(this->opts.qtype);
}

//
// cache room creation
//

namespace ircd::net::dns {
namespace [[gnu::visibility("hidden")]] cache
{
	static void create_room();
	extern m::hookfn<m::vm::eval &> create_room_hook;
}}

decltype(ircd::net::dns::cache::create_room_hook)
ircd::net::dns::cache::create_room_hook
{
	{
		{ "_site",       "vm.effect"      },
		{ "room_id",     "!ircd"          },
		{ "type",        "m.room.create"  },
	},
	[](const m::event &, m::vm::eval &)
	{
		create_room();
	}
};

void
ircd::net::dns::cache::create_room()
try
{
	const m::room room
	{
		m::create(room_id, m::me, "internal")
	};

	log::debug
	{
		m::log, "Created '%s' for the DNS cache module.",
		string_view{room.room_id}
	};
}
catch(const std::exception &e)
{
	log::critical
	{
		m::log, "Creating the '%s' room failed :%s",
		string_view{room_id},
		e.what()
	};
}