#include <qstring.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kwallet.h>
#include <stdio.h>

#include <config.h>

#define FOLDER	"roger"

static KApplication *app = NULL;
static KWallet::Wallet *wallet = NULL;

/**
 * \brief Initialize wallet
 */
extern "C" int kwallet4_init(void)
{
	QString local_wallet;

	if (app == NULL) {
		KAboutData aboutData("roger", 0, ki18n("roger"), "");
		KCmdLineArgs::init(&aboutData);
		app = new KApplication(false);
	}
	if (wallet == NULL) {
		local_wallet = KWallet::Wallet::LocalWallet();
		wallet = KWallet::Wallet::openWallet(local_wallet, 0);
	}

	return app && wallet;
}

extern "C" int kwallet4_get_password(const char *name, char **password)
{
	QString entry, pwd;
	int error = -1;

	if (!wallet) {
		return -2;
	}

	if (!wallet->hasFolder(FOLDER)) {
		error = -3;
		goto out;
	}

	if (!wallet->setFolder(FOLDER)) {
		error = -4;
		goto out;;
	}

	entry = QString::fromUtf8(name);

	if (!wallet->hasEntry(entry)) {
		error = -5;
		goto out;
	}

	pwd = "";

	if (wallet->readPassword(entry, pwd) == 0) {
		*password = strdup((char *) pwd.toUtf8().data());
		error = 0;
	}

out:

	return error;
}

extern "C" int kwallet4_store_password(const char *name, const char *password)
{
	QString entry, pwd;
	int error = -1;

	if (!wallet) {
		return -2;
	}

	if (!wallet->hasFolder(FOLDER)) {
		wallet->createFolder(FOLDER);
	}

	if (!wallet->setFolder(FOLDER)) {
		error = -4;
		goto out;;
	}

	entry = QString::fromUtf8(name);
	pwd = QString::fromUtf8(password);

	if (wallet->writePassword(entry, pwd)) {
		error = -5;
	} else {
		error = 0;
	}

out:

	return error;
}
