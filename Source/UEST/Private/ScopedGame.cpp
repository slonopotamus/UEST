#include "ScopedGame.h"
#include "Engine/PackageMapClient.h"
#include "EngineUtils.h"
#include "GameMapsSettings.h"
#include "Iris/ReplicationSystem/ObjectReplicationBridge.h"
#include "Iris/ReplicationSystem/ReplicationSystem.h"
#include "Net/OnlineEngineInterface.h"
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3)
#include "Runtime/Core/Internal/Misc/PlayInEditorLoadingScope.h"
#endif
#include "UESTGameInstance.h"

struct FGWorldGuard final : FNoncopyable
{
	FGWorldGuard()
	    : OldWorld{GWorld}
	{
	}

	~FGWorldGuard()
	{
		GWorld = OldWorld;
	}

	UWorld* OldWorld;
};

struct FCVarGuard final : FNoncopyable
{
	explicit FCVarGuard(const FString& CVarName, const FScopedGameInstance::FCVarConfig& CVarConfig)
	    : Variable{IConsoleManager::Get().FindConsoleVariable(*CVarName)}
	    , OldValue{Variable ? Variable->GetString() : TEXT("")}
	{
		if (Variable || (CVarConfig.bReportNonexistentVariable && ensureAlwaysMsgf(Variable, TEXT("Console variable not found: %s"), *CVarName)))
		{
			Variable->Set(*CVarConfig.Value);
		}
	}

	~FCVarGuard()
	{
		if (Variable)
		{
			Variable->Set(*OldValue);
		}
	}

private:
	IConsoleVariable* Variable;
	const FString OldValue;
};

struct FCVarsGuard final : FNoncopyable
{
	TArray<FCVarGuard> ConsoleVariableGuards;
};

static int32 NumScopedGames = 0;

// We are limited by MAX_PIE_INSTANCES :(
static TArray<int32> FreePIEInstances{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static TUniquePtr<FCVarsGuard> CVarsGuard;

FScopedGameInstance::FScopedGameInstance(TSubclassOf<UGameInstance> GameInstanceClass, const TMap<FString, FCVarConfig>& CVars)
    : GameInstanceClass{MoveTemp(GameInstanceClass)}
{
	if (NumScopedGames == 0)
	{
		CVarsGuard = MakeUnique<FCVarsGuard>();

		for (const auto& [CVarName, CVarConfig] : CVars)
		{
			CVarsGuard->ConsoleVariableGuards.Emplace(CVarName, CVarConfig);
		}
	}

	++NumScopedGames;
}

FScopedGameInstance::FScopedGameInstance(FScopedGameInstance&& Other)
    : GameInstanceClass{MoveTemp(Other.GameInstanceClass)}
    , Games{MoveTemp(Other.Games)}
{
	++NumScopedGames;
}

FScopedGameInstance::~FScopedGameInstance()
{
	for (const auto& Game : Games)
	{
		DestroyGameInternal(*Game);
	}

	const bool bCollectGarbage = !Games.IsEmpty();
	Games.Empty();

	if (bCollectGarbage)
	{
		CollectGarbage();
	}

	--NumScopedGames;

	if (NumScopedGames == 0)
	{
		CVarsGuard.Reset();
	}
}

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3)
struct FGPlayInEditorIDGuard final : UE::Core::Private::FPlayInEditorLoadingScope
{
	explicit FGPlayInEditorIDGuard(const int32 PlayInEditorID)
	    : FPlayInEditorLoadingScope(PlayInEditorID)
	{
	}
};
#else
struct FGPlayInEditorIDGuard final : TGuardValue<decltype(GPlayInEditorID), int32>
{
	explicit FGPlayInEditorIDGuard(const int32 PlayInEditorID)
	    : TGuardValue(GPlayInEditorID, PlayInEditorID)
	{
	}
};
#endif

