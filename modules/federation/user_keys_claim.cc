// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2019 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

using namespace ircd;

mapi::header
IRCD_MODULE
{
	"Federation 21 :End-to-End Encryption"
};

m::resource
user_keys_claim_resource
{
	"/_matrix/federation/v1/user/keys/claim",
	{
		"federation user keys claim",
	}
};

static m::resource::response
post__user_keys_claim(client &client,
                      const m::resource::request &request);

m::resource::method
user_keys_claim__post
{
	user_keys_claim_resource, "POST", post__user_keys_claim,
	{
		user_keys_claim__post.VERIFY_ORIGIN
	}
};

m::resource::response
post__user_keys_claim(client &client,
                      const m::resource::request &request)
{
	return m::resource::response
	{
		client, http::NOT_FOUND
	};
}
