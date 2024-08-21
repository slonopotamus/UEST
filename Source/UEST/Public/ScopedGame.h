#pragma once

#include "Engine/GameInstance.h"

enum class EScopedGameType : uint8
{
	Server,
	Client,
	Empty,
};

class UEST_API FScopedGameInstance : FNoncopyable
{
	TSubclassOf<UGameInstance> GameInstanceClass;

	TArray<TStrongObjectPtr<UGameInstance>> Games;

	static void DestroyGameInternal(UGameInstance& Game);

	int32 LastPIEInstance;

	bool bGarbageCollectOnDestroy;

	void TickInternal(float DeltaSeconds, const ELevelTick TickType);

	static UObject* StaticFindReplicatedObjectIn(UObject* Object, const UWorld* World);

public:
	static constexpr auto DefaultStepSeconds = 0.1f;

	struct FCVarConfig
	{
		FString Value;
		bool bEnsureIfVariableNotFound;
	};

	explicit FScopedGameInstance(TSubclassOf<UGameInstance> GameInstanceClass, const bool bGarbageCollectOnDestroy, const TMap<FString, FCVarConfig>& CVars);

	FScopedGameInstance(FScopedGameInstance&& Other);

	virtual ~FScopedGameInstance();

	UGameInstance* CreateGame(EScopedGameType Type = EScopedGameType::Client, const FString& MapToLoad = TEXT(""), bool bWaitForConnect = true);

	UGameInstance* CreateClientFor(const UGameInstance* Server, const bool bWaitForConnect = true);

	bool DestroyGame(UGameInstance* Game);

	/** Advances time in all created games by DeltaSeconds in Step increments */
	void Tick(float DeltaSeconds, float StepSeconds = DefaultStepSeconds, ELevelTick TickType = LEVELTICK_All);

	/** Advances time in all created games in StepSeconds increments until Condition returns true */
	bool TickUntil(const TFunction<bool()>& Condition, float StepSeconds = DefaultStepSeconds, float MaxWaitTime = 10.f, ELevelTick TickType = LEVELTICK_All);

	template<class T = UObject>
	    requires std::is_convertible_v<T*, const UObject*>
	T* FindReplicatedObjectIn(T* Object, const UWorld* World)
	{
		auto* Result = StaticFindReplicatedObjectIn(Object, World);
		return Cast<T>(Result);
	}
};

class UEST_API FScopedGame
{
	TSubclassOf<UGameInstance> GameInstanceClass;
	bool bGarbageCollectOnDestroy = true;
	TMap<FString, FScopedGameInstance::FCVarConfig> CVars;

public:
	FScopedGame();

	FScopedGame& WithGameInstance(TSubclassOf<UGameInstance> InGameInstanceClass)
	{
		GameInstanceClass = MoveTemp(InGameInstanceClass);
		return *this;
	}

	FScopedGame& WithConsoleVariable(FString Name, FString Value, const bool bEnsureIfVariableNotFound = true)
	{
		CVars.Add(MoveTemp(Name), {MoveTemp(Value), bEnsureIfVariableNotFound});
		return *this;
	}

	FScopedGame& WithGarbageCollectOnDestroy(const bool bInGarbageCollectOnDestroy)
	{
		bGarbageCollectOnDestroy = bInGarbageCollectOnDestroy;
		return *this;
	}

	FScopedGameInstance Create() const;
};
