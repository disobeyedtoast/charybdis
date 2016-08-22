/*
 * Realname extban type: bans all users with matching gecos
 * -- jilles
 */

namespace mode = ircd::chan::mode;
namespace ext = mode::ext;
using namespace ircd;

static const char extb_desc[] = "Realname/GECOS ($r) extban type";

static int _modinit(void);
static void _moddeinit(void);
static int eb_realname(const char *data, client::client *client_p, chan::chan *chptr, mode::type);

DECLARE_MODULE_AV2(extb_realname, _modinit, _moddeinit, NULL, NULL, NULL, NULL, NULL, extb_desc);

static int
_modinit(void)
{
	ext::table['r'] = eb_realname;

	return 0;
}

static void
_moddeinit(void)
{
	ext::table['r'] = NULL;
}

static int
eb_realname(const char *data, client::client *client_p, chan::chan *chptr, mode::type type)
{
	using namespace ext;
	using namespace mode;

	/* This type is not safe for exceptions */
	if (type == EXCEPTION || type == INVEX)
		return INVALID;

	if (data == NULL)
		return INVALID;

	return match(data, client_p->info)? MATCH : NOMATCH;
}
