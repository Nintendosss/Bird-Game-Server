#pragma once

int PlayersJumpedFromBus = 0;

namespace Abilities
{
	inline void PlayerRespawnThread(AFortPlayerControllerAthena* DeadPlayerController, FVector DeadPawnLocation)
	{
		Sleep(5000);

		auto RespawnedPawn = Core::InitializePawn(DeadPlayerController, FVector(DeadPawnLocation.X, DeadPawnLocation.Y, DeadPawnLocation.Z + 5000));

		DeadPlayerController->RespawnPlayerAfterDeath();

		auto NewCheatManager = (UFortCheatManager*)((UGameplayStatics*)UGameplayStatics::StaticClass())->STATIC_SpawnObject(UFortCheatManager::StaticClass(), DeadPlayerController);
		DeadPlayerController->CheatManager = NewCheatManager;

		NewCheatManager->RespawnPlayer();
		NewCheatManager->RespawnPlayerServer();
	}

	AFortGameStateAthena* GameState;

	inline std::vector<ABuildingSMActor*> ExistingBuildings;

	inline bool CanBuild(ABuildingSMActor* BuildingActor)
	{
		for (int i = 0; i < ExistingBuildings.size(); i++)
		{
			auto Building = ExistingBuildings[i];
			if (!Building) continue;

			if (Building->K2_GetActorLocation() == BuildingActor->K2_GetActorLocation() && Building->BuildingType == BuildingActor->BuildingType)
			{
				return false;
			}
		}

		ExistingBuildings.push_back(BuildingActor);
		return true;
	}