UGameInstance* FScopedGameInstance::CreateGame(const EScopedGameType Type, FString MapToLoad, const bool bWaitForConnect)
{
	if (!ensureAlwaysMsgf(!FreePIEInstances.IsEmpty(), TEXT("Attempt to create too many games at the same time!")))
	{
		return nullptr;
	}

	if (MapToLoad.IsEmpty())
	{
		MapToLoad = GetDefault<UGameMapsSettings>()->GetGameDefaultMap();
	}

	auto* Game = NewObject<UGameInstance>(GEngine, GameInstanceClass);
	if (!ensureAlwaysMsgf(Game, TEXT("Failed to create game instance")))
	{
		return nullptr;
	}

	Games.Emplace(Game);

	int32 PIEInstance = FreePIEInstances.Pop();

	const auto bRunAsDedicated = Type == EScopedGameType::Server;
	if (auto* TestableGame = Cast<IUESTGameInstance>(Game))
	{
		TestableGame->InitializeForTests(bRunAsDedicated, PIEInstance);
	}
	else
	{
		IUESTGameInstance::DefaultInitializeForTests(*Game, bRunAsDedicated, PIEInstance);
	}

	auto* WorldContext = Game->GetWorldContext();
	if (!ensureAlways(WorldContext))
	{
		DestroyGame(Game);
		return nullptr;
	}

	ensureAlwaysMsgf(WorldContext->PIEInstance == PIEInstance, TEXT("WorldContext must use supplied PIEInstance"));

	if (Type == EScopedGameType::Client)
	{
		WorldContext->GameViewport = NewObject<UGameViewportClient>(Game->GetEngine(), Game->GetEngine()->GameViewportClientClass);
		WorldContext->GameViewport->Init(*WorldContext, Game, false);

		if (FString Error; !ensureAlwaysMsgf(WorldContext->GameViewport->SetupInitialLocalPlayer(Error) && Error.IsEmpty(), TEXT("Failed to create local player: %s"), *Error))
		{
			DestroyGame(Game);
			return nullptr;
		}
	}

	if (!MapToLoad.IsEmpty())
	{
		FURL URL(nullptr, *MapToLoad, TRAVEL_Absolute);

		if (Type == EScopedGameType::Server)
		{
			URL.AddOption(TEXT("listen"));
		}

		const TGuardValue GIsPlayInEditorWorldGuard(GIsPlayInEditorWorld, false);
		const FGPlayInEditorIDGuard GPlayInEditorIDGuard(Game->GetWorldContext()->PIEInstance);
		const FGWorldGuard GWorldGuard;

		FString Error;
		const auto BrowseResult = Game->GetEngine()->Browse(*WorldContext, URL, Error);
		if (BrowseResult == EBrowseReturnVal::Pending)
		{
			const auto WaitForConnect = [&] {
				if (Game->GetWorldContext()->PendingNetGame)
				{
					return false;
				}

				const auto* ClientWorld = Game->GetWorld();

				const auto* NetDriver = ClientWorld->GetNetDriver();
				if (!NetDriver)
				{
					return false;
				}

				if (!NetDriver->ServerConnection)
				{
					return false;
				}

				if (NetDriver->ServerConnection->URL != URL)
				{
					return false;
				}

				if (!ClientWorld->GetGameState())
				{
					return false;
				}

				const auto* ClientPC = ClientWorld->GetFirstPlayerController();
				if (!ClientPC)
				{
					return false;
				}

				if (!ClientPC->PlayerState)
				{
					return false;
				}

				return true;
			};

			if (bWaitForConnect && !TickUntil(WaitForConnect))
			{
				ensureAlwaysMsgf(false, TEXT("Timeout connecting to dedicated server"));
				DestroyGame(Game);
				return nullptr;
			}
		}
		else if (!ensureAlways(BrowseResult == EBrowseReturnVal::Success))
		{
			DestroyGame(Game);
			return nullptr;
		}
	}

	return Game;
}

