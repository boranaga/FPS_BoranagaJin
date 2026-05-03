// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FPS_BoranagaJin : ModuleRules
{
	public FPS_BoranagaJin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
            "EngineCameras", //Added for CameraShake
			"Niagara",        //Added for the use of ParticleSystem
            "AnimGraphRuntime", //Added for Animation
            "MotionWarping", // Added for Enemy motion warping
            "PhysicsCore" //Added by Boranaga for Animation
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"FPS_BoranagaJin",
			"FPS_BoranagaJin/Variant_Horror",
			"FPS_BoranagaJin/Variant_Horror/UI",
			"FPS_BoranagaJin/Variant_Shooter",
			"FPS_BoranagaJin/Variant_Shooter/AI",
			"FPS_BoranagaJin/Variant_Shooter/UI",
			"FPS_BoranagaJin/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
