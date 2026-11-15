#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Containers/Ticker.h"
#include "ClaudeTestOutput.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Widgets/SViewport.h"

// Input commands for game tests and play testing: press keys, turn the mouse wheel and push sticks the way the player does,
// and rebind keys the way the options menu does.
namespace ClaudeTestInput
{
	constexpr float DefaultHoldSeconds = 0.1f;
	constexpr uint64 HeldScreenKey = 0xC1A0DE03ull;

	FKeyEvent MakeKeyEvent(const FKey& Key)
	{
		return FKeyEvent(Key, FSlateApplication::Get().GetModifierKeys(), /*UserIndex*/ 0, /*bIsRepeat*/ false, /*CharacterCode*/ 0, /*KeyCode*/ 0);
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice PressCommand(
		TEXT("Input.Press"),
		TEXT("Input.Press <Key> [seconds=0.1]: presses a key or gamepad button the way the player does (through the UI first, then the game's key mappings) and lets go after the given time (0.1 s). Keys use Unreal's names: SpaceBar, E, L, Escape, Down, Gamepad_FaceButton_Bottom, Gamepad_DPad_Down."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
		{
			const FKey Key = Args.IsEmpty() ? FKey() : FKey(*Args[0]);
			if (!Key.IsValid())
			{
				ClaudeTestOutput::Line(Ar, FString::Printf(TEXT("Input.Press: '%s' isn't a key Unreal knows (e.g. SpaceBar, E, Gamepad_FaceButton_Bottom)."), Args.IsEmpty() ? TEXT("") : *Args[0]), true);
				return;
			}
			if (!FSlateApplication::IsInitialized())
			{
				ClaudeTestOutput::Line(Ar, TEXT("Input.Press: there's no UI to press keys through here."), true);
				return;
			}
			const float Hold = Args.Num() > 1 ? FMath::Max(0.f, FCString::Atof(*Args[1])) : DefaultHoldSeconds;
			FSlateApplication::Get().ProcessKeyDownEvent(MakeKeyEvent(Key));
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Key](float)
			{
				if (FSlateApplication::IsInitialized())
				{
					FSlateApplication::Get().ProcessKeyUpEvent(MakeKeyEvent(Key));
				}
				return false;
			}), Hold);
			ClaudeTestOutput::Line(Ar, FString::Printf(TEXT("Input: pressed %s for %.2f s."), *Key.ToString(), Hold));
		}));

	FAutoConsoleCommandWithWorldArgsAndOutputDevice WheelCommand(
		TEXT("Input.Wheel"),
		TEXT("Input.Wheel <notches>: turns the mouse wheel with the pointer in the middle of the game's window, the way the player does (the UI under the pointer gets it first). Positive turns it up, away from you."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
		{
			const UGameViewportClient* GameViewport = World ? World->GetGameViewport() : nullptr;
			const TSharedPtr<SViewport> ViewportWidget = GameViewport ? GameViewport->GetGameViewportWidget() : nullptr;
			if (Args.IsEmpty() || !Args[0].IsNumeric())
			{
				ClaudeTestOutput::Line(Ar, TEXT("Input.Wheel: give a number of notches, e.g. Input.Wheel 1 (up) or Input.Wheel -2 (down)."), true);
				return;
			}
			if (!FSlateApplication::IsInitialized() || !ViewportWidget.IsValid())
			{
				ClaudeTestOutput::Line(Ar, TEXT("Input.Wheel: there's no game window to turn the wheel over here."), true);
				return;
			}
			const float Notches = FCString::Atof(*Args[0]);
			const FVector2D Middle = ViewportWidget->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f));
			FSlateApplication& Slate = FSlateApplication::Get();
			const FPointerEvent Wheel(/*UserIndex*/ 0, FSlateApplicationBase::CursorPointerIndex, Middle, Middle, TSet<FKey>(), EKeys::MouseWheelAxis, Notches, Slate.GetModifierKeys());
			const bool bTaken = Slate.ProcessMouseWheelOrGestureEvent(Wheel, nullptr);
			ClaudeTestOutput::Line(Ar, FString::Printf(TEXT("Input: turned the mouse wheel %g notch%s %s at (%.0f, %.0f); %s."), FMath::Abs(Notches), FMath::Abs(Notches) == 1.f ? TEXT("") : TEXT("es"),
				Notches >= 0.f ? TEXT("up") : TEXT("down"), Middle.X, Middle.Y, bTaken ? TEXT("something under the pointer took it") : TEXT("nothing under the pointer took it")));
		}));

	/** The stick's direction button a gamepad presses along with the stick, like Gamepad_LeftStick_Up for Gamepad_LeftY above 0. */
	FKey StickDirectionKey(const FKey& Axis, float Value)
	{
		const bool bPositive = Value > 0.f;
		if (Axis == EKeys::Gamepad_LeftY) { return bPositive ? EKeys::Gamepad_LeftStick_Up : EKeys::Gamepad_LeftStick_Down; }
		if (Axis == EKeys::Gamepad_LeftX) { return bPositive ? EKeys::Gamepad_LeftStick_Right : EKeys::Gamepad_LeftStick_Left; }
		if (Axis == EKeys::Gamepad_RightY) { return bPositive ? EKeys::Gamepad_RightStick_Up : EKeys::Gamepad_RightStick_Down; }
		if (Axis == EKeys::Gamepad_RightX) { return bPositive ? EKeys::Gamepad_RightStick_Right : EKeys::Gamepad_RightStick_Left; }
		return FKey();
	}

	/** Where a gamepad starts counting a stick as pushed (its direction button goes down), and how its buttons repeat while held.
	 *  The same numbers Unreal's gamepad input (GameInput) uses. */
	constexpr float StickPushedAt = 7849.f / 32768.f;
	constexpr double FirstRepeatSeconds = 0.2;
	constexpr double RepeatSeconds = 0.1;

	FAnalogInputEvent MakeAnalogEvent(const FKey& Axis, float Value)
	{
		return FAnalogInputEvent(Axis, FSlateApplication::Get().GetModifierKeys(), /*UserIndex*/ 0, /*bIsRepeat*/ false, 0, 0, Value);
	}

	FAutoConsoleCommandWithWorldArgsAndOutputDevice StickCommand(
		TEXT("Input.Stick"),
		TEXT("Input.Stick <axis> <value> [seconds=0.5]: holds a gamepad stick the way a real gamepad does, then lets go after the given time (0.5 s). Its position goes in every frame (Gamepad_LeftY from -1, down, to 1, up), and its direction button (Gamepad_LeftStick_Up...) is held, with repeats, while it's pushed past where a gamepad counts it as pushed."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
		{
			const FKey Axis = Args.IsEmpty() ? FKey() : FKey(*Args[0]);
			if (!Axis.IsValid() || !Axis.IsAxis1D() || !Axis.IsGamepadKey() || Args.Num() < 2 || !Args[1].IsNumeric())
			{
				ClaudeTestOutput::Line(Ar, TEXT("Input.Stick: give a stick axis and how far it's pushed, e.g. Input.Stick Gamepad_LeftY 1 0.5 (all the way up for half a second)."), true);
				return;
			}
			if (!FSlateApplication::IsInitialized())
			{
				ClaudeTestOutput::Line(Ar, TEXT("Input.Stick: there's no UI to send the stick through here."), true);
				return;
			}
			const float Value = FMath::Clamp(FCString::Atof(*Args[1]), -1.f, 1.f);
			const float Seconds = Args.Num() > 2 ? FMath::Max(0.f, FCString::Atof(*Args[2])) : 0.5f;
			const FKey Direction = FMath::Abs(Value) > StickPushedAt ? StickDirectionKey(Axis, Value) : FKey();

			FSlateApplication::Get().ProcessAnalogInputEvent(MakeAnalogEvent(Axis, Value));
			if (Direction.IsValid())
			{
				FSlateApplication::Get().ProcessKeyDownEvent(MakeKeyEvent(Direction));
			}
			const double Start = FPlatformTime::Seconds();
			double NextRepeat = Start + FirstRepeatSeconds;
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Axis, Value, Direction, Start, Seconds, NextRepeat](float) mutable
			{
				if (!FSlateApplication::IsInitialized())
				{
					return false;
				}
				FSlateApplication& Slate = FSlateApplication::Get();
				const double Now = FPlatformTime::Seconds();
				if (Now - Start >= Seconds)
				{
					Slate.ProcessAnalogInputEvent(MakeAnalogEvent(Axis, 0.f));
					if (Direction.IsValid())
					{
						Slate.ProcessKeyUpEvent(MakeKeyEvent(Direction));
					}
					return false;
				}
				// A gamepad keeps sending the position while the stick is out of its dead zone; a resting stick sends nothing new.
				if (FMath::Abs(Value) > StickPushedAt)
				{
					Slate.ProcessAnalogInputEvent(MakeAnalogEvent(Axis, Value));
				}
				if (Direction.IsValid() && Now >= NextRepeat)
				{
					Slate.ProcessKeyDownEvent(FKeyEvent(Direction, Slate.GetModifierKeys(), /*UserIndex*/ 0, /*bIsRepeat*/ true, 0, 0));
					NextRepeat += RepeatSeconds;
				}
				return true;
			}));
			ClaudeTestOutput::Line(Ar, FString::Printf(TEXT("Input: holding %s at %.2f for %.2f s%s."), *Axis.ToString(), Value, Seconds,
				Direction.IsValid() ? *FString::Printf(TEXT(" (with %s)"), *Direction.ToString()) : TEXT("")));
		}));

	FAutoConsoleCommandWithWorldArgsAndOutputDevice KeysCommand(
		TEXT("Input.Keys"),
		TEXT("Input.Keys <Key>: shows what a key does right now: each action it triggers (after higher-priority contexts take their keys) and that mapping's player-mappable name."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
		{
			APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
			const UEnhancedPlayerInput* PlayerInput = Player ? Cast<UEnhancedPlayerInput>(Player->PlayerInput) : nullptr;
			const FKey Key = Args.IsEmpty() ? FKey() : FKey(*Args[0]);
			if (!PlayerInput || !Key.IsValid())
			{
				ClaudeTestOutput::Line(Ar, !PlayerInput ? TEXT("Input.Keys: no player input here.") : TEXT("Input.Keys: give a key, e.g. Input.Keys Gamepad_DPad_Down."), true);
				return;
			}
			TArray<FString> Actions;
			for (const FEnhancedActionKeyMapping& Mapping : PlayerInput->GetEnhancedActionMappingsView())
			{
				if (Mapping.Key == Key && Mapping.Action)
				{
					const FName MappingName = Mapping.GetMappingName();
					Actions.Add(MappingName.IsNone() ? Mapping.Action->GetName() : FString::Printf(TEXT("%s (%s)"), *Mapping.Action->GetName(), *MappingName.ToString()));
				}
			}
			ClaudeTestOutput::Line(Ar, FString::Printf(TEXT("Input: %s triggers %s."), *Key.ToString(), Actions.IsEmpty() ? TEXT("nothing") : *FString::Join(Actions, TEXT(", "))));
		}));

	FAutoConsoleCommandWithWorldArgsAndOutputDevice HeldCommand(
		TEXT("Input.Held"),
		TEXT("Input.Held: shows the keys the game sees held right now and what each is doing: the actions it drives, with their values, or that it's ignored until let go."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
		{
			APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
			UEnhancedPlayerInput* PlayerInput = Player ? Cast<UEnhancedPlayerInput>(Player->PlayerInput) : nullptr;
			if (!PlayerInput)
			{
				ClaudeTestOutput::Line(Ar, TEXT("Input.Held: no player input here; run this while playing."), true);
				return;
			}
			TArray<FString> Lines;
			for (const FEnhancedActionKeyMapping& Mapping : PlayerInput->GetEnhancedActionMappingsView())
			{
				const FKeyState* State = Mapping.Action ? PlayerInput->GetKeyState(Mapping.Key) : nullptr;
				if (State && State->bDown)
				{
					Lines.Add(Mapping.bShouldBeIgnored ? FString::Printf(TEXT("%s: %s ignored until let go"), *Mapping.Key.ToString(), *Mapping.Action->GetName())
						: FString::Printf(TEXT("%s: %s %s"), *Mapping.Key.ToString(), *Mapping.Action->GetName(), *PlayerInput->GetActionValue(Mapping.Action).ToString()));
				}
			}
			ClaudeTestOutput::Result(Ar, HeldScreenKey, Lines.IsEmpty() ? TEXT("Held keys: none") : TEXT("Held keys:"), Lines);
		}));

	FAutoConsoleCommandWithWorldArgsAndOutputDevice RebindCommand(
		TEXT("Input.Rebind"),
		TEXT("Input.Rebind <mapping name> <Key|None>: gives a rebindable key mapping a new key, the way the options menu's keybinds do (e.g. Input.Rebind Jump K); None clears it back to its default. Saved with the player's input settings."),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
		{
			APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
			UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player ? Player->GetLocalPlayer() : nullptr);
			UEnhancedInputUserSettings* UserSettings = Input ? Input->GetUserSettings() : nullptr;
			const FKey Key = Args.Num() > 1 ? FKey(*Args[1]) : FKey();
			const bool bClear = Args.Num() > 1 && Args[1].Equals(TEXT("None"), ESearchCase::IgnoreCase);
			if (Args.Num() < 2 || (!Key.IsValid() && !bClear) || !UserSettings)
			{
				ClaudeTestOutput::Line(Ar, !UserSettings ? TEXT("Input.Rebind: no player input settings here (Enable User Settings must be on).")
					: TEXT("Input.Rebind: give a mapping name and a key, e.g. Input.Rebind Jump K."), true);
				return;
			}
			FMapPlayerKeyArgs MapArgs;
			MapArgs.MappingName = FName(*Args[0]);
			MapArgs.Slot = EPlayerMappableKeySlot::First;
			MapArgs.NewKey = Key;
			FGameplayTagContainer Failure;
			if (bClear)
			{
				UserSettings->UnMapPlayerKey(MapArgs, Failure);
			}
			else
			{
				UserSettings->MapPlayerKey(MapArgs, Failure);
			}
			if (!Failure.IsEmpty())
			{
				ClaudeTestOutput::Line(Ar, FString::Printf(TEXT("Input.Rebind: couldn't map %s: %s."), *Args[0], *Failure.ToStringSimple()), true);
				return;
			}
			UserSettings->ApplySettings();
			UserSettings->SaveSettings();
			ClaudeTestOutput::Line(Ar, bClear ? FString::Printf(TEXT("Input: %s is now back to its default key (%s)."), *Args[0], TEXT("empty if it has none"))
				: FString::Printf(TEXT("Input: %s is now %s."), *Args[0], *Key.ToString()));
		}));
}

#endif
