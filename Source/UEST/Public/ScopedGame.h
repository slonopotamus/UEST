#pragma once

#include "Engine/GameInstance.h"

enum class EScopedGameType : uint8
{
	/**
	 * Server game type represents a dedicated server.
	 * It accepts incoming network connections, has no local players and its NetMode is NM_DedicatedServer.
	 */
	Server,

	/**
	 * Client game type represents a game client.
	 * It initially has one local player (though games can add more) and a GameViewportClient.
	 * Its NetMode can be: NM_Standalone, NM_ListenServer, NM_Client.
	 */
	Client,

	/**
	 * Empty game type is possibly a not very useful type of game that neither listens to network nor has local players.
	 */
	Empty,
};

UENUM()
enum class EScopedGameConnectError : uint8
{
	Success,
	PendingNetGame,
	NoNetDriver,
	NoServerConnection,
	WrongURL,
	NoGameState,
	NoClientPC,
	NoClientPS,
};

class UEST_API FScopedGameInstance : FNoncopyable
{
	TSubclassOf<UGameInstance> GameInstanceClass;

	TArray<TStrongObjectPtr<UGameInstance>> Games;

	static void DestroyGameInternal(UGameInstance& Game);

	void TickInternal(float DeltaSeconds, ELevelTick TickType);

	[[nodiscard]] static UObject* StaticFindReplicatedObjectIn(UObject* Object, const UWorld* World);

	static void CollectGarbage();

	static int32 FindFreePIEInstance();

public:
	static constexpr auto DefaultStepSeconds = 0.1f;

	[[nodiscard]] explicit FScopedGameInstance(TSubclassOf<UGameInstance> GameInstanceClass, const TMap<IConsoleVariable*, FString>& CVars);

	[[nodiscard]] FScopedGameInstance(FScopedGameInstance&& Other);

	virtual ~FScopedGameInstance();

	UGameInstance* CreateGame(EScopedGameType Type = EScopedGameType::Client, FString MapToLoad = TEXT(""), bool bWaitForConnect = true) UE_LIFETIMEBOUND;

	UGameInstance* CreateClientFor(const UGameInstance& Server, bool bWaitForConnect = true) UE_LIFETIMEBOUND;

	bool DestroyGame(UGameInstance* Game);

	/** Advances time in all created games by DeltaSeconds in StepSeconds increments */
	void Tick(float DeltaSeconds, float StepSeconds = DefaultStepSeconds, ELevelTick TickType = LEVELTICK_All);

	/** Advances time in all created games in StepSeconds increments until Condition returns true */
	[[nodiscard]] bool TickUntil(const TFunctionRef<bool()>& Condition, float StepSeconds = DefaultStepSeconds, float MaxWaitTime = 10.f, ELevelTick TickType = LEVELTICK_All);

	template<class T = UObject>
	    requires std::is_convertible_v<T*, const UObject*>
	[[nodiscard]] T* FindReplicatedObjectIn(T* Object, const UWorld* World) UE_LIFETIMEBOUND
	{
		auto* Result = StaticFindReplicatedObjectIn(Object, World);
		return Cast<T>(Result);
	}
};

class UEST_API FScopedGame
{
	TSubclassOf<UGameInstance> GameInstanceClass;
	TMap<IConsoleVariable*, FString> CVars;

public:
	[[nodiscard]] FScopedGame();

	[[nodiscard]] FScopedGame& WithGameInstance(TSubclassOf<UGameInstance> InGameInstanceClass) UE_LIFETIMEBOUND;

	[[nodiscard]] FScopedGame& WithConsoleVariable(const FString& Name, FString Value, const bool bReportNonexistentVariable = true) UE_LIFETIMEBOUND;

	[[nodiscard]] FScopedGameInstance Create() const;
};
