#include "Sim/LevelLayout.h"

bool FLevelLayout::Blocks(ELevelPiece Kind)
{
	switch (Kind)
	{
	case ELevelPiece::House:
	case ELevelPiece::Medical:
	case ELevelPiece::Dock:
	case ELevelPiece::Coal:
	case ELevelPiece::Tree:
	case ELevelPiece::River:
		return true;
	default:
		return false;
	}
}

bool FLevelLayout::IsLow(ELevelPiece Kind)
{
	return Kind == ELevelPiece::River;
}

const TCHAR* FLevelLayout::GetPieceName(ELevelPiece Kind)
{
	switch (Kind)
	{
	case ELevelPiece::House:
		return TEXT("House");
	case ELevelPiece::Medical:
		return TEXT("Medical");
	case ELevelPiece::Dock:
		return TEXT("Dock");
	case ELevelPiece::Coal:
		return TEXT("Coal");
	case ELevelPiece::Tree:
		return TEXT("Tree");
	case ELevelPiece::River:
		return TEXT("River");
	case ELevelPiece::Bridge:
		return TEXT("Bridge");
	case ELevelPiece::Path:
		return TEXT("Path");
	case ELevelPiece::Farmland:
		return TEXT("Farmland");
	case ELevelPiece::Graveyard:
		return TEXT("Graveyard");
	case ELevelPiece::Lamp:
		return TEXT("Lamp");
	default:
		return TEXT("Unknown");
	}
}

int32 FLevelLayout::CountPieces(ELevelPiece Kind) const
{
	int32 Count = 0;
	for (const FLevelPiece& Piece : Pieces)
	{
		Count += Piece.Kind == Kind ? 1 : 0;
	}
	return Count;
}

TArray<FCrowdObstacle> FLevelLayout::MakeObstacles() const
{
	TArray<FCrowdObstacle> Obstacles = Walls;
	for (const FLevelPiece& Piece : Pieces)
	{
		if (Blocks(Piece.Kind))
		{
			FCrowdObstacle Obstacle;
			Obstacle.Center = Piece.Center;
			// The simulation's boxes are never turned: a turned piece blocks the square box around it.
			const double Turn = FMath::DegreesToRadians(static_cast<double>(Piece.Yaw));
			const double Cos = FMath::Abs(FMath::Cos(Turn));
			const double Sin = FMath::Abs(FMath::Sin(Turn));
			Obstacle.HalfSize = FVector2D(Piece.HalfSize.X * Cos + Piece.HalfSize.Y * Sin, Piece.HalfSize.X * Sin + Piece.HalfSize.Y * Cos);
			Obstacle.bLow = IsLow(Piece.Kind);
			Obstacle.bBoltPasses = Piece.Kind == ELevelPiece::House || Piece.Kind == ELevelPiece::Lamp;
			Obstacles.Add(Obstacle);
		}
	}
	return Obstacles;
}

