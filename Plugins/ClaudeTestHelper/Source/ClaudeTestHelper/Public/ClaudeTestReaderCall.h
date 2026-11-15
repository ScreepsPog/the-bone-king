#pragma once

#include "CoreMinimal.h"

class UFunction;
class UObject;
class UWorld;

/**
 * Calling a Reader's functions by name and getting the result as JSON, the way Remote Control returns it: field names as
 * the C++ names them (bFound, State), enums by name. For the Reader.Read console command and for tests. Only const
 * functions can be called, so a call never changes anything.
 */
namespace ClaudeTestReaderCall
{
	/** A world subsystem by class name, with or without the U (MyPluginReader), in World. Null if there's none. */
	CLAUDETESTHELPER_API UObject* FindReader(const UWorld* World, const FString& ClassName);

	/** The const functions a Reader declares, by name, in the order its header lists them. */
	CLAUDETESTHELPER_API TArray<UFunction*> GetReadFunctions(const UObject* Reader);

	/**
	 * Calls a const function on Reader with Args in parameter order (as text, like the Details panel takes them; the last
	 * string parameter takes the rest of the words, so it can have spaces). Parameters not given keep their defaults.
	 * OutJson gets the return value and out parameters as one JSON object on one line: {"ReturnValue": ...}.
	 * False with OutProblem when the function doesn't exist, isn't const, or an argument can't be read.
	 */
	CLAUDETESTHELPER_API bool Call(UObject* Reader, const FString& FunctionName, const TArray<FString>& Args, FString& OutJson, FString& OutProblem);
}
