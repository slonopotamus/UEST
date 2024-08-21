#include "UESTGameInstance.h"

// God forgive me
// UGameInstance::WorldContext is not public, and neither InitializeStandalone nor InitializeForPlayInEditor is suitable for us.
// So we use this template hack to access WorldContext field.
// Source: https://ledas.com/post/857-how-to-hack-c-with-templates-and-friends/
template<FWorldContext* UGameInstance::* WorldContext>
struct Stealer {
	static friend FWorldContext*& FieldGetter(UGameInstance& GameInstance) {
		return GameInstance.*WorldContext;
	}
};

template struct Stealer<&UGameInstance::WorldContext>;
static FWorldContext*& FieldGetter(UGameInstance&);

void IUESTGameInstance::DefaultInitializeForTests(UGameInstance& GameInstance, const bool bRunAsDedicated, const int32 PIEInstance)
{
	constexpr auto WorldType = EWorldType::Game;

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
