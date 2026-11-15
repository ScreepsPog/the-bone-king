#include "ClaudeTestScenario.h"

#include "Algo/AllOf.h"

namespace ClaudeTestScenario
{
	namespace
	{
		bool ParseSeconds(const FString& Text, float& OutSeconds)
		{
			if (!Text.IsNumeric())
			{
				return false;
			}
			OutSeconds = FCString::Atof(*Text);
			return OutSeconds >= 0.f;
		}

		/** One line into a step. False with a reason when it can't be understood. */
		bool ParseLine(const FString& Line, FStep& OutStep, FString& OutReason)
		{
			FString Keyword = Line;
			FString Rest;
			if (Line.Split(TEXT(" "), &Keyword, &Rest, ESearchCase::CaseSensitive))
			{
				Keyword.TrimStartAndEndInline();
				Rest.TrimStartAndEndInline();
			}
			Keyword.ToLowerInline();

			// expect(30): the wait is written inside the brackets.
			if (Keyword.StartsWith(TEXT("expect(")))
			{
				if (!Keyword.EndsWith(TEXT(")")))
				{
					OutReason = TEXT("expect(...) needs a closing bracket, like expect(30) some text");
					return false;
				}
				const FString SecondsText = Keyword.Mid(7, Keyword.Len() - 8);
				if (!ParseSeconds(SecondsText, OutStep.Seconds))
				{
					OutReason = FString::Printf(TEXT("'%s' isn't a number of seconds"), *SecondsText);
					return false;
				}
				Keyword = TEXT("expect");
			}
			else if (Keyword == TEXT("expect"))
			{
				OutStep.Seconds = DefaultExpectSeconds;
			}

			if (Keyword == TEXT("cmd") || Keyword == TEXT("expect") || Keyword == TEXT("reject") || Keyword == TEXT("allow") || Keyword == TEXT("note"))
			{
				if (Rest.IsEmpty())
				{
					OutReason = FString::Printf(TEXT("'%s' needs text after it"), *Keyword);
					return false;
				}
				OutStep.Text = Rest;
				OutStep.Kind = Keyword == TEXT("cmd") ? EStepKind::Command
					: Keyword == TEXT("expect") ? EStepKind::Expect
					: Keyword == TEXT("reject") ? EStepKind::Reject
					: Keyword == TEXT("allow") ? EStepKind::Allow
					: EStepKind::Note;
				return true;
			}
			if (Keyword == TEXT("expectnum"))
			{
				// expectnum <lowest> <highest> <text>
				FString MinText, MaxText, Text;
				FString AfterMin;
				if (!Rest.Split(TEXT(" "), &MinText, &AfterMin) || !AfterMin.TrimStart().Split(TEXT(" "), &MaxText, &Text)
					|| !MinText.IsNumeric() || !MaxText.IsNumeric() || Text.TrimStartAndEnd().IsEmpty())
				{
					OutReason = TEXT("expectnum needs the lowest number, the highest number and the text before the number, like expectnum 400 650 scrolled up");
					return false;
				}
				OutStep.Min = FCString::Atod(*MinText);
				OutStep.Max = FCString::Atod(*MaxText);
				if (OutStep.Min > OutStep.Max)
				{
					OutReason = FString::Printf(TEXT("expectnum's lowest number (%s) is above its highest (%s)"), *MinText, *MaxText);
					return false;
				}
				OutStep.Text = Text.TrimStartAndEnd();
				OutStep.Seconds = DefaultExpectSeconds;
				OutStep.Kind = EStepKind::ExpectNumber;
				return true;
			}
			if (Keyword == TEXT("shot"))
			{
				// Saved as <name>.png, so one plain word: letters, digits, _ and -.
				const bool bPlain = !Rest.IsEmpty() && Algo::AllOf(Rest, [](TCHAR C) { return FChar::IsAlnum(C) || C == TEXT('_') || C == TEXT('-'); });
				if (!bPlain)
				{
					OutReason = FString::Printf(TEXT("shot needs a name made of letters, digits, _ and -, like shot pause_menu (not '%s')"), *Rest);
					return false;
				}
				OutStep.Text = Rest;
				OutStep.Kind = EStepKind::Shot;
				return true;
			}
			if (Keyword == TEXT("wait"))
			{
				if (!ParseSeconds(Rest, OutStep.Seconds))
				{
					OutReason = FString::Printf(TEXT("wait needs a number of seconds, not '%s'"), *Rest);
					return false;
				}
				OutStep.Kind = EStepKind::Wait;
				return true;
			}

			if (Keyword == TEXT("remote"))
			{
				// remote <VERB> </route> [body]: the body keeps its spaces.
				FString Verb, Route, Body;
				if (!Rest.Split(TEXT(" "), &Verb, &Route))
				{
					Verb = Rest;
				}
				Route.TrimStartInline();
				if (Route.Split(TEXT(" "), &Route, &Body))
				{
					Body.TrimStartAndEndInline();
				}
				Verb.ToUpperInline();
				if (!(Verb == TEXT("GET") || Verb == TEXT("PUT") || Verb == TEXT("POST") || Verb == TEXT("DELETE")) || !Route.StartsWith(TEXT("/")))
				{
					OutReason = TEXT("remote needs a verb (GET, PUT, POST or DELETE) and a route starting with /, like remote GET /remote/info");
					return false;
				}
				OutStep.Kind = EStepKind::Remote;
				OutStep.Verb = Verb;
				OutStep.Text = Route;
				OutStep.Body = Body;
				return true;
			}

			OutReason = FString::Printf(TEXT("'%s' isn't a step (use cmd, expect, expectnum, reject, allow, wait, note, shot or remote)"), *Keyword);
			return false;
		}
	}