UGameInstance* FScopedGameInstance::CreateClientFor(const UGameInstance* Server, const bool bWaitForConnect)
{
	if (!ensure(Server))
	{
		return nullptr;
	}

	return CreateGame(EScopedGameType::Client, FString::Printf(TEXT("127.0.0.1:%d"), Server->GetWorld()->URL.Port), bWaitForConnect);
}

void FScopedGameInstance::DestroyGameInternal(UGameInstance& Game)
{
	if (const auto* WorldContext = Game.GetWorldContext())
	{
		FreePIEInstances.Push(WorldContext->PIEInstance);
	}

	const auto OnlineSubsystemId = UOnlineEngineInterface::Get()->GetOnlineIdentifier(*Game.GetWorldContext());
	const auto World = Game.GetWorld();

	// This is an equivalent of UEngine::CleanupGameViewport, but for a single GameInstance
	{
		Game.CleanupGameViewport();
		if (auto* GameViewport = Game.GetGameViewportClient())
		{
			GameViewport->DetachViewportClient();
		}
	}

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
	World->EndPlay(EEndPlayReason::EndPlayInEditor);
#else
	World->BeginTearingDown();

	for (FActorIterator ActorIt(World); ActorIt; ++ActorIt)
	{
		ActorIt->RouteEndPlay(EEndPlayReason::EndPlayInEditor);
	}
#endif

	Game.GetEngine()->ShutdownWorldNetDriver(World);

	Game.Shutdown();
	World->DestroyWorld(true);
	GEngine->DestroyWorldContext(World);

	if (auto* OnlineEngineInterface = UOnlineEngineInterface::Get())
	{
		if (OnlineEngineInterface->DoesInstanceExist(OnlineSubsystemId))
		{
			OnlineEngineInterface->DestroyOnlineSubsystem(OnlineSubsystemId);
		}
	}
}

bool FScopedGameInstance::DestroyGame(UGameInstance* Game)
{
	if (!Game)
	{
		return false;
	}

	for (int32 Index = 0; Index < Games.Num(); ++Index)
	{
		if (Games[Index].Get() != Game)
		{
			continue;
		}

		DestroyGameInternal(*Game);
		Games.RemoveAt(Index);

		CollectGarbage();
		return true;
	}

	return ensureMsgf(false, TEXT("Game %s was not registered"), *Game->GetPathName());
}

void FScopedGameInstance::TickInternal(const float DeltaSeconds, const ELevelTick TickType)
{
	// Unfortunately, there are issues with this helper
	// CommandletHelpers::TickEngine(nullptr, CurrentStep);

	++GFrameCounter;
	StaticTick(DeltaSeconds);
	FTSTicker::GetCoreTicker().Tick(DeltaSeconds);

	for (const auto& Game : Games)
	{
		const TGuardValue GIsPlayInEditorWorldGuard(GIsPlayInEditorWorld, false);
		const FGPlayInEditorIDGuard GPlayInEditorIDGuard(Game->GetWorldContext()->PIEInstance);
		const FGWorldGuard GWorldGuard;

		Game->GetEngine()->TickWorldTravel(*Game->GetWorldContext(), DeltaSeconds);
		Game->GetWorld()->Tick(TickType, DeltaSeconds);
	}
}

