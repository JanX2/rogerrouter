/**
 * Roger Router
 * Copyright (c) 2012-2014 Jan-Michael Brummer
 *
 * This file is part of Roger Router.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \TODO:
 * Remove key with removeEntry()
 */

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

/**
 * \brief Get password
 * \param name name of password
 * \param password pointer to store password to
 * \return 0 on success, negative values on error
 */
extern "C" int kwallet4_get_password(const char *name, char **password)
{
	QString entry, pwd;
	int error = -1;

	/* Sanity checks */
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

/**
 * \brief Store password in wallet
 * \param name name of password
 * \param password password text
 * \return 0 on success, negative values on error
 */
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
