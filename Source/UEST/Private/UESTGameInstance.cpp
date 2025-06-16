#include "UESTGameInstance.h"

// God forgive me
// UGameInstance::WorldContext is not public, and neither InitializeStandalone nor InitializeForPlayInEditor is suitable for us.
// So we use this template hack to access WorldContext field.
// Source: https://ledas.com/post/857-how-to-hack-c-with-templates-and-friends/
static FWorldContext*& FieldGetter(UGameInstance&);

template<typename T, auto T::*Field, typename RetVal>
struct Stealer
{
	friend RetVal& FieldGetter(T& Object)
	{
		return Object.*Field;
	}
};

template struct Stealer<UGameInstance, &UGameInstance::WorldContext, FWorldContext*>;

void IUESTGameInstance::DefaultInitializeForTests(UGameInstance& GameInstance, const bool bRunAsDedicated, const int32 PIEInstance)
{
	constexpr auto WorldType = WITH_EDITOR ? EWorldType::PIE : EWorldType::Game;

	auto& WorldContext = GameInstance.GetEngine()->CreateNewWorldContext(WorldType);
	WorldContext.OwningGameInstance = &GameInstance;
	WorldContext.PIEInstance = PIEInstance;
	WorldContext.PIEPrefix = UWorld::BuildPIEPackagePrefix(PIEInstance);

	// We want to init this before calling UGameInstance::Init
	WorldContext.RunAsDedicated = bRunAsDedicated;

	FieldGetter(GameInstance) = &WorldContext;

	auto* DummyWorld = UWorld::CreateWorld(WorldType, true);
	DummyWorld->SetGameInstance(&GameInstance);
	WorldContext.SetCurrentWorld(DummyWorld);

	GameInstance.Init();
}
