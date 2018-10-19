#include <config.h>
#include <glib.h>
#include "mono/utils/mono-threads-api.h"
#include "mono/utils/atomic.h"
#include "metadata/loader-internals.h"

#include "mono-native-platform.h"

extern MonoNativePlatformType mono_native_platform_type;

int32_t
mono_native_get_platform_type (void)
{
	return mono_native_platform_type;
}

static void
ves_icall_Martin_Test (void)
{
	fprintf (stderr, "MARTIN TEST!\n");
}

void
mono_native_initialize (void)
{
	volatile static gboolean module_initialized = FALSE;
	if (mono_atomic_cas_i32 (&module_initialized, TRUE, FALSE) != FALSE)
		return;

	mono_add_internal_call ("Mono.MonoNativePlatform::MartinTest", ves_icall_Martin_Test);
}
