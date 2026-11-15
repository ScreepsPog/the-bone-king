#include "ClaudeTestScenario.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ClaudeTestScenarioTests
{
	constexpr EAutomationTestFlags ScenarioTestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestScenarioParseEveryStep, "ClaudeTestHelper.Scenario.ParsesEveryKindOfStep", ClaudeTestScenarioTests::ScenarioTestFlags)
bool FClaudeTestScenarioParseEveryStep::RunTest(const FString& Parameters)
{
	const FString Text =
		TEXT("# a comment\n")
		TEXT("\n")
		TEXT("cmd Stat Unit\n")
		TEXT("expect QST_Test: now on step\n")
		TEXT("expect(30) Started QST_Test\n")
		TEXT("reject Started QST_TestFollowUp\n")
		TEXT("allow Started QST_TestFollowUp\n")
		TEXT("wait 2.5\n")
		TEXT("note Now the lever should work\n")
		TEXT("shot pause_menu\n");

	TArray<ClaudeTestScenario::FStep> Steps;
	FString Error;
	if (!TestTrue(TEXT("parses"), ClaudeTestScenario::Parse(Text, Steps, Error)))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("step count (comments and blank lines skipped)"), Steps.Num(), 8))
	{
		return false;
	}

	using ClaudeTestScenario::EStepKind;
	TestTrue(TEXT("cmd kind"), Steps[0].Kind == EStepKind::Command);
	TestEqual(TEXT("cmd text"), Steps[0].Text, TEXT("Stat Unit"));
	TestEqual(TEXT("cmd line number"), Steps[0].Line, 3);

	TestTrue(TEXT("expect kind"), Steps[1].Kind == EStepKind::Expect);
	TestEqual(TEXT("expect text keeps its colon"), Steps[1].Text, TEXT("QST_Test: now on step"));
	TestEqual(TEXT("expect default wait"), Steps[1].Seconds, ClaudeTestScenario::DefaultExpectSeconds);

	TestTrue(TEXT("expect(30) kind"), Steps[2].Kind == EStepKind::Expect);
	TestEqual(TEXT("expect(30) wait"), Steps[2].Seconds, 30.f);
	TestEqual(TEXT("expect(30) text"), Steps[2].Text, TEXT("Started QST_Test"));

	TestTrue(TEXT("reject kind"), Steps[3].Kind == EStepKind::Reject);
	TestEqual(TEXT("reject text"), Steps[3].Text, TEXT("Started QST_TestFollowUp"));
	TestTrue(TEXT("allow kind"), Steps[4].Kind == EStepKind::Allow);
	TestEqual(TEXT("allow text"), Steps[4].Text, TEXT("Started QST_TestFollowUp"));

	TestTrue(TEXT("wait kind"), Steps[5].Kind == EStepKind::Wait);
	TestEqual(TEXT("wait seconds"), Steps[5].Seconds, 2.5f);

	TestTrue(TEXT("note kind"), Steps[6].Kind == EStepKind::Note);
	TestEqual(TEXT("note text"), Steps[6].Text, TEXT("Now the lever should work"));
	TestEqual(TEXT("note line number"), Steps[6].Line, 9);

	TestTrue(TEXT("shot kind"), Steps[7].Kind == EStepKind::Shot);
	TestEqual(TEXT("shot name"), Steps[7].Text, TEXT("pause_menu"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestScenarioTrimsAndIgnoresCase, "ClaudeTestHelper.Scenario.TrimsSpacesAndIgnoresKeywordCase", ClaudeTestScenarioTests::ScenarioTestFlags)
bool FClaudeTestScenarioTrimsAndIgnoresCase::RunTest(const FString& Parameters)
{
	TArray<ClaudeTestScenario::FStep> Steps;
	FString Error;
	if (!TestTrue(TEXT("parses"), ClaudeTestScenario::Parse(TEXT("   CMD   Save.Status  \r\n\tExpect(5)   Save: saving is\r\n"), Steps, Error)))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("step count"), Steps.Num(), 2))
	{
		return false;
	}
	TestTrue(TEXT("CMD is a command"), Steps[0].Kind == ClaudeTestScenario::EStepKind::Command);
	TestEqual(TEXT("command text trimmed"), Steps[0].Text, TEXT("Save.Status"));
	TestTrue(TEXT("Expect(5) is an expect"), Steps[1].Kind == ClaudeTestScenario::EStepKind::Expect);
	TestEqual(TEXT("expect wait"), Steps[1].Seconds, 5.f);
	TestEqual(TEXT("expect text trimmed"), Steps[1].Text, TEXT("Save: saving is"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestScenarioRejectsBadLines, "ClaudeTestHelper.Scenario.RejectsLinesItCantUnderstand", ClaudeTestScenarioTests::ScenarioTestFlags)
bool FClaudeTestScenarioRejectsBadLines::RunTest(const FString& Parameters)
{
	struct FCase
	{
		const TCHAR* Text;
		const TCHAR* ExpectedStart;
	};
	const FCase Cases[] =
	{
		{ TEXT("cmd Save.Status\nfly away\n"), TEXT("line 2:") },
		{ TEXT("expect\n"), TEXT("line 1:") },
		{ TEXT("cmd\n"), TEXT("line 1:") },
		{ TEXT("wait soon\n"), TEXT("line 1:") },
		{ TEXT("wait -1\n"), TEXT("line 1:") },
		{ TEXT("# ok\nexpect(abc) text\n"), TEXT("line 2:") },
		{ TEXT("expect(10 text\n"), TEXT("line 1:") },
		// A shot is saved as <name>.png, so the name must be one plain word.
		{ TEXT("shot\n"), TEXT("line 1:") },
		{ TEXT("wait 1\nshot pause menu\n"), TEXT("line 2:") },
		{ TEXT("shot ../pause\n"), TEXT("line 1:") },
	};

	for (const FCase& Case : Cases)
	{
		TArray<ClaudeTestScenario::FStep> Steps;
		FString Error;
		const bool bParsed = ClaudeTestScenario::Parse(Case.Text, Steps, Error);
		TestFalse(FString::Printf(TEXT("'%s' is refused"), Case.Text), bParsed);
		TestTrue(FString::Printf(TEXT("'%s' error names its line (got '%s')"), Case.Text, *Error), Error.StartsWith(Case.ExpectedStart));
		TestEqual(FString::Printf(TEXT("'%s' leaves no steps to run"), Case.Text), Steps.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestScenarioEmptyIsError, "ClaudeTestHelper.Scenario.EmptyScenarioIsAnError", ClaudeTestScenarioTests::ScenarioTestFlags)
bool FClaudeTestScenarioEmptyIsError::RunTest(const FString& Parameters)
{
	TArray<ClaudeTestScenario::FStep> Steps;
	FString Error;
	TestFalse(TEXT("a file with only comments is refused (it would pass without testing anything)"), ClaudeTestScenario::Parse(TEXT("# nothing\n\n"), Steps, Error));
	TestFalse(TEXT("says why"), Error.IsEmpty());

	// The same file with one step is fine, so the refusal above is about being empty, not about the comments.
	FString SecondError;
	TestTrue(TEXT("one step after the comments is accepted"), ClaudeTestScenario::Parse(TEXT("# nothing\n\nwait 1\n"), Steps, SecondError));
	TestEqual(TEXT("and gives that one step"), Steps.Num(), 1);
	return true;
}

// expectnum waits for a log line with the text and a number in a range right after it: for values that vary a little from
// run to run, like how far a held stick scrolled.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestScenarioNumberRange, "ClaudeTestHelper.Scenario.ExpectNumberChecksARange", ClaudeTestScenarioTests::ScenarioTestFlags)
bool FClaudeTestScenarioNumberRange::RunTest(const FString& Parameters)
{
	TArray<ClaudeTestScenario::FStep> Steps;
	FString Error;
	if (!TestTrue(TEXT("parses"), ClaudeTestScenario::Parse(TEXT("expectnum 400 650 scrolled up\nExpectNum -5 5.5 drift\n"), Steps, Error)) || !TestEqual(TEXT("two steps"), Steps.Num(), 2))
	{
		AddError(Error);
		return false;
	}
	using ClaudeTestScenario::EStepKind;
	TestTrue(TEXT("expectnum kind"), Steps[0].Kind == EStepKind::ExpectNumber);
	TestEqual(TEXT("its text"), Steps[0].Text, TEXT("scrolled up"));
	TestEqual(TEXT("its lowest number"), Steps[0].Min, 400.0);
	TestEqual(TEXT("its highest number"), Steps[0].Max, 650.0);
	TestEqual(TEXT("it waits as long as expect"), Steps[0].Seconds, ClaudeTestScenario::DefaultExpectSeconds);
	TestEqual(TEXT("negative and decimal limits"), Steps[1].Min, -5.0);
	TestEqual(TEXT("negative and decimal limits"), Steps[1].Max, 5.5);

	for (const TCHAR* Bad : {TEXT("expectnum 400 scrolled up\n"), TEXT("expectnum 650 400 scrolled up\n"), TEXT("expectnum 1 2\n"), TEXT("expectnum\n")})
	{
		TArray<ClaudeTestScenario::FStep> BadSteps;
		FString BadError;
		TestFalse(FString::Printf(TEXT("'%s' is refused"), Bad), ClaudeTestScenario::Parse(Bad, BadSteps, BadError));
		TestTrue(FString::Printf(TEXT("'%s' error names its line (got '%s')"), Bad, *BadError), BadError.StartsWith(TEXT("line 1:")));
	}

	double Number = 0.0;
	TestTrue(TEXT("finds the number after the text"), ClaudeTestScenario::NumberAfter(TEXT("backlog: 20 entries, open, scrolled up 590 px, still"), TEXT("scrolled up"), Number));
	TestEqual(TEXT("which is 590"), Number, 590.0);
	TestTrue(TEXT("reads a negative decimal"), ClaudeTestScenario::NumberAfter(TEXT("drift -3.5 px"), TEXT("drift"), Number));
	TestEqual(TEXT("which is -3.5"), Number, -3.5);
	TestFalse(TEXT("no number right after the text"), ClaudeTestScenario::NumberAfter(TEXT("scrolled up px 590"), TEXT("scrolled up"), Number));
	TestFalse(TEXT("the text isn't in the line"), ClaudeTestScenario::NumberAfter(TEXT("backlog: 20 entries"), TEXT("scrolled up"), Number));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClaudeTestScenarioRemote, "ClaudeTestHelper.Scenario.RemoteSendsAnHttpRequest", ClaudeTestScenarioTests::ScenarioTestFlags)
bool FClaudeTestScenarioRemote::RunTest(const FString& Parameters)
{
	TArray<ClaudeTestScenario::FStep> Steps;
	FString Error;
	const TCHAR* Text = TEXT("remote PUT /remote/batch {\"Requests\": []}\nRemote GET /remote/info\n");
	if (!TestTrue(TEXT("parses"), ClaudeTestScenario::Parse(Text, Steps, Error)) || !TestEqual(TEXT("two steps"), Steps.Num(), 2))
	{
		return false;
	}
	TestTrue(TEXT("a remote step"), Steps[0].Kind == ClaudeTestScenario::EStepKind::Remote);
	TestEqual(TEXT("its verb"), Steps[0].Verb, FString(TEXT("PUT")));
	TestEqual(TEXT("its route"), Steps[0].Text, FString(TEXT("/remote/batch")));
	TestEqual(TEXT("its body, spaces kept"), Steps[0].Body, FString(TEXT("{\"Requests\": []}")));
	TestEqual(TEXT("no body"), Steps[1].Body, FString());

	for (const TCHAR* Bad : {TEXT("remote\n"), TEXT("remote PUT\n"), TEXT("remote FETCH /remote/info\n"), TEXT("remote GET remote/info\n")})
	{
		TArray<ClaudeTestScenario::FStep> BadSteps;
		FString BadError;
		TestFalse(FString::Printf(TEXT("'%s' is refused"), Bad), ClaudeTestScenario::Parse(Bad, BadSteps, BadError));
	}

	TestEqual(TEXT("{world} becomes the world's path"), ClaudeTestScenario::FillWorld(TEXT("{\"objectPath\":\"{world}:Reader_0\"}"), TEXT("/Game/Maps/Lvl.Lvl")),
		FString(TEXT("{\"objectPath\":\"/Game/Maps/Lvl.Lvl:Reader_0\"}")));
	TArray<FString> Lines = ClaudeTestScenario::DescribeRemoteReply(200, TEXT("{\n\t\"Responses\": [ {\"RequestId\": 1, \"ResponseCode\": 200, \"ResponseBody\": {\"ReturnValue\": 15.3}},\n{\"RequestId\": 2, \"ResponseCode\": 400, \"ResponseBody\": null} ]\n}"));
	TestEqual(TEXT("the reply, then one line per batch response"), Lines.Num(), 3);
	if (Lines.Num() == 3)
	{
		TestEqual(TEXT("the reply on one line"), Lines[0].Left(30), FString(TEXT("Remote reply 200: {\"Responses\"")));
		TestEqual(TEXT("first response"), Lines[1], FString(TEXT("Remote response 1: 200 {\"ReturnValue\":15.3}")));
		TestEqual(TEXT("second response"), Lines[2], FString(TEXT("Remote response 2: 400 null")));
	}
	return true;
}

#endif
