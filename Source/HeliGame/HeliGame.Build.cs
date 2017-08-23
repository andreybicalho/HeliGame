// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class HeliGame : ModuleRules
{
	public HeliGame(ReadOnlyTargetRules Target) : base(Target)
	{
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "OnlineSubsystem", "OnlineSubsystemUtils" });

        //DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
        DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");        
	}
}
