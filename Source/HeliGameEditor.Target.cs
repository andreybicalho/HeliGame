// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class HeliGameEditorTarget : TargetRules
{
	public HeliGameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

        ExtraModuleNames.AddRange( new string[] { "HeliGame" } );

        bUseUnityBuild = false;
        bUsePCHFiles = false;
    }
}
