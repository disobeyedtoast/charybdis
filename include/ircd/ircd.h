// Matrix Construct
//
// Copyright (C) Matrix Construct Developers, Authors & Contributors
// Copyright (C) 2016-2018 Jason Volk <jason@zemos.net>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice is present in all copies. The
// full license for this software is available in the LICENSE file.

#define HAVE_IRCD_IRCD_H

// Generated by ./configure
#include "config.h"
#include "stdinc.h"

namespace ircd
{
	enum class runlevel :int;

	extern const enum runlevel &runlevel;

	extern bool debugmode;             ///< Toggle; available only ifdef RB_DEBUG
	extern bool nolisten;              ///< Init option to not bind listener socks.
	extern bool noautomod;             ///< Option to not load modules on init.
	extern bool checkdb;               ///< Perform checks on database opens
	extern bool pitrecdb;              ///< Allow Point-In-Time Recovery if DB corrupt.
	extern bool nojs;                  ///< Option to not init js subsystem.
	extern bool nodirect;              ///< Option to not use direct IO (O_DIRECT).
	extern bool noaio;                 ///< Option to not use AIO even if available.
}

#include "string_view.h"
#include "vector_view.h"
#include "byte_view.h"
#include "buffer/buffer.h"
#include "allocator.h"
#include "util/util.h"
#include "exception.h"
#include "fpe.h"
#include "demangle.h"
#include "localee.h"
#include "date.h"
#include "logger.h"
#include "info.h"
#include "nacl.h"
#include "rand.h"
#include "crh.h"
#include "ed25519.h"
#include "color.h"
#include "lex_cast.h"
#include "base.h"
#include "stringops.h"
#include "tokens.h"
#include "iov.h"
#include "parse.h"
#include "rfc1459.h"
#include "json/json.h"
//#include "cbor/cbor.h"
#include "openssl.h"
#include "http.h"
#include "fmt.h"
#include "magics.h"
#include "conf.h"
#include "fs/fs.h"
#include "ios.h"
#include "ctx/ctx.h"
#include "db/db.h"
#include "js.h"
#include "mods/mods.h"
#include "rfc3986.h"
#include "rfc1035.h"
#include "net/net.h"
#include "server/server.h"
#include "resource.h"
#include "client.h"

/// \brief Internet Relay Chat daemon. This is the principal namespace for IRCd.
///
namespace ircd
{
	struct init;
	struct runlevel_changed;

	string_view reflect(const enum runlevel &);

	seconds uptime();

	void init(boost::asio::io_context &ios, const string_view &origin, const string_view &hostname);
	bool quit() noexcept;
}

/// An instance of this class registers itself to be called back when
/// the ircd::runlevel has changed.
///
/// Note: Its destructor will access a list in libircd; after a callback
/// for a HALT do not unload libircd.so until destructing this object.
///
/// A static ctx::dock is also available for contexts to wait for a runlevel
/// change notification.
///
struct ircd::runlevel_changed
:instance_list<ircd::runlevel_changed>
,std::function<void (const enum runlevel &)>
{
	using handler = std::function<void (const enum runlevel &)>;

	static ctx::dock dock;

	runlevel_changed(handler function);
	~runlevel_changed() noexcept;
};

/// The runlevel allows all observers to know the coarse state of IRCd and to
/// react accordingly. This can be used by the embedder of libircd to know
/// when it's safe to use or delete libircd resources. It is also used
/// similarly by the library and its modules.
///
/// Primary modes:
///
/// * HALT is the off mode. Nothing is/will be running in libircd until
/// an invocation of ircd::init();
///
/// * RUN is the service mode. Full client and application functionality
/// exists in this mode. Leaving the RUN mode is done with ircd::quit();
///
/// - Transitional modes: Modes which are working towards the next mode.
/// - Interventional modes:  Modes which are *not* working towards the next
/// mode and may require some user action to continue.
///
enum class ircd::runlevel
:int
{
	HALT     = 0,    ///< [inter] IRCd Powered off.
	READY    = 1,    ///< [inter] Ready for user to run ios event loop.
	START    = 2,    ///< [trans] Starting up subsystems for service.
	RUN      = 3,    ///< [inter] IRCd in service.
	QUIT     = 4,    ///< [trans] Clean shutdown in progress
	FAULT    = -1,   ///< [trans] QUIT with exception (dirty shutdown)
};
