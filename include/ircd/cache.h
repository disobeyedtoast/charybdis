/**
 * Copyright (C) 2016 Charybdis development team
 * Copyright (C) 2016 Jason Volk <jason@zemos.net>
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

#pragma once
#define HAVE_IRCD_CACHE_H

#ifdef __cplusplus
namespace ircd  {
namespace cache {

constexpr auto CACHEFILELEN = 30;
constexpr auto LINKSLINELEN = HOSTLEN + HOSTLEN + REALLEN + 6;  // 2 servernames, 1 gecos, 3 spaces, ":1", '\0'

struct file;
uint flags(const file &file);
const std::string &name(const file &file);
const std::vector<std::string> &contents(const file &file);

using dict = std::map<std::string, std::shared_ptr<file>, case_insensitive_less>;

namespace motd
{
	extern char user_motd_changed[MAX_DATE_STRING];

	extern file user;
	extern file oper;

	void send_user(client::client *);
	void send_oper(client::client *);

	void cache_user();
	void cache_oper();
}

namespace help
{
	constexpr auto MAX = 100;
	constexpr auto USER = 0x01;
	constexpr auto OPER = 0x02;

	extern dict user;
	extern dict oper;

	void load();
}

void init();

}       // namespace cache
}       // namespace ircd
#endif  // __cplusplus
