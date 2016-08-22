/*
 * Server name extban type: bans all users using a certain server
 * -- jilles
 */

namespace mode = ircd::chan::mode;
namespace ext = mode::ext;
using namespace ircd;

static const char extb_desc[] = "Server ($s) extban type";

static int _modinit(void);
static void _moddeinit(void);
static int eb_server(const char *data, client::client *client_p, chan::chan *chptr, mode::type);

DECLARE_MODULE_AV2(extb_server, _modinit, _moddeinit, NULL, NULL, NULL, NULL, NULL, extb_desc);

static int
_modinit(void)
{
	ext::table['s'] = eb_server;

	return 0;
}

static void
_moddeinit(void)
{
	ext::table['s'] = NULL;
}

static int
eb_server(const char *data, client::client *client_p, chan::chan *chptr, mode::type type)
{
	using namespace ext;
	using namespace mode;

	/* This type is not safe for exceptions */
	if (type == EXCEPTION || type == INVEX)
		return INVALID;

	if (data == NULL)
		return INVALID;

	return match(data, me.name)? MATCH : NOMATCH;
}
