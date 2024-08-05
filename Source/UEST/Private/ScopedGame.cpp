#include "ScopedGame.h"

#include "Engine/PackageMapClient.h"
#include "EngineUtils.h"
#include "GameMapsSettings.h"
#include "Iris/ReplicationSystem/ObjectReplicationBridge.h"
#include "Iris/ReplicationSystem/ReplicationSystem.h"
#include "Net/OnlineEngineInterface.h"

struct TGWorldGuard final : FNoncopyable
{
	TGWorldGuard()
		: OldWorld{GWorld}
	{
	}

	~TGWorldGuard()
	{
		GWorld = OldWorld;
	}

	UWorld* OldWorld;
};

struct FCVarGuard final : FNoncopyable
{
	explicit FCVarGuard(IConsoleVariable* Variable, const TCHAR* NewValue)
		: Variable(Variable)
		, OldValue(Variable ? Variable->GetString() : TEXT(""))
	{
		if (ensureAlways(Variable))
		{
			Variable->Set(NewValue);
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

static TUniquePtr<FCVarsGuard> CVarsGuard;

FScopedGameInstance::FScopedGameInstance(TSubclassOf<UGameInstance> GameInstanceClass, const bool bGarbageCollectOnDestroy)
	: GameInstanceClass{MoveTemp(GameInstanceClass)}
	, LastInstanceId{0}
	, bGarbageCollectOnDestroy{bGarbageCollectOnDestroy}
{
	if (NumScopedGames == 0)
	{
		CVarsGuard = MakeUnique<FCVarsGuard>();

		// Client runs DNS lookup in separate thread without any way to wait for it (except sleeping real time)
		// So just disable it for now because we know that we connect via IP address
		CVarsGuard->ConsoleVariableGuards.Emplace(IConsoleManager::Get().FindConsoleVariable(TEXT("net.IpConnectionDisableResolution")),TEXT("1"));
	}

	++NumScopedGames;
}

FScopedGameInstance::FScopedGameInstance(FScopedGameInstance&& Other)
	: GameInstanceClass{MoveTemp(Other.GameInstanceClass)}
	, Games{MoveTemp(Other.Games)}
	, LastInstanceId{Other.LastInstanceId}
	, bGarbageCollectOnDestroy{Other.bGarbageCollectOnDestroy}
{
	++NumScopedGames;
}

FScopedGameInstance::~FScopedGameInstance()
{
	for (const auto& Game : Games)
	{
		DestroyGameInternal(*Game);
	}

	const bool bCollectGarbage = bGarbageCollectOnDestroy && !Games.IsEmpty();
	Games.Empty();

	if (bCollectGarbage)
	{
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		GEngine->CheckAndHandleStaleWorldObjectReferences();
	}

	--NumScopedGames;

	if (NumScopedGames == 0)
	{
		CVarsGuard.Reset();
	}
}

UGameInstance* FScopedGameInstance::CreateGame(const EScopedGameType Type, const FString& MapToLoad, const bool bWaitForConnect)
{
	auto Game = NewObject<UGameInstance>(GEngine, GameInstanceClass);
	Games.Emplace(Game);
	Game->InitializeStandalone();

	auto* WorldContext = Game->GetWorldContext();
	WorldContext->PIEInstance = ++LastInstanceId;
	WorldContext->PIEPrefix = UWorld::BuildPIEPackagePrefix(WorldContext->PIEInstance);
	WorldContext->RunAsDedicated = Type == EScopedGameType::Server;

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

		TGWorldGuard GWorldGuard;

		FString Error;
		const auto BrowseResult = Game->GetEngine()->Browse(*WorldContext, URL, Error);
		if (BrowseResult == EBrowseReturnVal::Pending)
		{
			if (bWaitForConnect && !TickUntil([&]
			{
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
			}))
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
	const auto OnlineSubsystemId = UOnlineEngineInterface::Get()->GetOnlineIdentifier(*Game.GetWorldContext());
	const auto World = Game.GetWorld();
	World->BeginTearingDown();

	// This is an equivalent of UEngine::CleanupGameViewport, but for a single GameInstance
	{
		Game.CleanupGameViewport();
		if (auto* GameViewport = Game.GetGameViewportClient())
		{
			GameViewport->DetachViewportClient();
		}
	}

	for (FActorIterator ActorIt(World); ActorIt; ++ActorIt)
	{
		ActorIt->RouteEndPlay(EEndPlayReason::Quit);
	}

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
		const TGWorldGuard GWorldGuard;
		const TGuardValue GPlayInEditorInstanceIDGuard(GPlayInEditorID, Game->GetWorldContext()->PIEInstance);

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
}

FScopedGameInstance FScopedGame::Create() const
{
	return FScopedGameInstance{GameInstanceClass, bGarbageCollectOnDestroy};
}
