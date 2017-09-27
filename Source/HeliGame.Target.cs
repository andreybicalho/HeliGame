// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class HeliGameTarget : TargetRules
{
	public HeliGameTarget(TargetInfo Target) : base(Target)
	{        
		Type = TargetType.Game;
        ExtraModuleNames.AddRange( new string[] { "HeliGame" } );
	}
}