bool FLevelLayout::CanWalk(FVector2D From, FVector2D To, double Clearance) const
{
	// A flood over a grid of 100 unit squares that covers the level.
	const double Cell = 100.0;
	const TArray<FCrowdObstacle> Obstacles = MakeObstacles();
	FBox2D Bounds(ForceInit);
	Bounds += From;
	Bounds += To;
	for (const FCrowdObstacle& Obstacle : Obstacles)
	{
		Bounds += Obstacle.Center - Obstacle.HalfSize;
		Bounds += Obstacle.Center + Obstacle.HalfSize;
	}
	Bounds = Bounds.ExpandBy(500.0);
	const int32 SizeX = FMath::CeilToInt32(Bounds.GetSize().X / Cell);
	const int32 SizeY = FMath::CeilToInt32(Bounds.GetSize().Y / Cell);
	const auto ToCell = [&Bounds, Cell, SizeX, SizeY](FVector2D Point)
	{
		return FIntPoint(FMath::Clamp(FMath::FloorToInt32((Point.X - Bounds.Min.X) / Cell), 0, SizeX - 1),
			FMath::Clamp(FMath::FloorToInt32((Point.Y - Bounds.Min.Y) / Cell), 0, SizeY - 1));
	};

	TArray<bool> Blocked;
	Blocked.SetNumZeroed(SizeX * SizeY);
	for (const FCrowdObstacle& Obstacle : Obstacles)
	{
		const FIntPoint Low = ToCell(Obstacle.Center - Obstacle.HalfSize - FVector2D(Clearance + Cell, Clearance + Cell));
		const FIntPoint High = ToCell(Obstacle.Center + Obstacle.HalfSize + FVector2D(Clearance + Cell, Clearance + Cell));
		for (int32 Y = Low.Y; Y <= High.Y; ++Y)
		{
			for (int32 X = Low.X; X <= High.X; ++X)
			{
				const FVector2D Middle = Bounds.Min + FVector2D((X + 0.5) * Cell, (Y + 0.5) * Cell);
				if (Obstacle.Contains(Middle, Clearance))
				{
					Blocked[Y * SizeX + X] = true;
				}
			}
		}
	}

	const FIntPoint StartCell = ToCell(From);
	const FIntPoint EndCell = ToCell(To);
	if (Blocked[StartCell.Y * SizeX + StartCell.X] || Blocked[EndCell.Y * SizeX + EndCell.X])
	{
		return false;
	}
	TArray<FIntPoint> Open;
	Open.Add(StartCell);
	Blocked[StartCell.Y * SizeX + StartCell.X] = true;
	const FIntPoint Steps[] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
	for (int32 Index = 0; Index < Open.Num(); ++Index)
	{
		const FIntPoint Here = Open[Index];
		if (Here == EndCell)
		{
			return true;
		}
		for (const FIntPoint& Step : Steps)
		{
			const FIntPoint Next = Here + Step;
			if (Next.X < 0 || Next.Y < 0 || Next.X >= SizeX || Next.Y >= SizeY || Blocked[Next.Y * SizeX + Next.X])
			{
				continue;
			}
			Blocked[Next.Y * SizeX + Next.X] = true;
			Open.Add(Next);
		}
	}
	return false;
}

FLevelLayout FLevelLayout::MakePractice()
{
	FLevelLayout Layout;
	Layout.Name = TEXT("Practice");
	Layout.Shrine = FVector2D(420.0, 120.0);
	Layout.bWaves = true;

	// +X is up the screen and +Y is to the right. The start, at the origin, is kept clear.
	const auto Box = [&Layout](double CenterX, double CenterY, double HalfX, double HalfY)
	{
		FCrowdObstacle Obstacle;
		Obstacle.Center = FVector2D(CenterX, CenterY);
		Obstacle.HalfSize = FVector2D(HalfX, HalfY);
		Layout.Walls.Add(Obstacle);
	};
	// A long wall ahead and to the left.
	Box(900.0, -450.0, 50.0, 350.0);
	// A wall across, to the right.
	Box(-250.0, 950.0, 400.0, 50.0);
	// A corner behind and to the left.
	Box(-900.0, -600.0, 300.0, 50.0);
	Box(-1150.0, -300.0, 50.0, 250.0);
	// A gate on the left: two walls with a gap 200 wide.
	Box(-250.0, -1350.0, 350.0, 50.0);
	Box(650.0, -1350.0, 350.0, 50.0);
	// Pillars.
	Box(350.0, 520.0, 60.0, 60.0);
	Box(-520.0, 250.0, 60.0, 60.0);
	Box(1350.0, 650.0, 80.0, 80.0);
	Box(1500.0, -1100.0, 80.0, 80.0);
	return Layout;
}

int32 FLevelLayout::CountPostEnemies() const
{
	int32 Count = bHasCommander ? EscortGrunts + EscortBrutes : 0;
	for (const FLevelPost& Post : Posts)
	{
		Count += Post.Grunts + Post.Brutes;
	}
	return Count;
}

