#include "ScopedGame.h"
#include "UEST.h"

TEST(UEST, ScopedGame, Simple)
{
	auto Tester = FScopedGame().Create();

	// You can create dedicated server
	UGameInstance* Server = Tester.CreateGame(EScopedGameType::DedicatedServer, TEXT("/Engine/Maps/Entry"));

	// You can connect a client to it
	UGameInstance* Client = Tester.CreateClientFor(Server);
	ASSERT_THAT(Client, Is::Not::Null);

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
