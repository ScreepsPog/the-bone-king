#include "ClaudeTestReaderCall.h"

#include "Algo/Reverse.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "JsonObjectConverter.h"
#include "Misc/ScopeExit.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/UnrealType.h"

namespace ClaudeTestReaderCall
{
	namespace
	{
		bool IsInput(const FProperty* Param)
		{
			return !Param->HasAnyPropertyFlags(CPF_ReturnParm) && (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ConstParm));
		}

		bool IsOutput(const FProperty* Param)
		{
			return Param->HasAnyPropertyFlags(CPF_ReturnParm) || (Param->HasAnyPropertyFlags(CPF_OutParm) && !Param->HasAnyPropertyFlags(CPF_ConstParm));
		}

		bool SetParam(FProperty* Param, void* Buffer, const FString& Text, FString& OutProblem)
		{
			void* Value = Param->ContainerPtrToValuePtr<void>(Buffer);
			if (FStrProperty* String = CastField<FStrProperty>(Param))
			{
				String->SetPropertyValue(Value, Text);
				return true;
			}
			if (FNameProperty* Name = CastField<FNameProperty>(Param))
			{
				Name->SetPropertyValue(Value, FName(*Text));
				return true;
			}
			if (!Param->ImportText_Direct(*Text, Value, nullptr, PPF_None))
			{
				OutProblem = FString::Printf(TEXT("'%s' isn't a value for %s (%s)"), *Text, *Param->GetName(), *Param->GetCPPType());
				return false;
			}
			return true;
		}
	}

	UObject* FindReader(const UWorld* World, const FString& ClassName)
	{
		if (!World || ClassName.IsEmpty())
		{
			return nullptr;
		}
		UClass* Class = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::NativeFirst);
		if (!Class && ClassName.StartsWith(TEXT("U"), ESearchCase::CaseSensitive))
		{
			Class = FindFirstObject<UClass>(*ClassName.Mid(1), EFindFirstObjectOptions::NativeFirst);
		}
		return Class && Class->IsChildOf(UWorldSubsystem::StaticClass()) ? World->GetSubsystemBase(Class) : nullptr;
	}

	TArray<UFunction*> GetReadFunctions(const UObject* Reader)
	{
		TArray<UFunction*> Functions;
		for (TFieldIterator<UFunction> It(Reader ? Reader->GetClass() : nullptr, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			if (It->HasAnyFunctionFlags(FUNC_Const))
			{
				Functions.Add(*It);
			}
		}
		// The field list runs backwards from the header's order.
		Algo::Reverse(Functions);
		return Functions;
	}

	bool Call(UObject* Reader, const FString& FunctionName, const TArray<FString>& Args, FString& OutJson, FString& OutProblem)
	{
		UFunction* Function = Reader ? Reader->FindFunction(FName(*FunctionName)) : nullptr;
		if (!Function)
		{
			OutProblem = FString::Printf(TEXT("%s has no function %s"), *GetNameSafe(Reader ? Reader->GetClass() : nullptr), *FunctionName);
			return false;
		}
		if (!Function->HasAnyFunctionFlags(FUNC_Const))
		{
			OutProblem = FString::Printf(TEXT("%s isn't const, so it may change something: only reads can be called"), *FunctionName);
			return false;
		}

		TArray<FProperty*> Inputs;
		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			if (IsInput(*It))
			{
				Inputs.Add(*It);
			}
		}

		TArray<uint8> Buffer;
		Buffer.SetNumZeroed(FMath::Max<int32>(Function->ParmsSize, 1));
		Function->InitializeStruct(Buffer.GetData());
		ON_SCOPE_EXIT { Function->DestroyStruct(Buffer.GetData()); };

		for (int32 Index = 0; Index < Inputs.Num(); ++Index)
		{
			FString Text;
			if (Index < Args.Num())
			{
				const bool bLastString = Index == Inputs.Num() - 1 && CastField<FStrProperty>(Inputs[Index]);
				Text = bLastString ? FString::Join(TArrayView<const FString>(Args).RightChop(Index), TEXT(" ")) : Args[Index];
			}
#if WITH_EDITOR
			else
			{
				// Defaults written in C++ (Count = 4) are kept as metadata in editor builds only.
				Text = Function->GetMetaData(*(TEXT("CPP_Default_") + Inputs[Index]->GetName()));
			}
#endif
			if (!Text.IsEmpty() && !SetParam(Inputs[Index], Buffer.GetData(), Text, OutProblem))
			{
				return false;
			}
		}

		Reader->ProcessEvent(Function, Buffer.GetData());

		const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			if (IsOutput(*It))
			{
				Result->SetField(It->GetName(), FJsonObjectConverter::UPropertyToJsonValue(*It, It->ContainerPtrToValuePtr<void>(Buffer.GetData()),
					0, 0, nullptr, nullptr, EJsonObjectConversionFlags::SkipStandardizeCase));
			}
		}
		OutJson.Reset();
		FJsonSerializer::Serialize(Result, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutJson));
		return true;
	}
}
