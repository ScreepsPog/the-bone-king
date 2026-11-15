// Reader.Read: calls a Reader's read functions from the console, with the same JSON Live Control gets over Remote Control.

#if !UE_BUILD_SHIPPING
#include "ClaudeTestOutput.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "ClaudeTestReaderCall.h"
#include "UObject/UnrealType.h"

namespace ClaudeTestReaderCommand
{
	namespace
	{
		/** The function's signature as Reader.Read takes it: Name <Param> <Param>. */
		FString Signature(const UFunction* Function)
		{
			FString Text = Function->GetName();
			for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
			{
				if (!It->HasAnyPropertyFlags(CPF_ReturnParm) && (!It->HasAnyPropertyFlags(CPF_OutParm) || It->HasAnyPropertyFlags(CPF_ConstParm)))
				{
					Text += FString::Printf(TEXT(" <%s>"), *It->GetName());
				}
			}
			return Text;
		}

		FAutoConsoleCommandWithWorldArgsAndOutputDevice ReadCommand(
			TEXT("Reader.Read"),
			TEXT("Reader.Read <Reader> [Function] [Arguments]: calls a Reader's read function and prints its result as one line of JSON, as Live Control gets it over Remote Control (e.g. Reader.Read ClaudeTestHelperReader GetScenarioStatus). A Reader is a world subsystem; only its const functions can be called. Without a function, lists them."),
			FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
			{
				UObject* Reader = Args.IsEmpty() ? nullptr : ClaudeTestReaderCall::FindReader(World, Args[0]);
				if (!Reader)
				{
					ClaudeTestOutput::Line(Ar, Args.IsEmpty() ? FString(TEXT("Reader.Read: give a Reader, e.g. Reader.Read ClaudeTestHelperReader GetScenarioStatus."))
						: FString::Printf(TEXT("Reader.Read: no Reader called %s in this world. Run it while playing."), *Args[0]), true);
					return;
				}
				if (Args.Num() == 1)
				{
					TArray<FString> Lines;
					for (const UFunction* Function : ClaudeTestReaderCall::GetReadFunctions(Reader))
					{
						Lines.Add(TEXT("  ") + Signature(Function));
					}
					ClaudeTestOutput::Line(Ar, FString::Printf(TEXT("Reader %s (%s):\n%s"), *Reader->GetClass()->GetName(), *Reader->GetPathName(), *FString::Join(Lines, TEXT("\n"))));
					return;
				}
				FString Json, Problem;
				if (!ClaudeTestReaderCall::Call(Reader, Args[1], TArray<FString>(TArrayView<const FString>(Args).RightChop(2)), Json, Problem))
				{
					ClaudeTestOutput::Line(Ar, FString::Printf(TEXT("Reader.Read: %s."), *Problem), true);
					return;
				}
				ClaudeTestOutput::Line(Ar, FString::Printf(TEXT("Reader %s.%s: %s"), *Reader->GetClass()->GetName(), *Args[1], *Json));
			}));
	}
}
#endif
