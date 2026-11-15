#include "Sim/CommandState.h"

bool FCommandState::Enter(int32 Available)
{
	if (Available <= 0)
	{
		return false;
	}
	bActive = true;
	Kind = ECrowdCommandKind::Any;
	Count = (Available + 1) / 2;
	return true;
}

void FCommandState::Scroll(int32 Notches, int32 Available)
{
	Count += Notches;
	Clamp(Available);
}

ECrowdCommandKind FCommandState::CycleKind()
{
	Kind = static_cast<ECrowdCommandKind>((static_cast<int32>(Kind) + 1) % CrowdCommandKindCount);
	return Kind;
}

ECrowdCommandKind FCommandState::CycleKind(TConstArrayView<int32> Available)
{
	for (int32 Try = 0; Try < CrowdCommandKindCount; ++Try)
	{
		const int32 Next = static_cast<int32>(CycleKind());
		if (Available.IsValidIndex(Next) && Available[Next] > 0)
		{
			return Kind;
		}
	}
	// Nothing can be sent at all.
	Kind = ECrowdCommandKind::Any;
	return Kind;
}

void FCommandState::Clamp(int32 Available)
{
	Count = Available <= 0 ? 0 : FMath::Clamp(Count, 1, Available);
}
