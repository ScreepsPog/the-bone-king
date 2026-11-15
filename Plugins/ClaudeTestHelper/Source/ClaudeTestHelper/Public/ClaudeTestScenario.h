#pragma once

#include "CoreMinimal.h"

/**
 * Game test scripts ("scenarios"): plain text files in <project>/Tests/Scenarios, one step per line, run in the game by the
 * Test.Scenario console command (ue.py game <name> runs one off-screen). They drive the game with console commands and check its log.
 *
 *   # a comment                     (# map: /Game/Maps/Lvl and # render [WxH] are headers ue.py reads)
 *   cmd Stat Unit                   runs a console command
 *   expect Some text                waits (15 s) for a log line containing the text, after the last match and never before the last cmd
 *   expect(30) Some text            the same, waiting up to 30 seconds
 *   expectnum 400 650 scrolled up   waits (15 s) for a log line with the text and a number from 400 to 650 right after it
 *   reject Some text                fails the scenario if a log line with the text shows up from now on (until the end, or allow)
 *   allow Some text                 stops rejecting that text
 *   wait 2                          waits 2 seconds
 *   note Now the door should open   writes a note in the log
 *   shot pause_menu                 saves a screenshot as pause_menu.png in screenshot runs (ue.py uishots); skipped otherwise
 *   remote PUT /remote/batch {...}  sends an HTTP request to the game's own Remote Control web server ({world} becomes the
 *                                   world's path) and logs "Remote reply <code>: <body on one line>"; ue.py turns the server on.
 */
namespace ClaudeTestScenario
{
	enum class EStepKind : uint8
	{
		Command,
		Expect,
		ExpectNumber,
		Reject,
		Allow,
		Wait,
		Note,
		Shot,
		Remote
	};

	struct FStep
	{
		EStepKind Kind = EStepKind::Note;
		FString Text;
		/** Expect: how long to wait. Wait: how long. */
		float Seconds = 0.f;
		/** ExpectNumber: the lowest and highest number that pass. */
		double Min = 0.0;
		double Max = 0.0;
		/** Line in the file, for messages. */
		int32 Line = 0;
		/** Remote: the HTTP verb (Text is the route) and the request's body. */
		FString Verb;
		FString Body;
	};

	/** How long an expect waits when it doesn't say. */
	constexpr float DefaultExpectSeconds = 15.f;

	/** Reads a scenario. False with OutError ("line 3: ...") when a line can't be understood; nothing runs then. */
	CLAUDETESTHELPER_API bool Parse(const FString& Text, TArray<FStep>& OutSteps, FString& OutError);

	/** The number right after Text in Line (spaces allowed between), like 590 in "scrolled up 590 px". False if there's none. */
	CLAUDETESTHELPER_API bool NumberAfter(const FString& Line, const FString& Text, double& OutNumber);

	/** A remote step's body with {world} replaced by the world's path (/Game/Maps/Lvl.Lvl), as Remote Control addresses objects. */
	CLAUDETESTHELPER_API FString FillWorld(const FString& Body, const FString& WorldPath);

	/** The log lines for a remote step's reply: the reply on one line, then one line per response of a batch reply. */
	CLAUDETESTHELPER_API TArray<FString> DescribeRemoteReply(int32 Code, const FString& Body);
}
