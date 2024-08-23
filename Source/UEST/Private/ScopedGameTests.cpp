#include "ScopedGame.h"
#include "UEST.h"

TEST(UEST, ScopedGame, Simple)
{
	auto Tester = FScopedGame().Create();

	// You can create a dedicated server
	UGameInstance* Server = Tester.CreateGame(EScopedGameType::Server, TEXT("/Engine/Maps/Entry"));
	ASSERT_THAT(Server, Is::Not::Null);
	ASSERT_THAT(Server->GetWorld()->GetNetMode(), Is::EqualTo<ENetMode>(NM_DedicatedServer));

	// You can connect a client to it
	UGameInstance* Client = Tester.CreateClientFor(Server);
	ASSERT_THAT(Client, Is::Not::Null);

	// Actually, you can connect as many clients as you want! (Unfortunately, max number of game instances is, see MAX_PIE_INSTANCES in UE sources)
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Tester.CreateClientFor(Server);
	}

	// You can access game worlds
	UWorld* ServerWorld = Server->GetWorld();
	ASSERT_THAT(ServerWorld, Is::Not::Null);
	UWorld* ClientWorld = Client->GetWorld();
	ASSERT_THAT(ClientWorld, Is::Not::Null);

	// You can access actors in worlds
	APlayerController* ClientPC = ClientWorld->GetFirstPlayerController();
	ASSERT_THAT(ClientPC, Is::Not::Null);

	// You can lookup matching replicated actors in paired worlds
	APlayerController* ServerPC = Tester.FindReplicatedObjectIn(ClientPC, Server->GetWorld());
	ASSERT_THAT(ServerPC, Is::Not::Null);

	// You can advance game time
	Tester.Tick(1);

	// You can shut down individual game instances
	Tester.DestroyGame(Client);

	// You can also create standalone game worlds
	UGameInstance* Standalone = Tester.CreateGame(EScopedGameType::Client, TEXT("/Engine/Maps/Entry"));

	// Tester automatically cleans everything up when goes out of scope
}

TEST(UEST, ScopedGame, CheckNetMode)
{
	auto Tester = FScopedGame().Create();

	UGameInstance* Server = Tester.CreateGame(EScopedGameType::Server, TEXT("/Engine/Maps/Entry"));
	ASSERT_THAT(Server, Is::Not::Null);

	UWorld* ServerWorld = Server->GetWorld();
	ASSERT_THAT(ServerWorld, Is::Not::Null);
	const auto ServerNetMode = ServerWorld->GetNetMode();
	ASSERT_THAT(ServerNetMode, Is::EqualTo<ENetMode>(NM_DedicatedServer));

	UGameInstance* Client = Tester.CreateClientFor(Server);
	ASSERT_THAT(Client, Is::Not::Null);

	UWorld* ClientWorld = Client->GetWorld();
	ASSERT_THAT(ClientWorld, Is::Not::Null);
	const auto ClientNetMode = ClientWorld->GetNetMode();
	ASSERT_THAT(ClientNetMode, Is::EqualTo<ENetMode>(NM_Client));

	UGameInstance* Standalone = Tester.CreateGame(EScopedGameType::Client, TEXT("/Engine/Maps/Entry"));
	ASSERT_THAT(Standalone, Is::Not::Null);
	UWorld* StandaloneWorld = Standalone->GetWorld();
	ASSERT_THAT(StandaloneWorld, Is::Not::Null);
	const auto StandaloneNetMode = StandaloneWorld->GetNetMode();
	ASSERT_THAT(StandaloneNetMode, Is::EqualTo<ENetMode>(NM_Standalone));
}
