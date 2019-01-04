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
#define HAVE_IRCD_UTIL_SCOPE_COUNT_H

namespace ircd::util
{
	struct scope_count;
};

/// A simple boiler-plate for incrementing a counter when constructed and
/// decrementing it to its previous value when destructed. This takes a runtime
/// reference to that counter.
struct ircd::util::scope_count
{
	ulong *count {nullptr};

	scope_count(ulong &count);
	scope_count(const scope_count &) = delete;
	scope_count(scope_count &&) noexcept = delete;
	~scope_count() noexcept;
};

inline
ircd::util::scope_count::scope_count(ulong &count)
:count{&count}
{
	++count;
}

inline
ircd::util::scope_count::~scope_count()
noexcept
{
	assert(count);
	--(*count);
}