	bool NumberAfter(const FString& Line, const FString& Text, double& OutNumber)
	{
		for (int32 Found = Line.Find(Text, ESearchCase::CaseSensitive); Found != INDEX_NONE && !Text.IsEmpty();
			Found = Line.Find(Text, ESearchCase::CaseSensitive, ESearchDir::FromStart, Found + 1))
		{
			int32 Start = Found + Text.Len();
			while (Start < Line.Len() && Line[Start] == TEXT(' '))
			{
				++Start;
			}
			int32 End = Start;
			if (End < Line.Len() && Line[End] == TEXT('-'))
			{
				++End;
			}
			const int32 DigitsStart = End;
			while (End < Line.Len() && (FChar::IsDigit(Line[End]) || (Line[End] == TEXT('.') && End + 1 < Line.Len() && FChar::IsDigit(Line[End + 1]))))
			{
				++End;
			}
			if (End > DigitsStart)
			{
				OutNumber = FCString::Atod(*Line.Mid(Start, End - Start));
				return true;
			}
		}
		return false;
	}

	bool Parse(const FString& Text, TArray<FStep>& OutSteps, FString& OutError)
	{
		OutSteps.Reset();
		OutError.Reset();

		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);

		TArray<FStep> Steps;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			FString Line = Lines[Index].TrimStartAndEnd();
			if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
			{
				continue;
			}

			FStep Step;
			Step.Line = Index + 1;
			FString Reason;
			if (!ParseLine(Line, Step, Reason))
			{
				OutError = FString::Printf(TEXT("line %d: %s"), Step.Line, *Reason);
				return false;
			}
			Steps.Add(MoveTemp(Step));
		}

		if (Steps.IsEmpty())
		{
			OutError = TEXT("the scenario has no steps, so it would pass without testing anything");
			return false;
		}
		OutSteps = MoveTemp(Steps);
		return true;
	}
}

namespace ClaudeTestScenario
{
	FString FillWorld(const FString& Body, const FString& WorldPath)
	{
		return Body.Replace(TEXT("{world}"), *WorldPath, ESearchCase::CaseSensitive);
	}