UObject* FScopedGameInstance::StaticFindReplicatedObjectIn(UObject* Object, const UWorld* World)
{
	if (!ensure(World) || !Object)
	{
		return nullptr;
	}
	if (Object->GetWorld() == World)
	{
		return Object;
	}

	const auto TheirNetDriver = Object->GetWorld()->GetNetDriver();

#if UE_WITH_IRIS
	if (const auto TheirReplicationSystem = TheirNetDriver->GetReplicationSystem())
	{
		const auto TheirObjectBridge = Cast<UObjectReplicationBridge>(TheirReplicationSystem->GetReplicationBridge());
		if (!TheirObjectBridge)
		{
			return nullptr;
		}

		const auto ObjectRefHandle = TheirObjectBridge->GetReplicatedRefHandle(Object);
		if (!ObjectRefHandle.IsValid())
		{
			return nullptr;
		}

		auto* OurNetDriver = World->GetNetDriver();
		if (!OurNetDriver)
		{
			return nullptr;
		}

		const auto WorldReplicationSystem = OurNetDriver->GetReplicationSystem();
		if (!WorldReplicationSystem)
		{
			return nullptr;
		}

		if (const auto OurObjectBridge = Cast<UObjectReplicationBridge>(WorldReplicationSystem->GetReplicationBridge()))
		{
			return OurObjectBridge->GetReplicatedObject(ObjectRefHandle);
		}
	}
	else
#endif
	{
		const auto* OurNetDriver = World->GetNetDriver();
		if (!OurNetDriver)
		{
			return nullptr;
		}

		const FNetworkGUID* GUID = TheirNetDriver->GuidCache->NetGUIDLookup.Find(Object);
		if (!GUID)
		{
			return nullptr;
		}

		const auto* ObjectRef = OurNetDriver->GuidCache->ObjectLookup.Find(*GUID);
		return ObjectRef ? ObjectRef->Object.Get() : nullptr;
	}

	return nullptr;
}

void FScopedGameInstance::CollectGarbage()
{
	// find objects like Textures in the playworld levels that won't get garbage collected as they are marked RF_Standalone
	for (FThreadSafeObjectIterator It; It; ++It)
	{
		UObject* Object = *It;

		if (Object->GetOutermost()->HasAnyPackageFlags(PKG_PlayInEditor))
		{
			if (Object->HasAnyFlags(RF_Standalone))
			{
				// Clear RF_Standalone flag from objects in the levels used for PIE so they get cleaned up.
				Object->ClearFlags(RF_Standalone);
			}
		}
	}

	::CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	GEngine->CheckAndHandleStaleWorldObjectReferences();
}

void FScopedGameInstance::Tick(const float DeltaSeconds, const float StepSeconds, const ELevelTick TickType)
{
	if (!ensureMsgf(StepSeconds > 0, TEXT("Tick step must be positive: %f"), StepSeconds))
	{
		return;
	}

	auto RemainingTickTime = DeltaSeconds;
	while (RemainingTickTime >= 0)
	{
		const float CurrentStep = FMath::Min(RemainingTickTime, StepSeconds);
		TickInternal(CurrentStep, TickType);
		RemainingTickTime -= StepSeconds;
	}
}

bool FScopedGameInstance::TickUntil(const TFunction<bool()>& Condition, const float StepSeconds, const float MaxWaitTime, ELevelTick TickType)
{
	if (!ensureMsgf(StepSeconds > 0, TEXT("Tick step must be positive: %f"), StepSeconds))
	{
		return false;
	}

	auto RemainingTickTime = MaxWaitTime;
	while (RemainingTickTime > 0)
	{
		if (Condition())
		{
			return true;
		}

		const float CurrentStep = FMath::Min(RemainingTickTime, StepSeconds);

		TickInternal(CurrentStep, TickType);

		RemainingTickTime -= StepSeconds;
	}

	return Condition();
}

FScopedGame::FScopedGame()
{
	GameInstanceClass = GetDefault<UGameMapsSettings>()->GameInstanceClass.TryLoadClass<UGameInstance>();
	if (!GameInstanceClass)
	{
		GameInstanceClass = UGameInstance::StaticClass();
	}

	// Client runs DNS lookup in separate thread without any way to wait for it (except sleeping real time)
	// So just disable it for now because we know that we connect via IP address
	WithConsoleVariable(TEXT("net.IpConnectionDisableResolution"), TEXT("1"));
}

FScopedGameInstance FScopedGame::Create() const
{
	return FScopedGameInstance{GameInstanceClass, CVars};
}
