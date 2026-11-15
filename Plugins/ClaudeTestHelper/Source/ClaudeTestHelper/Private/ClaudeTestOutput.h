#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING
/** Where the test console commands (Input.*, Anim.Freeze, Reader.Read) print, so their results are seen wherever they were typed. */
namespace ClaudeTestOutput
{
	/** One line of a command's output, into the console and the Output Log. Warnings also show on screen, so a mistyped command isn't silent. */
	CLAUDETESTHELPER_API void Line(FOutputDevice& Ar, const FString& Message, bool bWarning = false);

	/** A command's result: into the console and the Output Log, and on screen for a few seconds. Showing the same key again replaces it. */
	CLAUDETESTHELPER_API void Result(FOutputDevice& Ar, uint64 ScreenKey, const FString& Title, const TArray<FString>& Lines = TArray<FString>());
}
#endif
