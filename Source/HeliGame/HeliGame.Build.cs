// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

using UnrealBuildTool;

public class HeliGame : ModuleRules
{
	public HeliGame(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        //PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "OnlineSubsystem", "OnlineSubsystemUtils", "Steamworks" });
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "OnlineSubsystem", "OnlineSubsystemUtils", "Steamworks" });

        // Uncomment if using online features
        //PrivateDependencyModuleNames.AddRange(new string[] { });
        //PrivateDependencyModuleNames.Add("OnlineSubsystem");

        //DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
        //DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
    }
}