FLevelLayout FLevelLayout::MakeOutskirts(double WantedUnitsPerPixel)
{
	FLevelLayout Layout;
	Layout.Name = TEXT("Outskirts");

	// Everything below is read off the director's drawing in its own pixels (about 1170 wide, 1470 tall, north at the top).
	// The wizard starts at the bottom of the east road and goes north, which is up the screen (+X).
	const double UnitsPerPixel = FMath::Clamp(WantedUnitsPerPixel, 5.0, 40.0);
	const FVector2D StartPixel(697.0, 1375.0);
	const auto World = [UnitsPerPixel, StartPixel](double PixelX, double PixelY)
	{
		return FVector2D((StartPixel.Y - PixelY) * UnitsPerPixel, (PixelX - StartPixel.X) * UnitsPerPixel);
	};
	// A box by its middle and half its width and height on the drawing.
	const auto Box = [&Layout, &World, UnitsPerPixel](ELevelPiece Kind, double PixelX, double PixelY, double HalfWide, double HalfTall, float Yaw = 0.0f)
	{
		FLevelPiece Piece;
		Piece.Kind = Kind;
		Piece.Center = World(PixelX, PixelY);
		Piece.HalfSize = FVector2D(HalfTall * UnitsPerPixel, HalfWide * UnitsPerPixel);
		Piece.Yaw = Yaw;
		Layout.Pieces.Add(Piece);
	};
	// A walkway along a line of points, as one turned plate for each stretch.
	const double PathHalfWidth = 15.0 * UnitsPerPixel;
	const auto Road = [&Layout, &World, PathHalfWidth](std::initializer_list<FVector2D> Pixels)
	{
		const FVector2D* Previous = nullptr;
		for (const FVector2D& Pixel : Pixels)
		{
			if (Previous)
			{
				const FVector2D A = World(Previous->X, Previous->Y);
				const FVector2D B = World(Pixel.X, Pixel.Y);
				FLevelPiece Piece;
				Piece.Kind = ELevelPiece::Path;
				Piece.Center = (A + B) * 0.5;
				// Long enough to cover the corner where two stretches meet.
				Piece.HalfSize = FVector2D(FVector2D::Distance(A, B) * 0.5 + PathHalfWidth * 0.6, PathHalfWidth);
				Piece.Yaw = FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X));
				Layout.Pieces.Add(Piece);
			}
			Previous = &Pixel;
		}
	};

	// ---- The river: blue, top to bottom. Built from short straight slices that follow its middle line.
	const FVector2D RiverLine[] = {
		FVector2D(580.0, 110.0), FVector2D(600.0, 250.0), FVector2D(650.0, 400.0), FVector2D(660.0, 520.0), FVector2D(640.0, 640.0),
		FVector2D(590.0, 760.0), FVector2D(545.0, 860.0), FVector2D(535.0, 960.0), FVector2D(560.0, 1060.0), FVector2D(545.0, 1160.0),
		FVector2D(545.0, 1420.0),
	};
	const auto RiverMiddle = [&RiverLine](double PixelY)
	{
		for (int32 Index = 1; Index < UE_ARRAY_COUNT(RiverLine); ++Index)
		{
			if (PixelY <= RiverLine[Index].Y)
			{
				const double Alpha = (PixelY - RiverLine[Index - 1].Y) / (RiverLine[Index].Y - RiverLine[Index - 1].Y);
				return FMath::Lerp(RiverLine[Index - 1].X, RiverLine[Index].X, FMath::Clamp(Alpha, 0.0, 1.0));
			}
		}
		return RiverLine[UE_ARRAY_COUNT(RiverLine) - 1].X;
	};
	// Two bridges, as the stretches of the drawing where a walkway crosses the water.
	const FVector2D Bridges[] = { FVector2D(600.0, 660.0), FVector2D(1230.0, 1290.0) };
	const double RiverHalfWide = 28.0;
	const double SliceTall = 30.0;
	for (double PixelY = 90.0; PixelY < 1410.0; PixelY += SliceTall)
	{
		bool bBridge = false;
		for (const FVector2D& Bridge : Bridges)
		{
			bBridge |= PixelY < Bridge.Y && PixelY + SliceTall > Bridge.X;
		}
		if (!bBridge)
		{
			Box(ELevelPiece::River, RiverMiddle(PixelY + SliceTall * 0.5), PixelY + SliceTall * 0.5, RiverHalfWide, SliceTall * 0.5);
		}
	}
	for (const FVector2D& Bridge : Bridges)
	{
		const double Middle = (Bridge.X + Bridge.Y) * 0.5;
		Box(ELevelPiece::Bridge, RiverMiddle(Middle), Middle, RiverHalfWide + 6.0, (Bridge.Y - Bridge.X) * 0.5);
	}
	// The pond beside the river, with its tree.
	Box(ELevelPiece::River, 728.0, 505.0, 40.0, 70.0);

	// ---- Walkways: brown.
	// The east road, bottom to the top right: the way the wizard takes.
	Road({ FVector2D(697.0, 1400.0), FVector2D(697.0, 1250.0), FVector2D(700.0, 1000.0), FVector2D(735.0, 860.0), FVector2D(760.0, 720.0),
		FVector2D(790.0, 640.0), FVector2D(860.0, 580.0), FVector2D(940.0, 500.0), FVector2D(990.0, 420.0), FVector2D(985.0, 330.0),
		FVector2D(930.0, 230.0), FVector2D(860.0, 115.0) });
	// The road across the middle, from the far left over the middle bridge to the east road.
	Road({ FVector2D(95.0, 510.0), FVector2D(180.0, 522.0), FVector2D(370.0, 580.0), FVector2D(460.0, 612.0), FVector2D(600.0, 637.0),
		FVector2D(790.0, 640.0) });
	// The road down the west bank from the top.
	Road({ FVector2D(515.0, 115.0), FVector2D(520.0, 300.0), FVector2D(585.0, 420.0), FVector2D(600.0, 520.0), FVector2D(597.0, 630.0) });
	// The west road south from the middle road, winding down to the bottom bridge.
	Road({ FVector2D(445.0, 615.0), FVector2D(445.0, 740.0), FVector2D(415.0, 820.0), FVector2D(400.0, 900.0), FVector2D(385.0, 970.0),
		FVector2D(330.0, 1030.0), FVector2D(270.0, 1085.0), FVector2D(290.0, 1130.0), FVector2D(385.0, 1160.0), FVector2D(435.0, 1205.0),
		FVector2D(445.0, 1262.0) });
	// The road over the bottom bridge, and on off the bottom of the drawing.
	Road({ FVector2D(445.0, 1262.0), FVector2D(697.0, 1262.0) });
	Road({ FVector2D(445.0, 1262.0), FVector2D(420.0, 1340.0), FVector2D(340.0, 1400.0) });

	// ---- Farmland: green, top middle.
	Box(ELevelPiece::Farmland, 742.0, 238.0, 137.0, 112.0);

	// ---- Graveyards: RIP and R.
	Box(ELevelPiece::Graveyard, 155.0, 445.0, 50.0, 42.0, -25.0f);
	Box(ELevelPiece::Graveyard, 272.0, 510.0, 82.0, 40.0, -20.0f);
	Box(ELevelPiece::Graveyard, 154.0, 841.0, 46.0, 19.0);
	Box(ELevelPiece::Graveyard, 780.0, 1265.0, 45.0, 30.0);
	Box(ELevelPiece::Graveyard, 772.0, 1355.0, 57.0, 28.0);

	// ---- Buildings. H, T, F, PS and the unlabelled ones are houses. Hos and + are medical. Station is the boat dock.
	struct FBuilding
	{
		double X;
		double Y;
		double HalfWide;
		double HalfTall;
	};
	const FBuilding Houses[] = {
		// North-west block, above the middle road.
		{ 365, 160, 17, 28 }, { 427, 200, 42, 40 }, { 480, 150, 15, 22 }, { 372, 270, 17, 30 }, { 445, 262, 20, 30 }, { 485, 265, 14, 20 },
		{ 455, 335, 62, 33 }, { 362, 338, 18, 30 }, { 468, 412, 18, 30 }, { 512, 392, 14, 17 }, { 543, 425, 17, 19 }, { 543, 500, 33, 33 },
		{ 480, 572, 20, 20 }, { 530, 588, 10, 17 }, { 562, 588, 10, 17 },
		// West, below the middle road.
		{ 118, 585, 15, 25 }, { 120, 655, 15, 25 }, { 183, 672, 15, 25 }, { 230, 635, 15, 30 }, { 283, 645, 25, 28 }, { 380, 672, 20, 28 },
		{ 565, 682, 18, 30 }, { 307, 790, 15, 28 }, { 368, 765, 17, 28 }, { 492, 775, 20, 30 }, { 540, 772, 14, 30 }, { 472, 815, 9, 20 },
		{ 222, 822, 15, 22 }, { 305, 855, 15, 28 }, { 355, 848, 14, 22 }, { 470, 865, 18, 28 }, { 197, 937, 38, 38 }, { 292, 928, 15, 28 },
		{ 338, 930, 14, 28 }, { 450, 930, 25, 28 }, { 497, 1010, 15, 20 }, { 220, 1015, 15, 20 }, { 272, 1000, 26, 24 }, { 188, 1080, 28, 38 },
		{ 342, 1082, 30, 30 }, { 388, 1045, 14, 17 }, { 386, 1100, 12, 14 }, { 215, 1140, 15, 22 }, { 458, 1140, 26, 24 }, { 250, 1178, 12, 20 },
		{ 295, 1207, 30, 36 }, { 362, 1207, 14, 24 }, { 492, 1222, 24, 18 }, { 145, 1240, 38, 38 }, { 258, 1282, 15, 30 }, { 295, 1302, 12, 28 },
		{ 325, 1277, 12, 19 }, { 150, 1345, 38, 38 }, { 500, 1345, 28, 30 },
		// Between the river and the east road.
		{ 735, 603, 14, 16 }, { 690, 705, 26, 33 }, { 678, 785, 15, 30 }, { 658, 865, 20, 36 }, { 608, 888, 9, 20 }, { 633, 1075, 20, 38 },
		{ 620, 1205, 28, 20 }, { 645, 1310, 12, 18 }, { 607, 1352, 12, 30 },
		// East of the east road, north part.
		{ 785, 575, 17, 20 }, { 830, 555, 13, 20 }, { 840, 480, 17, 40 }, { 883, 497, 12, 21 }, { 905, 140, 14, 14 }, { 950, 145, 15, 18 },
		{ 1000, 280, 18, 33 }, { 928, 355, 12, 22 }, { 1040, 435, 18, 38 }, { 1017, 518, 28, 25 }, { 968, 580, 15, 22 }, { 923, 612, 15, 22 },
		// East, the middle: PS, Th and the houses around them.
		{ 895, 743, 55, 31 }, { 895, 807, 52, 27 }, { 818, 815, 14, 21 }, { 784, 857, 12, 21 }, { 835, 862, 17, 19 }, { 910, 870, 45, 23 },
		{ 915, 917, 48, 14 }, { 1020, 855, 38, 38 }, { 772, 945, 19, 38 }, { 815, 972, 12, 24 }, { 860, 965, 13, 38 },
		// East, the dense block of small houses.
		{ 1005, 935, 15, 22 }, { 1055, 935, 15, 22 }, { 900, 965, 15, 22 }, { 975, 985, 15, 22 }, { 1035, 990, 15, 22 }, { 920, 1018, 15, 22 },
		{ 840, 1050, 15, 22 }, { 885, 1058, 15, 22 }, { 928, 1072, 15, 22 }, { 962, 1040, 15, 22 }, { 1005, 1050, 15, 22 }, { 1048, 1050, 15, 22 },
		{ 825, 1108, 15, 22 }, { 895, 1118, 15, 22 }, { 990, 1108, 15, 22 }, { 1030, 1112, 15, 22 }, { 992, 1168, 15, 22 },
		// East, the bottom.
		{ 757, 1020, 12, 17 }, { 772, 1087, 36, 33 }, { 752, 1157, 14, 21 }, { 795, 1157, 14, 21 }, { 833, 1187, 22, 33 }, { 860, 1262, 17, 25 },
		{ 928, 1292, 35, 35 }, { 1010, 1327, 38, 38 },
	};
	for (const FBuilding& House : Houses)
	{
		Box(ELevelPiece::House, House.X, House.Y, House.HalfWide, House.HalfTall);
	}
	const FBuilding Medical[] = {
		{ 240, 732, 35, 40 }, { 490, 682, 22, 32 }, { 440, 1003, 35, 30 }, { 368, 1325, 40, 30 }, { 885, 680, 55, 33 }, { 805, 750, 19, 33 },
		{ 987, 192, 14, 36 },
	};
	for (const FBuilding& House : Medical)
	{
		Box(ELevelPiece::Medical, House.X, House.Y, House.HalfWide, House.HalfTall);
	}
	Box(ELevelPiece::Dock, 612.0, 972.0, 42.0, 38.0);
	Box(ELevelPiece::Coal, 897.0, 318.0, 22.0, 18.0);
	Box(ELevelPiece::Coal, 922.0, 452.0, 24.0, 18.0);

	// ---- Trees: green.
	const FVector2D Trees[] = {
		FVector2D(285, 390), FVector2D(335, 405), FVector2D(270, 440), FVector2D(315, 460), FVector2D(400, 425), FVector2D(365, 480),
		FVector2D(442, 472), FVector2D(487, 510), FVector2D(370, 540), FVector2D(430, 555), FVector2D(165, 622), FVector2D(315, 705),
		FVector2D(168, 765), FVector2D(262, 822), FVector2D(232, 880), FVector2D(512, 838), FVector2D(145, 992), FVector2D(468, 1048),
		FVector2D(505, 1072), FVector2D(428, 1092), FVector2D(505, 1137), FVector2D(163, 1168), FVector2D(355, 1255), FVector2D(202, 1290),
		FVector2D(252, 1337), FVector2D(818, 350), FVector2D(720, 378), FVector2D(820, 405), FVector2D(785, 432), FVector2D(865, 415),
		FVector2D(1030, 357), FVector2D(948, 392), FVector2D(995, 620), FVector2D(1025, 688), FVector2D(968, 722), FVector2D(1008, 735),
		FVector2D(1038, 765), FVector2D(972, 780), FVector2D(818, 912), FVector2D(938, 965), FVector2D(805, 1032), FVector2D(860, 1100),
		FVector2D(950, 1095), FVector2D(935, 1137), FVector2D(770, 1192), FVector2D(888, 1202), FVector2D(952, 1217), FVector2D(1050, 1177),
		FVector2D(628, 1150),
	};
	for (const FVector2D& Tree : Trees)
	{
		Box(ELevelPiece::Tree, Tree.X, Tree.Y, 4.0, 4.0);
		// A trunk is a trunk at any scale.
		Layout.Pieces.Last().HalfSize = FVector2D(40.0, 40.0);
	}

	// ---- Lamp posts: yellow. They do not block.
	const FVector2D Lamps[] = {
		FVector2D(590, 410), FVector2D(345, 605), FVector2D(420, 603), FVector2D(535, 612), FVector2D(418, 678), FVector2D(395, 775),
		FVector2D(378, 885), FVector2D(338, 962), FVector2D(250, 1085), FVector2D(350, 1140), FVector2D(405, 1200), FVector2D(738, 630),
		FVector2D(727, 725), FVector2D(700, 822), FVector2D(680, 970), FVector2D(668, 1080), FVector2D(672, 1172), FVector2D(662, 1332),
		FVector2D(915, 205), FVector2D(972, 392),
	};
	for (const FVector2D& Lamp : Lamps)
	{
		Box(ELevelPiece::Lamp, Lamp.X, Lamp.Y, 1.0, 1.0);
		Layout.Pieces.Last().HalfSize = FVector2D(10.0, 10.0);
	}

	// ---- The edges of the drawing are walls, so nothing walks around the end of the river or off the level.
	{
		const FVector2D Low = World(85.0, 1405.0);
		const FVector2D High = World(1075.0, 105.0);
		const FVector2D Middle = (Low + High) * 0.5;
		const FVector2D Half = (High - Low) * 0.5;
		const double Thick = 100.0;
		const auto Edge = [&Layout](FVector2D Center, FVector2D HalfSize)
		{
			FCrowdObstacle Obstacle;
			Obstacle.Center = Center;
			Obstacle.HalfSize = HalfSize;
			Layout.Walls.Add(Obstacle);
		};
		Edge(FVector2D(High.X + Thick, Middle.Y), FVector2D(Thick, Half.Y + Thick * 2.0));
		Edge(FVector2D(Low.X - Thick, Middle.Y), FVector2D(Thick, Half.Y + Thick * 2.0));
		Edge(FVector2D(Middle.X, Low.Y - Thick), FVector2D(Half.X + Thick * 2.0, Thick));
		Edge(FVector2D(Middle.X, High.Y + Thick), FVector2D(Half.X + Thick * 2.0, Thick));
	}

	// ---- The wizard's way: up the east road. The shrine stands where the middle bridge meets it.
	Layout.Start = World(StartPixel.X, StartPixel.Y);
	Layout.Shrine = World(747.0, 668.0);
	const auto Post = [&Layout, &World](double PixelX, double PixelY, int32 Grunts, int32 Brutes)
	{
		FLevelPost NewPost;
		NewPost.Position = World(PixelX, PixelY);
		NewPost.Grunts = Grunts;
		NewPost.Brutes = Brutes;
		Layout.Posts.Add(NewPost);
	};
	// Along the east road, each larger than the last.
	Post(697.0, 1200.0, 6, 0);
	Post(700.0, 1030.0, 9, 0);
	Post(735.0, 860.0, 10, 1);
	Post(790.0, 640.0, 12, 2);
	Post(862.0, 578.0, 10, 2);
	Post(962.0, 468.0, 12, 3);
	Post(985.0, 330.0, 14, 3);
	// Off the way, for a player who wants more bones: the bottom bridge, the west road, the old graveyard, the north-west.
	Post(480.0, 1262.0, 8, 0);
	Post(445.0, 700.0, 10, 1);
	Post(300.0, 585.0, 12, 0);
	Post(520.0, 250.0, 10, 3);
	Post(400.0, 900.0, 8, 1);
	Post(290.0, 1130.0, 8, 0);
	Post(592.0, 470.0, 8, 1);

	// ---- Roads that patrols walk: along the middle of the walkways.
	const auto Route = [&Layout, &World](std::initializer_list<FVector2D> Pixels)
	{
		TArray<FVector2D>& Points = Layout.Routes.AddDefaulted_GetRef();
		for (const FVector2D& Pixel : Pixels)
		{
			Points.Add(World(Pixel.X, Pixel.Y));
		}
	};
	// The east road, from the start up to the shrine.
	Route({ FVector2D(697.0, 1380.0), FVector2D(697.0, 1250.0), FVector2D(700.0, 1000.0), FVector2D(735.0, 860.0), FVector2D(760.0, 720.0),
		FVector2D(790.0, 640.0) });
	// The east road, from the shrine up toward the commander.
	Route({ FVector2D(790.0, 640.0), FVector2D(860.0, 580.0), FVector2D(940.0, 500.0), FVector2D(990.0, 420.0), FVector2D(985.0, 330.0),
		FVector2D(930.0, 230.0) });
	// The middle road, over the middle bridge and out to the west.
	Route({ FVector2D(790.0, 640.0), FVector2D(600.0, 637.0), FVector2D(460.0, 612.0), FVector2D(370.0, 580.0), FVector2D(180.0, 522.0) });
	// Over the bottom bridge and up the west road.
	Route({ FVector2D(697.0, 1262.0), FVector2D(445.0, 1262.0), FVector2D(435.0, 1205.0), FVector2D(385.0, 1160.0), FVector2D(290.0, 1130.0) });
	// The commander holds the way out at the top, toward the walls.
	Layout.bHasCommander = true;
	Layout.Commander = World(900.0, 195.0);
	Layout.EscortGrunts = 10;
	Layout.EscortBrutes = 3;
	// The kingdom gate: past the commander, on the line the road was taking toward the walls.
	const FVector2D Onward = (Layout.Commander - World(985.0, 330.0)).GetSafeNormal();
	Layout.bHasGate = true;
	Layout.Gate = Layout.Commander + Onward * 60.0 * UnitsPerPixel;
	Layout.GateHalfSize = FVector2D(10.0, 20.0) * UnitsPerPixel;
	Layout.GateYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Onward.Y, Onward.X)));
	return Layout;
}

bool FLevelLayout::Make(const FString& InName, FLevelLayout& OutLayout, double UnitsPerPixel)
{
	if (InName.Equals(TEXT("Outskirts"), ESearchCase::IgnoreCase))
	{
		OutLayout = MakeOutskirts(UnitsPerPixel);
		return true;
	}
	if (InName.Equals(TEXT("Practice"), ESearchCase::IgnoreCase))
	{
		OutLayout = MakePractice();
		return true;
	}
	return false;
}
