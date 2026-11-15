#pragma once

#include "CoreMinimal.h"
#include "Sim/CrowdTypes.h"

/**
 * Command mode: the player is choosing how many of the army to send, and which kind. Plain data, no engine objects.
 * Available is always the number of that kind that can be sent right now.
 */
class FCommandState
{
public:
	/** Enters command mode with half of what can be sent (rounded up), any kind. False, and stays out, with nothing to send. */
	bool Enter(int32 Available);

	void Exit() { bActive = false; }

	bool IsActive() const { return bActive; }

	/** How many will be sent: 1 to Available, or 0 when none of the chosen kind can be sent. */
	int32 GetCount() const { return Count; }

	ECrowdCommandKind GetKind() const { return Kind; }

	/** The mouse wheel: up adds, down takes away, held between 1 and Available. */
	void Scroll(int32 Notches, int32 Available);

	/** Right mouse: the next kind. The caller then calls Clamp with what that kind has. */
	ECrowdCommandKind CycleKind();

	/**
	 * Right mouse: the next kind the army has any of. Available holds how many of each kind can be sent, in the order
	 * of ECrowdCommandKind. A kind with none is passed over.
	 */
	ECrowdCommandKind CycleKind(TConstArrayView<int32> Available);

	/** Holds the count between 1 and Available (0 when Available is 0). */
	void Clamp(int32 Available);

private:
	bool bActive = false;
	int32 Count = 0;
	ECrowdCommandKind Kind = ECrowdCommandKind::Any;
};
