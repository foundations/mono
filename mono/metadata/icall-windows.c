/**
 * \file
 * Windows icall support.
 *
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
#include <config.h>
#include <glib.h>

#if defined(HOST_WIN32)
#include <winsock2.h>
#include <windows.h>
#include "mono/metadata/icall-windows-internals.h"
#include "mono/metadata/w32subset.h"
#if HAVE_API_SUPPORT_WIN32_SH_GET_FOLDER_PATH
#include <shlobj.h>
#endif

void
mono_icall_make_platform_path (gchar *path)
{
	for (size_t i = strlen (path); i > 0; i--)
		if (path [i-1] == '\\')
			path [i-1] = '/';
}

const gchar *
mono_icall_get_file_path_prefix (const gchar *path)
{
	if (*path == '/' && *(path + 1) == '/') {
		return "file:";
	} else {
		return "file:///";
	}
}

gpointer
mono_icall_module_get_hinstance (MonoReflectionModuleHandle module)
{
	MonoImage *image = MONO_HANDLE_GETVAL (module, image);
	if (image && image->is_module_handle)
		return image->raw_data;

	return (gpointer) (-1);
}

#if HAVE_API_SUPPORT_WIN32_GET_COMPUTER_NAME
// Support older UWP SDK?
WINBASEAPI
BOOL
WINAPI
GetComputerNameW (
	PWSTR buffer,
	PDWORD size
	);

MonoStringHandle
mono_icall_get_machine_name (MonoError *error)
{
	gunichar2 buf [MAX_COMPUTERNAME_LENGTH + 1];
	DWORD len = G_N_ELEMENTS (buf);

	if (GetComputerNameW (buf, &len))
		return mono_string_new_utf16_handle (mono_domain_get (), buf, len, error);
	return MONO_HANDLE_NEW (MonoString, NULL);
}
#endif

int
mono_icall_get_platform (void)
{
	/* Win32NT */
	return 2;
}

MonoStringHandle
mono_icall_get_new_line (MonoError *error)
{
	error_init (error);
	return mono_string_new_handle (mono_domain_get (), "\r\n", error);
}

MonoBoolean
mono_icall_is_64bit_os (void)
{
#if SIZEOF_VOID_P == 8
	return TRUE;
#else
	gboolean isWow64Process = FALSE;
	if (IsWow64Process (GetCurrentProcess (), &isWow64Process)) {
		return (MonoBoolean)isWow64Process;
	}
	return FALSE;
#endif
}

MonoArray *
mono_icall_get_environment_variable_names (MonoError *error)
{
	MonoArray *names;
	MonoDomain *domain;
	MonoString *str;
	WCHAR* env_strings;
	WCHAR* env_string;
	WCHAR* equal_str;
	int n = 0;

	error_init (error);
	env_strings = GetEnvironmentStrings();

	if (env_strings) {
		env_string = env_strings;
		while (*env_string != '\0') {
		/* weird case that MS seems to skip */
			if (*env_string != '=')
				n++;
			while (*env_string != '\0')
				env_string++;
			env_string++;
		}
	}

	domain = mono_domain_get ();
	names = mono_array_new_checked (domain, mono_defaults.string_class, n, error);
	return_val_if_nok (error, NULL);

	if (env_strings) {
		n = 0;
		env_string = env_strings;
		while (*env_string != '\0') {
			/* weird case that MS seems to skip */
			if (*env_string != '=') {
				equal_str = wcschr(env_string, '=');
				g_assert(equal_str);
				str = mono_string_new_utf16_checked (domain, env_string, (gint32)(equal_str - env_string), error);
				goto_if_nok (error, cleanup);

				mono_array_setref_internal (names, n, str);
				n++;
			}
			while (*env_string != '\0')
				env_string++;
			env_string++;
		}

	}

cleanup:
	if (env_strings)
		FreeEnvironmentStrings (env_strings);
	if (!is_ok (error))
		return NULL;
	return names;
}

void
mono_icall_set_environment_variable (MonoString *name, MonoString *value)
{
	gunichar2 *utf16_name, *utf16_value;

	utf16_name = name ? mono_string_chars_internal (name) : NULL;
	if ((value == NULL) || (mono_string_length_internal (value) == 0) || (mono_string_chars_internal (value)[0] == 0)) {
		SetEnvironmentVariable (utf16_name, NULL);
		return;
	}

	utf16_value = mono_string_chars_internal (value);

	SetEnvironmentVariable (utf16_name, utf16_value);
}

#if HAVE_API_SUPPORT_WIN32_SH_GET_FOLDER_PATH
MonoStringHandle
mono_icall_get_windows_folder_path (int folder, MonoError *error)
{
	error_init (error);
	#ifndef CSIDL_FLAG_CREATE
		#define CSIDL_FLAG_CREATE	0x8000
	#endif

	WCHAR path [MAX_PATH];
	/* Create directory if no existing */
	if (SUCCEEDED (SHGetFolderPathW (NULL, folder | CSIDL_FLAG_CREATE, NULL, 0, path))) {
		int len = 0;
		while (path [len])
			++ len;
		return mono_string_new_utf16_handle (mono_domain_get (), path, len, error);
	}
	return mono_string_new_handle (mono_domain_get (), "", error);
}
#endif

#if HAVE_API_SUPPORT_WIN32_SEND_MESSAGE_TIMEOUT
MonoBoolean
mono_icall_broadcast_setting_change (MonoError *error)
{
	error_init (error);
	SendMessageTimeout (HWND_BROADCAST, WM_SETTINGCHANGE, (WPARAM)NULL, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 2000, 0);
	return TRUE;
}
#endif

#if HAVE_API_SUPPORT_WIN32_WAIT_FOR_INPUT_IDLE
gint32
mono_icall_wait_for_input_idle (gpointer handle, gint32 milliseconds)
{
	return WaitForInputIdle (handle, milliseconds);
}
#endif

void
mono_icall_write_windows_debug_string (const gunichar2 *message)
{
	OutputDebugString (message);
}

#endif /* HOST_WIN32 */
