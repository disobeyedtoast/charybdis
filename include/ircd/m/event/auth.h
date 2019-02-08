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
#define HAVE_IRCD_M_EVENT_AUTH_H

namespace ircd::m
{
	bool is_power_event(const event &);
}

struct ircd::m::event::auth
{
	using closure_bool = event::closure_idx_bool;

	event::idx idx;

  public:
	bool for_each(const closure_bool &) const;
	bool has(const event::idx &) const noexcept;
	size_t count() const noexcept;

	auth(const event::idx &idx) noexcept;
};

inline
ircd::m::event::auth::auth(const event::idx &idx)
noexcept
:idx{idx}
{
	assert(idx);
}