#include "ClaudeTestOutput.h"

#if !UE_BUILD_SHIPPING
#include "Engine/Engine.h"

namespace ClaudeTestOutput
{
	void Line(FOutputDevice& Ar, const FString& Message, bool bWarning)
	{
		Ar.Log(bWarning ? ELogVerbosity::Warning : ELogVerbosity::Display, *Message);
		if (bWarning && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 6.f, FColor(255, 170, 80), Message);
		}
	}

	void Result(FOutputDevice& Ar, uint64 ScreenKey, const FString& Title, const TArray<FString>& Lines)
	{
		Ar.Log(ELogVerbosity::Display, *Title);
		for (const FString& Text : Lines)
		{
			Ar.Log(ELogVerbosity::Display, *Text);
		}

		if (GEngine)
		{
			const FString ScreenText = Lines.IsEmpty() ? Title : Title + TEXT("\n") + FString::Join(Lines, TEXT("\n"));
			GEngine->AddOnScreenDebugMessage(ScreenKey, 10.f, FColor(170, 230, 170), ScreenText);
		}
	}
}
#endif
