#pragma once

#include "CoreMinimal.h"

/**
 * Anim.Freeze holds every character still for screenshot tests; it also holds looping UI motion (a menu's selection pulse, a still
 * background's drift) at its start, so a picture doesn't depend on the moment it was taken. Not in shipping builds.
 * Game code shouldn't depend on this test plugin: read the console variable Test.LoopsFrozen (1 while frozen) by name instead.
 */
namespace ClaudeTestFreeze
{
#if !UE_BUILD_SHIPPING
	CLAUDETESTHELPER_API bool AreLoopsFrozen();
	CLAUDETESTHELPER_API void SetLoopsFrozen(bool bFrozen);
#else
	inline bool AreLoopsFrozen() { return false; }
	inline void SetLoopsFrozen(bool) {}
#endif
}
