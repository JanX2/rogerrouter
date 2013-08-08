#include <gtk/gtk.h>
#include <main.h>
#include <windows.h>
#include <stdio.h>

typedef int (CALLBACK *LPFNROGERMAIN)(HINSTANCE, int, char **);
typedef BOOL (WINAPI* LPFNSETPROCESSDEPPOLICY)(DWORD);
typedef BOOL (WINAPI* LPFNATTACHCONSOLE)(DWORD);

static wchar_t *win_lcid_to_posix(LCID lcid) {
	wchar_t *posix = NULL;
	int lang_id = PRIMARYLANGID(lcid);

	switch (lang_id) {
		case LANG_GERMAN:
			posix = L"de";
			break;
		case LANG_ENGLISH:
			posix = L"en";
			break;
		case LANG_DUTCH:
			posix = L"nl";
			break;
	}

	return posix;
}

static const wchar_t *win_get_locale() {
	LCID lcid;
	const wchar_t *locale;

	lcid = GetUserDefaultLCID();
	if ((locale = win_lcid_to_posix(lcid))) {
		return locale;
	}

	return L"en";
}

static const wchar_t *get_win32_error_message(DWORD err) {
	static wchar_t err_msg[512];

	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR) &err_msg, sizeof(err_msg) / sizeof(wchar_t), NULL );

	return err_msg;
}

int _stdcall WinMain(struct HINSTANCE__ *hInstance, struct HINSTANCE__ *hPrevInstance, char *lpszCmdLine, int nCmdShow ) {
	LPFNROGERMAIN roger_main = NULL;
	gchar *roger_dir = NULL;
	int roger_argc = 0;
	char **roger_argv = NULL;
	const wchar_t *locale;
	wchar_t envstr[25];
	HMODULE hmod;
	BOOL debug = FALSE, help = FALSE;
	int i;

	/* If debug or help or version flag used, create console for output */
	for (i = 1; i < __argc; i++) {
		if (strlen(__argv[i]) > 1 && __argv[i][0] == '-') {
			/* check if we're looking at -- or - option */
			if (__argv[i][1] == '-') {
				if (strstr(__argv[i], "--debug") == __argv[i])
					debug = TRUE;
				else if (strstr(__argv[i], "--help") == __argv[i])
					help = TRUE;
			} else {
				if (strchr(__argv[i], 'd'))
					debug = TRUE;
				if (strchr(__argv[i], 'h'))
					help = TRUE;
			}
		}
	}

	/* Permanently enable DEP if the OS supports it */
	if ((hmod = GetModuleHandleW(L"kernel32.dll"))) {
		LPFNSETPROCESSDEPPOLICY MySetProcessDEPPolicy =
			(LPFNSETPROCESSDEPPOLICY)
			GetProcAddress(hmod, "SetProcessDEPPolicy");
		if (MySetProcessDEPPolicy)
			MySetProcessDEPPolicy(1); //PROCESS_DEP_ENABLE
	}

	if (debug || help) {
		/* If stdout hasn't been redirected to a file, alloc a console
		 *  (_istty() doesn't work for stuff using the GUI subsystem) */
		if (_fileno(stdout) == -1 || _fileno(stdout) == -2) {
			LPFNATTACHCONSOLE MyAttachConsole = NULL;
			if (hmod)
				MyAttachConsole =
					(LPFNATTACHCONSOLE)
					GetProcAddress(hmod, "AttachConsole");
			if ((MyAttachConsole && MyAttachConsole(ATTACH_PARENT_PROCESS))
					|| AllocConsole()) {
				freopen("CONOUT$", "w", stdout);
				freopen("CONOUT$", "w", stderr);
			}
		}
	}

	locale = win_get_locale();
	_snwprintf(envstr, sizeof(envstr) / sizeof(wchar_t), L"LANG=%s", locale);
	_wputenv(envstr);

	roger_dir = g_win32_get_package_installation_directory_of_module(NULL);
	strcpy(roger_dir, "libroger_core.dll");

	if ((hmod = LoadLibrary(roger_dir))) {
		roger_main = (LPFNROGERMAIN) GetProcAddress(hmod, "roger_main");
	} else {
		printf("Could not load %s\n", roger_dir);
	}

	if (!roger_main) {
		DWORD dw = GetLastError();
		BOOL mod_not_found = (dw == ERROR_MOD_NOT_FOUND || dw == ERROR_DLL_NOT_FOUND );
		const wchar_t *err_msg = get_win32_error_message(dw);

		printf("Could not load roger_main from %s\n", roger_dir);
		wprintf( L"Error (%u) %s%s%s\n", dw, err_msg, mod_not_found ? L"\n" : L"", mod_not_found ? L"This probably means that GTK+ can't be found" : L"");
		return 0;
	}

	roger_argc = __argc;
	roger_argv = __argv;

	return roger_main(hInstance, roger_argc, roger_argv);
}
