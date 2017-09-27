// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

using UnrealBuildTool;

public class HeliGame : ModuleRules
{
	public HeliGame(ReadOnlyTargetRules Target) : base(Target)
	{        
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "OnlineSubsystem", "OnlineSubsystemUtils" });

        //DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
        DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    }
}