	namespace
	{
		/** JSON on one line, as sent: whitespace outside strings dropped, everything else (numbers too) kept as written. */
		FString Condense(const FString& Json)
		{
			FString Out;
			Out.Reserve(Json.Len());
			bool bInString = false;
			for (int32 Index = 0; Index < Json.Len(); ++Index)
			{
				const TCHAR Char = Json[Index];
				if (bInString)
				{
					Out.AppendChar(Char);
					if (Char == TEXT('\\') && Index + 1 < Json.Len())
					{
						Out.AppendChar(Json[++Index]);
					}
					else if (Char == TEXT('"'))
					{
						bInString = false;
					}
				}
				else if (!FChar::IsWhitespace(Char))
				{
					bInString = Char == TEXT('"');
					Out.AppendChar(Char);
				}
			}
			return Out;
		}

		/** One past the end of the value starting at Start, in condensed JSON. */
		int32 ValueEnd(const FString& Json, int32 Start)
		{
			int32 Depth = 0;
			bool bInString = false;
			for (int32 Index = Start; Index < Json.Len(); ++Index)
			{
				const TCHAR Char = Json[Index];
				if (bInString)
				{
					if (Char == TEXT('\\'))
					{
						++Index;
					}
					else if (Char == TEXT('"'))
					{
						bInString = false;
						if (Depth == 0)
						{
							return Index + 1;
						}
					}
				}
				else if (Char == TEXT('"'))
				{
					bInString = true;
				}
				else if (Char == TEXT('{') || Char == TEXT('['))
				{
					++Depth;
				}
				else if (Char == TEXT('}') || Char == TEXT(']'))
				{
					if (Depth == 0)
					{
						return Index;
					}
					if (--Depth == 0)
					{
						return Index + 1;
					}
				}
				else if (Char == TEXT(',') && Depth == 0)
				{
					return Index;
				}
			}
			return Json.Len();
		}

		/** The text of one of the object's own fields (the object starts at ObjectStart), as written; empty if it has none. */
		FString FieldText(const FString& Json, int32 ObjectStart, const FString& Field)
		{
			int32 Index = ObjectStart + 1;
			while (Index < Json.Len() && Json[Index] == TEXT('"'))
			{
				const int32 NameEnd = ValueEnd(Json, Index);
				const FString Name = Json.Mid(Index + 1, NameEnd - Index - 2);
				const int32 ValueStart = NameEnd + 1;
				const int32 End = ValueEnd(Json, ValueStart);
				if (Name == Field)
				{
					return Json.Mid(ValueStart, End - ValueStart);
				}
				Index = End < Json.Len() && Json[End] == TEXT(',') ? End + 1 : Json.Len();
			}
			return FString();
		}
	}

	TArray<FString> DescribeRemoteReply(int32 Code, const FString& Body)
	{
		const FString Trimmed = Body.TrimStartAndEnd();
		const bool bJson = Trimmed.StartsWith(TEXT("{")) || Trimmed.StartsWith(TEXT("["));
		const FString Text = bJson ? Condense(Trimmed) : Trimmed.Replace(TEXT("\r"), TEXT("")).Replace(TEXT("\n"), TEXT(" "));
		TArray<FString> Lines;
		Lines.Add(FString::Printf(TEXT("Remote reply %d: %s"), Code, *Text));

		// A batch reply: one line per response, so each can be expected on its own.
		const FString Responses = bJson && Text.StartsWith(TEXT("{")) ? FieldText(Text, 0, TEXT("Responses")) : FString();
		int32 Index = Responses.StartsWith(TEXT("[")) ? 1 : Responses.Len();
		while (Index < Responses.Len() && Responses[Index] == TEXT('{'))
		{
			const int32 End = ValueEnd(Responses, Index);
			const FString Response = Responses.Mid(Index, End - Index);
			Lines.Add(FString::Printf(TEXT("Remote response %s: %s %s"), *FieldText(Response, 0, TEXT("RequestId")), *FieldText(Response, 0, TEXT("ResponseCode")),
				*FieldText(Response, 0, TEXT("ResponseBody"))));
			Index = End < Responses.Len() && Responses[End] == TEXT(',') ? End + 1 : Responses.Len();
		}
		return Lines;
	}
}
