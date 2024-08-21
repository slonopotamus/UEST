#pragma once

#include "UESTGameInstance.generated.h"

UINTERFACE()
class UEST_API UUESTGameInstance : public UInterface
{
	GENERATED_BODY()
};

/**
 * Your GameInstance class can implement this interface if you want custom initialization in tests.
 * If you don't, IUESTGameInstance::DefaultInitializeForTests is used.
 */
class UEST_API IUESTGameInstance
{
	GENERATED_BODY()
public:
	virtual void InitializeForTests(const bool bRunAsDedicated, int32 PIEInstance) = 0;

	static void DefaultInitializeForTests(UGameInstance& GameInstance, bool bRunAsDedicated, int32 PIEInstance);
};