	inline FGameplayAbilitySpec* FindAbilitySpecFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle)
	{
		auto Specs = AbilitySystemComponent->ActivatableAbilities.Items;

		for (int i = 0; i < Specs.Num(); i++)
		{
			auto& Spec = Specs[i];

			if (Spec.Handle.Handle == Handle.Handle)
			{
				return &Spec;
			}
		}

		return nullptr;
	}
	template <typename Class>
	static FFortItemEntry FindItemInInventory(AFortPlayerControllerAthena* PC, bool& bFound)
	{

		auto ret = FFortItemEntry();

		auto& ItemInstances = PC->WorldInventory->Inventory.ItemInstances;

		bFound = false;

		for (int i = 0; i < ItemInstances.Num(); i++)
		{
			auto ItemInstance = ItemInstances[i];

			if (!ItemInstance)
				continue;

			auto Def = ItemInstance->ItemEntry.ItemDefinition;

			if (Def && Def->IsA(Class::StaticClass()))
			{
				bFound = true;
				ret = ItemInstance->ItemEntry;
			}
		}

		return ret;
	}
	inline void InternalServerTryActiveAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, FPredictionKey PredictionKey, FGameplayEventData* TriggerEventData)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilitySystemComponent, Handle);
		if (!Spec)
		{
			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
			return;
		}

		Spec->InputPressed = true;

		UGameplayAbility* InstancedAbility;
		if (!Core::InternalTryActivateAbility(AbilitySystemComponent, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
		{
			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
			Spec->InputPressed = false;

			Core::MarkAbilitySpecDirty(AbilitySystemComponent, *Spec);
		}
	}
	inline UFortEngine* GetFortEngine()
	{
		static auto FortEngine = UObject::FindObject<UFortEngine>("FortEngine_");
		return FortEngine;
	}
	inline UWorld* GetWorld()
	{
		return GetFortEngine()->GameViewport->World;
	}
	inline UGameplayStatics* GetGameplayStatics()
	{
		return reinterpret_cast<UGameplayStatics*>(UGameplayStatics::StaticClass());
	}
	constexpr auto PI = 3.1415926535897932f;

	static void sinCos(float* ScalarSin, float* ScalarCos, float Value)
	{
		float quotient = (0.31830988618f * 0.5f) * Value;
		if (Value >= 0.0f)
		{
			quotient = (float)((int)(quotient + 0.5f));
		}
		else
		{
			quotient = (float)((int)(quotient - 0.5f));
		}
		float y = Value - (2.0f * PI) * quotient;

		float sign;
		if (y > 1.57079632679f)
		{
			y = PI - y;
			sign = -1.0f;
		}
		else if (y < -1.57079632679f)
		{
			y = -PI - y;
			sign = -1.0f;
		}
		else
		{
			sign = +1.0f;
		}

		float y2 = y * y;

		*ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

		float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
		*ScalarCos = sign * p;
	}
	static auto RotToQuat(FRotator Rotator)
	{
		const float DEG_TO_RAD = PI / (180.f);
		const float DIVIDE_BY_2 = DEG_TO_RAD / 2.f;
		float SP, SY, SR;
		float CP, CY, CR;

		sinCos(&SP, &CP, Rotator.Pitch * DIVIDE_BY_2);
		sinCos(&SY, &CY, Rotator.Yaw * DIVIDE_BY_2);
		sinCos(&SR, &CR, Rotator.Roll * DIVIDE_BY_2);

		FQuat RotationQuat;
		RotationQuat.X = CR * SP * SY - SR * CP * CY;
		RotationQuat.Y = -CR * SP * CY - SR * CP * SY;
		RotationQuat.Z = CR * CP * SY - SR * SP * CY;
		RotationQuat.W = CR * CP * CY + SR * SP * SY;

		return RotationQuat;
	}
	static AActor* SpawnActorTrans(UClass* StaticClass, FTransform SpawnTransform, AActor* Owner = nullptr, ESpawnActorCollisionHandlingMethod Flags = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		AActor* FirstActor = GetGameplayStatics()->STATIC_BeginDeferredActorSpawnFromClass(GetWorld(), StaticClass, SpawnTransform, Flags, Owner);

		if (FirstActor)
		{
			AActor* FinalActor = GetGameplayStatics()->STATIC_FinishSpawningActor(FirstActor, SpawnTransform);

			if (FinalActor)
			{
				return FinalActor;
			}
		}

		return nullptr;
	}

	inline AActor* SpawnActorByClass(UClass* ActorClass, FVector Location = { 0.0f, 0.0f, 0.0f }, FRotator Rotation = { 0, 0, 0 }, AActor* Owner = nullptr, ESpawnActorCollisionHandlingMethod Flags = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		FTransform SpawnTransform;

		SpawnTransform.Translation = Location;
		SpawnTransform.Scale3D = FVector{ 1, 1, 1 };
		SpawnTransform.Rotation = RotToQuat(Rotation);

		return SpawnActorTrans(ActorClass, SpawnTransform, Owner, Flags);
	}
	inline UFortWorldItem* GetInstanceFromGuid(AController* PC, const FGuid& ToFindGuid)
	{
		auto& ItemInstances = ((AFortPlayerController*)PC)->WorldInventory->Inventory.ItemInstances;
		for (int j = 0; j < ItemInstances.Num(); j++)
		{
			auto ItemInstance = ItemInstances[j];

			if (!ItemInstance)
				continue;

			auto Guid = ItemInstance->ItemEntry.ItemGuid;

			if (ToFindGuid == Guid)
			{
				return ItemInstance;
			}
		}

		return nullptr;
	}

	inline AFortWeapon* EquipWeaponDefinition(AFortPlayerControllerAthena* PlayerController, UFortWeaponItemDefinition* Definition, FGuid Guid, bool bEquipWithMaxAmmo = false)
	{
		if (!PlayerController) return 0;
		if (!Definition) return 0;

		auto Pawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;
		if (!Pawn) return 0;

		auto Instance = GetInstanceFromGuid(PlayerController, Guid);
		if (!Instance) return 0;

		auto WeaponClass = Definition->GetWeaponActorClass();
		if (!WeaponClass) return 0;

		AFortWeapon* Weapon;

		if (WeaponClass->GetFullName() == "BlueprintGeneratedClass TrapTool.TrapTool_C")
		{
			Weapon = (AFortWeapon*)Core::SpawnActor(WeaponClass, {}, PlayerController);
			if (!Weapon) return 0;

			Weapon->bReplicates = true;
			Weapon->bOnlyRelevantToOwner = false;
			((AFortTrapTool*)Weapon)->ItemDefinition = Definition;
		}
		else
		{
			Weapon = Pawn->EquipWeaponDefinition(Definition, Guid);
			if (!Weapon) return 0;

			if (bEquipWithMaxAmmo)
				Weapon->AmmoCount = Weapon->GetBulletsPerClip();

			Instance->ItemEntry.LoadedAmmo = Weapon->AmmoCount;
			Weapon->OnRep_AmmoCount(Instance->ItemEntry.LoadedAmmo);
		}

		Weapon->WeaponData = Definition;
		Weapon->ItemEntryGuid = Guid;
		Weapon->Owner = Pawn;
		Weapon->SetOwner(Pawn);
		Weapon->OnRep_ReplicatedWeaponData();
		Weapon->ClientGivenTo(Pawn);
		Pawn->ClientInternalEquipWeapon(Weapon);
		Pawn->OnRep_CurrentWeapon();

		return Weapon;
	}
	void Initialize(UObject* Object, UFunction* Function, void* Parameters, std::string ObjectName, std::string FunctionName)
	{
		if (!Core::bStartedMatch || !Core::GetWorld()->NetDriver) return;

		if (strstr(FunctionName.c_str(), "ServerTryActivateAbility"))
		{
			auto AbilitySystemComponent = (UAbilitySystemComponent*)Object;

			if (strstr(FunctionName.c_str(), "WithEventData"))
			{
				auto Params = (UAbilitySystemComponent_ServerTryActivateAbilityWithEventData_Params*)Parameters;
				InternalServerTryActiveAbility(AbilitySystemComponent, Params->AbilityToActivate, Params->InputPressed, Params->PredictionKey, &Params->TriggerEventData);
			}
			else
			{
				auto Params = (UAbilitySystemComponent_ServerTryActivateAbility_Params*)Parameters;
				InternalServerTryActiveAbility(AbilitySystemComponent, Params->AbilityToActivate, Params->InputPressed, Params->PredictionKey, nullptr);
			}
		}
		else if (strstr(FunctionName.c_str(), "ServerAbilityRPCBatch"))
		{
			auto AbilitySystemComponent = (UAbilitySystemComponent*)Object;
			auto Params = (UAbilitySystemComponent_ServerAbilityRPCBatch_Params*)Parameters;

			InternalServerTryActiveAbility(AbilitySystemComponent, Params->BatchInfo.AbilitySpecHandle, Params->BatchInfo.InputPressed, Params->BatchInfo.PredictionKey, nullptr);
		}
		else if (strstr(FunctionName.c_str(), "ServerReviveFromDBNO"))
		{
			auto Pawn = (AFortPlayerPawn*)Object;
			auto Params = (AFortPlayerPawn_ServerReviveFromDBNO_Params*)Parameters;

			Pawn->bIsDBNO = false;
			Pawn->OnRep_IsDBNO();
			Pawn->SetHealth(Pawn->GetMaxHealth());

			((AFortPlayerControllerAthena*)Pawn->Controller)->ClientOnPawnRevived(Params->EventInstigator);
		}
		else if (strstr(FunctionName.c_str(), "ClientOnPawnDied"))
		{
			auto DeadPlayerController = (AFortPlayerControllerAthena*)Object;
			auto DeadPawn = (AFortPlayerPawnAthena*)DeadPlayerController->Pawn;
			auto DeadPawnLocation = DeadPawn->K2_GetActorLocation();
			auto DeadPlayerState = (AFortPlayerStateAthena*)DeadPlayerController->PlayerState;
			auto Params = (AFortPlayerControllerZone_ClientOnPawnDied_Params*)Parameters;

			auto KillerPawn = Params->DeathReport.KillerPawn;
			auto KillerPlayerState = (AFortPlayerStateAthena*)Params->DeathReport.KillerPlayerState;

			bool bSuicided = !KillerPawn || !KillerPlayerState || KillerPawn == DeadPawn;

			if (bSuicided)
			{
				FDeathInfo DeathInfo = { DeadPlayerState, false, EDeathCause::FallDamage };

				DeathInfo.Distance = 6900; //69 meters
				DeathInfo.DeathLocation = DeadPawnLocation;

				DeadPlayerState->ClientReportKill(DeadPlayerState);

				DeadPlayerState->DeathInfo = DeathInfo;
				DeadPlayerState->OnRep_DeathInfo();

				DeadPlayerController->ClientReceiveKillNotification(DeadPlayerState, DeadPlayerState);
			}
			else
			{
				FDeathInfo DeathInfo = { KillerPlayerState, false, EDeathCause::Banhammer };

				DeathInfo.Distance = KillerPawn->GetDistanceTo(DeadPawn);
				DeathInfo.DeathLocation = DeadPawnLocation;

				KillerPlayerState->ClientReportKill(DeadPlayerState);

				DeadPlayerState->DeathInfo = DeathInfo;
				DeadPlayerState->OnRep_DeathInfo();

				((AFortPlayerControllerAthena*)KillerPawn->Controller)->ClientReceiveKillNotification(KillerPlayerState, DeadPlayerState);

				KillerPlayerState->KillScore++;
				KillerPlayerState->TeamKillScore++;
				KillerPlayerState->OnRep_Kills();
			}

			if (DeadPlayerController->QuickBars && DeadPlayerController->WorldInventory)
			{
				auto ItemInstances = DeadPlayerController->WorldInventory->Inventory.ItemInstances;

				for (int i = 0; i < ItemInstances.Num(); i++)
				{
					auto ItemInstance = ItemInstances[i];
					if (!ItemInstance) continue;

					auto Weapon = Inventory::GetWeaponFromGuid(DeadPlayerController, ItemInstance->ItemEntry.ItemGuid);
					if (!Weapon) continue;

					ItemInstance->ItemEntry.LoadedAmmo = Weapon->GetBulletsPerClip();
				}
			}

			std::thread(PlayerRespawnThread, DeadPlayerController, DeadPawnLocation).detach();
		}
		else if (strstr(FunctionName.c_str(), "ServerHandlePickup"))
		{
			Inventory::ServerHandlePickup((AFortPlayerPawn*)Object, Parameters);
		}
		else if (strstr(FunctionName.c_str(), "ServerAttemptInventoryDrop"))
		{
			Inventory::ServerAttemptInventoryDrop((AFortPlayerControllerAthena*)Object, Parameters);
		}
		else if (strstr(FunctionName.c_str(), "ServerExecuteInventoryItem"))
		{
			Inventory::ServerExecuteInventoryItem((AFortPlayerControllerAthena*)Object, *(FGuid*)Parameters);
		}
		else if (strstr(FunctionName.c_str(), "ServerAttemptAircraftJump"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			auto Params = (AFortPlayerControllerAthena_ServerAttemptAircraftJump_Params*)Parameters;
			auto GameState = (AFortGameStateAthena*)Core::GetWorld()->AuthorityGameMode->GameState;

			auto Aircraft = ((AFortGameStateAthena*)Core::GetWorld()->GameState)->GetAircraft(0);
			if (!Aircraft) return;

			Core::InitializePawn(PlayerController, Aircraft->K2_GetActorLocation());
			PlayersJumpedFromBus++;


			Aircraft->PlayEffectsForPlayerJumped();

			GameState->SafeZonePhase = false;


		}
		else if (strstr(FunctionName.c_str(), "OnAircraftExitedDropZone"))
		{
			auto Connections = Core::GetWorld()->NetDriver->ClientConnections;

			for (int i = 0; i < Connections.Num(); i++)
			{
				auto PlayerController = (AFortPlayerControllerAthena*)Connections[i]->PlayerController;
				if (!PlayerController || PlayerController->PlayerState->bIsSpectator)
					continue;

				if (PlayerController->IsInAircraft())
					PlayerController->ServerAttemptAircraftJump(PlayerController->GetControlRotation());
			}
		}
		else if (strstr(FunctionName.c_str(), "BuildingActor.OnDeathServer"))
		{
			auto Actor = (ABuildingActor*)Object;
			if (!Actor) return;

			for (int i = 0; i < ExistingBuildings.size(); i++)
			{
				auto Building = ExistingBuildings[i];
				if (!Building) continue;

				if (Building == Actor)
				{
					ExistingBuildings.erase(ExistingBuildings.begin() + i);
					return;
				}
			}
		}
		else if (strstr(FunctionName.c_str(), "ServerCreateBuildingActor"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			auto Params = (AFortPlayerController_ServerCreateBuildingActor_Params*)Parameters;
			auto CurrentBuildClass = Params->BuildingClassData.BuildingClass;

			auto GameState = (AFortGameStateAthena*)Core::GetWorld()->GameState;

			auto BuildingActor = (ABuildingSMActor*)Core::SpawnActor(Params->BuildingClassData.BuildingClass, Params->BuildLoc, PlayerController, Math::RotToQuat(Params->BuildRot), ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (!BuildingActor) return;

			if (CanBuild(BuildingActor))
			{
				BuildingActor->DynamicBuildingPlacementType = EDynamicBuildingPlacementType::DestroyAnythingThatCollides;
				BuildingActor->SetMirrored(Params->bMirrored);
				BuildingActor->InitializeKismetSpawnedBuildingActor(BuildingActor, PlayerController, 0);
				Inventory::RemoveBuildingMaterials(PlayerController, CurrentBuildClass);
				BuildingActor->Team = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
				return;
			}

			//BuildingActor->K2_DestroyActor();
			BuildingActor->SetActorHiddenInGame(true);
			BuildingActor->SilentDie();
		}
		else if (strstr(FunctionName.c_str(), "ServerRepairBuildingActor"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			auto Params = (AFortPlayerController_ServerRepairBuildingActor_Params*)Parameters;

			if (!Params->BuildingActorToRepair) return;

			Params->BuildingActorToRepair->RepairBuilding(PlayerController, 20); //todo: not hardcode it here
		}


		else if (strstr(FunctionName.c_str(), "ServerAttemptInteract"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			auto Params = (AFortPlayerController_ServerAttemptInteract_Params*)Parameters;
			auto ReceivingActor = Params->ReceivingActor;
			if (!ReceivingActor) return;

			if (ReceivingActor->IsA(APlayerPawn_Athena_C::StaticClass()))
			{
				((APlayerPawn_Athena_C*)ReceivingActor)->ReviveFromDBNO(PlayerController);
			}
			else if (ReceivingActor->IsA(ABuildingContainer::StaticClass())/*strstr(ReceivingActorName.c_str(), "Tiered_Chest_Athena") || strstr(ReceivingActorName.c_str(), "Tiered_Ammo_Athena")*/)
			{
				((ABuildingContainer*)ReceivingActor)->bAlreadySearched = true;
				((ABuildingContainer*)ReceivingActor)->OnRep_bAlreadySearched();


				auto ReceivingActorName = ReceivingActor->GetName();
				std::cout << "ServerAttemptInteract Container: " << ReceivingActorName << "\n";

				if (strstr(ReceivingActorName.c_str(), "Chest"))
				{
					std::cout << "Player opened a chest" << "\n";

					auto ContainerLocation = ReceivingActor->K2_GetActorLocation();

					auto Weapon = Inventory::GetRandomWeapon();
					auto Ammo = Weapon->GetAmmoWorldItemDefinition_BP();
					auto Consumable = Inventory::GetRandomConsumable();

					Core::SummonPickup(Weapon, 1, ContainerLocation, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Chest);
					Core::SummonPickup(Ammo, Ammo->DropCount, ContainerLocation, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Chest);
					Core::SummonPickup(Consumable, 1, ContainerLocation, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Chest);

					//todo: add materials
				}
				else if (strstr(ReceivingActorName.c_str(), "Ammo"))
				{
					std::cout << "Player opened an ammobox" << "\n";

					auto ContainerLocation = ReceivingActor->K2_GetActorLocation();

					auto Ammo = Inventory::GetRandomAmmo();

					Core::SummonPickup(Ammo, Ammo->DropCount, ContainerLocation, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::AmmoBox);
				}
			}

			return;
		}

		else if (strstr(FunctionName.c_str(), "ServerBeginEditingBuildingActor"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			bool bFound = false;
			auto Pawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;
			auto Params = (AFortPlayerController_ServerBeginEditingBuildingActor_Params*)Parameters;
			auto EditToolEntry = FindItemInInventory<UFortEditToolItemDefinition>(PlayerController, bFound);
			if (PlayerController && Pawn && Params->BuildingActorToEdit && bFound)
			{
				auto EditTool = (AFortWeap_EditingTool*)EquipWeaponDefinition(PlayerController, (UFortWeaponItemDefinition*)EditToolEntry.ItemDefinition, EditToolEntry.ItemGuid);

				if (EditTool)
				{
					EditTool->EditActor = Params->BuildingActorToEdit;
					EditTool->OnRep_EditActor();
					Params->BuildingActorToEdit->EditingPlayer = (AFortPlayerStateZone*)PlayerController->PlayerState;
					Params->BuildingActorToEdit->OnRep_EditingPlayer();
					return;

				}
			}
		}
		else if (strstr(FunctionName.c_str(), "ServerEditBuildingActor"))
		{
			auto Params = (AFortPlayerController_ServerEditBuildingActor_Params*)Parameters;
			auto PC = (AFortPlayerControllerAthena*)Object;

			if (PC && Params)
			{
				auto BuildingActor = Params->BuildingActorToEdit;
				auto NewBuildingClass = Params->NewBuildingClass;
				auto RotIterations = Params->RotationIterations;

				if (BuildingActor && NewBuildingClass)
				{
					auto location = BuildingActor->K2_GetActorLocation();
					auto rotation = BuildingActor->K2_GetActorRotation();

					if (BuildingActor->BuildingType != EFortBuildingType::Wall)
					{
						int Yaw = (int(rotation.Yaw) + 360) % 360;

						if (Yaw > 80 && Yaw < 100) // 90
						{
							if (RotIterations == 1)
								location = location + FVector(-256, 256, 0);
							else if (RotIterations == 2)
								location = location + FVector(-512, 0, 0);
							else if (RotIterations == 3)
								location = location + FVector(-256, -256, 0);
						}
						else if (Yaw > 170 && Yaw < 190) // 180
						{
							if (RotIterations == 1)
								location = location + FVector(-256, -256, 0);
							else if (RotIterations == 2)
								location = location + FVector(0, -512, 0);
							else if (RotIterations == 3)
								location = location + FVector(256, -256, 0);
						}
						else if (Yaw > 260 && Yaw < 280) // 270
						{
							if (RotIterations == 1)
								location = location + FVector(256, -256, 0);
							else if (RotIterations == 2)
								location = location + FVector(512, 0, 0);
							else if (RotIterations == 3)
								location = location + FVector(256, 256, 0);
						}
						else // 0 - 360
						{
							if (RotIterations == 1)
								location = location + FVector(256, 256, 0);
							else if (RotIterations == 2)
								location = location + FVector(0, 512, 0);
							else if (RotIterations == 3)
								location = location + FVector(-256, 256, 0);
						}
					}

					rotation.Yaw += 90 * RotIterations;

					auto bWasInitiallyBuilding = BuildingActor->bIsInitiallyBuilding;
					auto HealthPercent = BuildingActor->GetHealthPercent();
					auto old_Team = BuildingActor->Team;

					BuildingActor->SilentDie();

					if (auto NewBuildingActor = (ABuildingSMActor*)SpawnActorByClass(NewBuildingClass, location, rotation, PC))
					{
						if (!bWasInitiallyBuilding)
							NewBuildingActor->ForceBuildingHealth(NewBuildingActor->GetMaxHealth() * HealthPercent);

						NewBuildingActor->SetMirrored(Params->bMirrored);
						NewBuildingActor->InitializeKismetSpawnedBuildingActor(NewBuildingActor, PC, 0);
						NewBuildingActor->Team = old_Team; //set team for build
						return;
					}
				}
			}
		}
		else if (strstr(FunctionName.c_str(), "ServerEndEditingBuildingActor"))
		{
			auto Params = (AFortPlayerController_ServerEndEditingBuildingActor_Params*)Parameters;
			auto PC = (AFortPlayerControllerAthena*)Object;

			if (!PC->IsInAircraft() && Params->BuildingActorToStopEditing)
			{
				Params->BuildingActorToStopEditing->EditingPlayer = nullptr;
				Params->BuildingActorToStopEditing->OnRep_EditingPlayer();

				auto EditTool = (AFortWeap_EditingTool*)((APlayerPawn_Athena_C*)PC->Pawn)->CurrentWeapon;

				if (EditTool)
				{
					EditTool->bEditConfirmed = true;
					EditTool->EditActor = nullptr;
					EditTool->OnRep_EditActor();
					return;
				}
			}
		}
		else if (strstr(FunctionName.c_str(), "ServerPlayEmoteItem"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			auto Pawn = (APlayerPawn_Athena_C*)PlayerController->Pawn;
			auto CurrentParams = (AFortPlayerController_ServerPlayEmoteItem_Params*)Parameters;

			if (Pawn)
			{
				auto EmoteItem = CurrentParams->EmoteAsset;
				auto Animation = EmoteItem->GetAnimationHardReference(EFortCustomBodyType::All, EFortCustomGender::Both);

				if (Pawn->RepAnimMontageInfo.AnimMontage != Animation)
				{
					Pawn->RepAnimMontageInfo.AnimMontage = Animation;
					Pawn->RepAnimMontageInfo.PlayRate = 1;
					Pawn->RepAnimMontageInfo.IsStopped = false;
					Pawn->RepAnimMontageInfo.SkipPositionCorrection = true;

					Pawn->PlayAnimMontage(Animation, 1, FName());
					Pawn->OnRep_AttachmentMesh();
					Pawn->OnRep_AttachmentReplication();
					Pawn->OnRep_ReplicatedAnimMontage();
					//Pawn->Mesh->GetAnimInstance()->Montage_Play(Animation, 1, EMontagePlayReturnType::Duration, 0, true);
				}
			}
		}




		//end of thing
	}
}
